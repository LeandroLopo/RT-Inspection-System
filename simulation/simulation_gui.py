import json
import math
import random
import time
from dataclasses import dataclass
from threading import Lock

import paho.mqtt.client as mqtt
import pygame


# ============================================================
# Configuracoes MQTT
# ============================================================

BROKER = "localhost"
PORTA = 1883

TOPICO_SENSORES = "atr/sim/sensors"
TOPICO_ATUADORES = "atr/core/actuators"


# ============================================================
# Configuracoes da simulacao fisica
# ============================================================

PERIODO_SIMULACAO = 0.08      # 80 ms
COMPRIMENTO_TUNEL = 35.0       # metros

VELOCIDADE_MAXIMA = 3.0        # m/s
ACELERACAO_MAXIMA = 1.8        # m/s²
ATRITO = 0.35                   # resistencia proporcional a velocidade

DISTANCIA_NORMAL_TETO = 100.0  # centimetros
DESVIO_PADRAO_RUIDO = 0.8      # centimetros


# ============================================================
# Configuracoes graficas
# ============================================================

LARGURA_JANELA = 1280
ALTURA_JANELA = 720

MARGEM_ESQUERDA = 70
MARGEM_DIREITA = 40

Y_CHAO = 585
Y_SENSOR_ROBO = 540

LARGURA_AREA_TUNEL = (
    LARGURA_JANELA - MARGEM_ESQUERDA - MARGEM_DIREITA
)

ESCALA_VERTICAL_LIDAR = 2.35

COR_FUNDO = (19, 24, 33)
COR_PAINEL = (30, 38, 50)
COR_TEXTO = (235, 240, 245)
COR_TEXTO_SECUNDARIO = (160, 175, 190)

COR_CHAO = (120, 125, 135)
COR_TETO_NORMAL = (85, 170, 225)
COR_TETO_FALHA = (235, 85, 85)
COR_LIDAR = (70, 240, 130)

COR_ROBO = (235, 185, 55)
COR_RODA = (40, 45, 50)
COR_CAMERA_DESLIGADA = (90, 100, 110)
COR_CAMERA_LIGADA = (235, 60, 65)

COR_AREA_FALHA = (75, 35, 42)
COR_AREA_SALIENCIA = (38, 52, 72)


# ============================================================
# Atuadores recebidos do nucleo C++
# ============================================================

@dataclass
class Atuadores:
    aceleracao: int = 0
    camera_ligada: bool = False


# ============================================================
# Simulacao do robo e do tunel
# ============================================================

class SimulacaoTunel:
    def __init__(self) -> None:
        self.x = 0.0
        self.velocidade = 0.0

        self.estado_encoder = False
        self.ultimo_metro_encoder = 0

        self.atuadores = Atuadores()
        self.mutex_atuadores = Lock()

        self.inicio = time.monotonic()

        self.sensor_atual = {
            "i_encoder": False,
            "i_lidar": 100,
            "velocidade": 0.0,
            "posicao_real": 0.0,
            "timestamp": 0.0
        }

        self.mqtt_conectado = False
        self.finalizado = False
        self.mensagem_status = "Aguardando conexao MQTT..."

    def distancia_teto(self, x: float) -> float:
        """
        Distancia entre o sensor do robo e o teto, em centimetros.

        12 m a 15 m:
            Falha severa: cavidade no teto.

        24 m a 26 m:
            Pequena saliencia, menor que a falha principal.
        """
        if 12.0 <= x <= 15.0:
            return 128.0

        if 24.0 <= x <= 26.0:
            return 94.0

        ondulacao = 1.5 * math.sin(x * 0.6)
        return DISTANCIA_NORMAL_TETO + ondulacao

    def atualizar_encoder(self) -> None:
        metro_atual = int(self.x)

        while self.ultimo_metro_encoder < metro_atual:
            self.estado_encoder = not self.estado_encoder
            self.ultimo_metro_encoder += 1

        while self.ultimo_metro_encoder > metro_atual:
            self.estado_encoder = not self.estado_encoder
            self.ultimo_metro_encoder -= 1

    def atualizar_movimento(self, dt: float) -> None:
        with self.mutex_atuadores:
            comando_motor = self.atuadores.aceleracao

        aceleracao_motor = (
            comando_motor / 100.0
        ) * ACELERACAO_MAXIMA

        aceleracao_resultante = (
            aceleracao_motor - ATRITO * self.velocidade
        )

        self.velocidade += aceleracao_resultante * dt

        self.velocidade = max(
            -VELOCIDADE_MAXIMA,
             min(self.velocidade, VELOCIDADE_MAXIMA)
        )

        self.x += self.velocidade * dt

        if self.x <= 0.0:
            self.x = 0.0
            self.velocidade = max(0.0, self.velocidade)

        if self.x >= COMPRIMENTO_TUNEL:
            self.x = COMPRIMENTO_TUNEL
            self.finalizado = True

        self.atualizar_encoder()

    def gerar_sensor(self) -> dict:
        distancia_real = self.distancia_teto(self.x)
        ruido = random.gauss(0.0, DESVIO_PADRAO_RUIDO)

        leitura_lidar = int(round(distancia_real + ruido))

        timestamp = round(time.monotonic() - self.inicio, 3)

        self.sensor_atual = {
            "i_encoder": self.estado_encoder,
            "i_lidar": leitura_lidar,
            "velocidade": round(self.velocidade, 3),
            "posicao_real": round(self.x, 3),
            "timestamp": timestamp
        }

        return self.sensor_atual

    def receber_atuadores(self, mensagem: dict) -> None:
        if mensagem.get("finalizado", False):
            return

        aceleracao = int(mensagem.get("o_aceleracao", 0))
        camera_ligada = bool(mensagem.get("o_liga_camera", False))

        with self.mutex_atuadores:
            self.atuadores.aceleracao = aceleracao
            self.atuadores.camera_ligada = camera_ligada

        if camera_ligada:
            self.mensagem_status = "Inspecao em andamento: camera acionada"
        else:
            self.mensagem_status = "Simulacao em execucao"


# ============================================================
# Conversoes de coordenadas
# ============================================================

def x_para_tela(x: float) -> int:
    proporcao = x / COMPRIMENTO_TUNEL

    return int(
        MARGEM_ESQUERDA +
        proporcao * LARGURA_AREA_TUNEL
    )


def distancia_para_y_teto(distancia: float) -> int:
    return int(
        Y_SENSOR_ROBO -
        distancia * ESCALA_VERTICAL_LIDAR
    )


# ============================================================
# Desenho da interface
# ============================================================

def desenhar_texto(
    tela: pygame.Surface,
    fonte: pygame.font.Font,
    texto: str,
    posicao: tuple[int, int],
    cor: tuple[int, int, int] = COR_TEXTO
) -> None:
    superficie = fonte.render(texto, True, cor)
    tela.blit(superficie, posicao)


def desenhar_areas_especiais(tela: pygame.Surface) -> None:
    inicio_falha = x_para_tela(12.0)
    fim_falha = x_para_tela(15.0)

    pygame.draw.rect(
        tela,
        COR_AREA_FALHA,
        pygame.Rect(
            inicio_falha,
            95,
            fim_falha - inicio_falha,
            Y_CHAO - 95
        )
    )

    inicio_salien = x_para_tela(24.0)
    fim_salien = x_para_tela(26.0)

    pygame.draw.rect(
        tela,
        COR_AREA_SALIENCIA,
        pygame.Rect(
            inicio_salien,
            95,
            fim_salien - inicio_salien,
            Y_CHAO - 95
        )
    )


def desenhar_tunel(
    tela: pygame.Surface,
    simulacao: SimulacaoTunel
) -> None:
    pontos_teto = []

    quantidade_amostras = 250

    for indice in range(quantidade_amostras + 1):
        x_real = (
            COMPRIMENTO_TUNEL *
            indice /
            quantidade_amostras
        )

        distancia = simulacao.distancia_teto(x_real)

        pontos_teto.append(
            (
                x_para_tela(x_real),
                distancia_para_y_teto(distancia)
            )
        )

    for indice in range(len(pontos_teto) - 1):
        x_real = (
            COMPRIMENTO_TUNEL *
            indice /
            quantidade_amostras
        )

        cor = (
            COR_TETO_FALHA
            if 12.0 <= x_real <= 15.0
            else COR_TETO_NORMAL
        )

        pygame.draw.line(
            tela,
            cor,
            pontos_teto[indice],
            pontos_teto[indice + 1],
            4
        )

    pygame.draw.line(
        tela,
        COR_CHAO,
        (MARGEM_ESQUERDA, Y_CHAO),
        (LARGURA_JANELA - MARGEM_DIREITA, Y_CHAO),
        4
    )

    for metro in range(0, int(COMPRIMENTO_TUNEL) + 1, 5):
        x_tela = x_para_tela(float(metro))

        pygame.draw.line(
            tela,
            COR_CHAO,
            (x_tela, Y_CHAO),
            (x_tela, Y_CHAO + 9),
            2
        )


def desenhar_robo(
    tela: pygame.Surface,
    simulacao: SimulacaoTunel
) -> None:
    x_robo = x_para_tela(simulacao.x)

    largura_robo = 55
    altura_robo = 30

    corpo = pygame.Rect(
        x_robo - largura_robo // 2,
        Y_CHAO - altura_robo - 12,
        largura_robo,
        altura_robo
    )

    pygame.draw.rect(
        tela,
        COR_ROBO,
        corpo,
        border_radius=6
    )

    pygame.draw.circle(
        tela,
        COR_RODA,
        (x_robo - 17, Y_CHAO - 10),
        8
    )

    pygame.draw.circle(
        tela,
        COR_RODA,
        (x_robo + 17, Y_CHAO - 10),
        8
    )

    sensor_x = x_robo
    sensor_y = corpo.top

    distancia_sensor = simulacao.sensor_atual["i_lidar"]
    y_teto_medido = distancia_para_y_teto(distancia_sensor)

    pygame.draw.circle(
        tela,
        COR_LIDAR,
        (sensor_x, sensor_y),
        4
    )

    pygame.draw.line(
        tela,
        COR_LIDAR,
        (sensor_x, sensor_y),
        (sensor_x, y_teto_medido),
        2
    )

    with simulacao.mutex_atuadores:
        camera_ligada = simulacao.atuadores.camera_ligada

    cor_camera = (
        COR_CAMERA_LIGADA
        if camera_ligada
        else COR_CAMERA_DESLIGADA
    )

    pygame.draw.circle(
        tela,
        cor_camera,
        (corpo.right - 8, corpo.top + 8),
        6
    )

    if camera_ligada:
        pygame.draw.circle(
            tela,
            COR_CAMERA_LIGADA,
            (corpo.right - 8, corpo.top + 8),
            14,
            2
        )


def desenhar_painel(
    tela: pygame.Surface,
    fonte_titulo: pygame.font.Font,
    fonte: pygame.font.Font,
    fonte_pequena: pygame.font.Font,
    simulacao: SimulacaoTunel
) -> None:
    pygame.draw.rect(
        tela,
        COR_PAINEL,
        pygame.Rect(0, 0, LARGURA_JANELA, 84)
    )

    desenhar_texto(
        tela,
        fonte_titulo,
        "RT Inspection System - Simulacao do Tunel",
        (24, 14)
    )

    status_cor = (
        COR_LIDAR
        if simulacao.mqtt_conectado
        else COR_CAMERA_LIGADA
    )

    status_mqtt = (
        "MQTT conectado"
        if simulacao.mqtt_conectado
        else "MQTT desconectado"
    )

    desenhar_texto(
        tela,
        fonte_pequena,
        status_mqtt,
        (28, 51),
        status_cor
    )

    sensor = simulacao.sensor_atual

    with simulacao.mutex_atuadores:
        aceleracao = simulacao.atuadores.aceleracao
        camera_ligada = simulacao.atuadores.camera_ligada

    valores = [
        f"Posicao: {sensor['posicao_real']:.2f} m",
        f"Velocidade: {sensor['velocidade']:.2f} m/s",
        f"LIDAR: {sensor['i_lidar']} cm",
        f"Encoder: {int(sensor['i_encoder'])}",
        f"Motor: {aceleracao}",
        f"Camera: {'LIGADA' if camera_ligada else 'DESLIGADA'}"
    ]

    x_inicial = 460

    for indice, valor in enumerate(valores):
        desenhar_texto(
            tela,
            fonte,
            valor,
            (x_inicial + (indice % 3) * 235, 14 + (indice // 3) * 33)
        )

    desenhar_texto(
        tela,
        fonte_pequena,
        simulacao.mensagem_status,
        (MARGEM_ESQUERDA, 625),
        COR_TEXTO_SECUNDARIO
    )


def desenhar_legenda(
    tela: pygame.Surface,
    fonte_pequena: pygame.font.Font
) -> None:
    itens = [
        ("Teto normal", COR_TETO_NORMAL),
        ("Falha severa", COR_TETO_FALHA),
        ("Feixe LIDAR", COR_LIDAR),
        ("Camera ativa", COR_CAMERA_LIGADA)
    ]

    x_atual = MARGEM_ESQUERDA

    for texto, cor in itens:
        pygame.draw.rect(
            tela,
            cor,
            pygame.Rect(x_atual, 670, 18, 12),
            border_radius=2
        )

        desenhar_texto(
            tela,
            fonte_pequena,
            texto,
            (x_atual + 26, 667),
            COR_TEXTO_SECUNDARIO
        )

        x_atual += 185


def desenhar_tela_final(
    tela: pygame.Surface,
    fonte_titulo: pygame.font.Font,
    fonte: pygame.font.Font,
    simulacao: SimulacaoTunel
) -> None:
    superficie = pygame.Surface(
        (LARGURA_JANELA, ALTURA_JANELA),
        pygame.SRCALPHA
    )

    superficie.fill((0, 0, 0, 175))
    tela.blit(superficie, (0, 0))

    texto_final = "Simulacao finalizada"

    largura_texto = fonte_titulo.size(texto_final)[0]

    desenhar_texto(
        tela,
        fonte_titulo,
        texto_final,
        (
            LARGURA_JANELA // 2 - largura_texto // 2,
            ALTURA_JANELA // 2 - 45
        )
    )

    resultado = (
        f"Tunel percorrido: {simulacao.x:.2f} m  |  "
        f"Velocidade final: {simulacao.velocidade:.2f} m/s"
    )

    largura_resultado = fonte.size(resultado)[0]

    desenhar_texto(
        tela,
        fonte,
        resultado,
        (
            LARGURA_JANELA // 2 - largura_resultado // 2,
            ALTURA_JANELA // 2 + 8
        ),
        COR_TEXTO_SECUNDARIO
    )


# ============================================================
# MQTT
# ============================================================

simulacao = SimulacaoTunel()


def ao_conectar(
    cliente: mqtt.Client,
    dados_usuario,
    flags,
    codigo_motivo,
    propriedades
) -> None:
    simulacao.mqtt_conectado = True
    simulacao.mensagem_status = "MQTT conectado. Aguardando atuadores do nucleo C++."

    cliente.subscribe(TOPICO_ATUADORES)

    print(f"Simulacao conectada ao broker MQTT: {codigo_motivo}")
    print(f"Assinando atuadores em: {TOPICO_ATUADORES}")
    print(f"Publicando sensores em: {TOPICO_SENSORES}")


def ao_desconectar(
    cliente: mqtt.Client,
    dados_usuario,
    flags_desconexao,
    codigo_motivo,
    propriedades
) -> None:
    simulacao.mqtt_conectado = False
    simulacao.mensagem_status = "Conexao MQTT encerrada."


def ao_receber_mensagem(
    cliente: mqtt.Client,
    dados_usuario,
    mensagem: mqtt.MQTTMessage
) -> None:
    try:
        payload = mensagem.payload.decode("utf-8")
        dados = json.loads(payload)

        simulacao.receber_atuadores(dados)

    except (UnicodeDecodeError, json.JSONDecodeError, ValueError) as erro:
        simulacao.mensagem_status = f"Erro ao receber atuadores: {erro}"


# ============================================================
# Execucao principal
# ============================================================

def main() -> None:
    pygame.init()

    tela = pygame.display.set_mode(
        (LARGURA_JANELA, ALTURA_JANELA)
    )

    pygame.display.set_caption(
        "RT Inspection System - Simulação"
    )

    relogio = pygame.time.Clock()

    fonte_titulo = pygame.font.SysFont("arial", 26, bold=True)
    fonte = pygame.font.SysFont("arial", 18)
    fonte_pequena = pygame.font.SysFont("arial", 15)

    cliente = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

    cliente.on_connect = ao_conectar
    cliente.on_disconnect = ao_desconectar
    cliente.on_message = ao_receber_mensagem

    try:
        cliente.connect(BROKER, PORTA, 60)
        cliente.loop_start()

    except OSError as erro:
        simulacao.mensagem_status = (
            f"Erro MQTT: {erro}. Verifique se o Mosquitto esta ativo."
        )

    executando = True
    finalizacao_enviada = False
    instante_finalizacao = None

    acumulador_tempo = 0.0

    print("Interface grafica da simulacao iniciada.")
    print("A regiao vermelha entre 12 m e 15 m representa a falha severa.")

    while executando:
        dt_real = relogio.tick(60) / 1000.0
        acumulador_tempo += dt_real

        for evento in pygame.event.get():
            if evento.type == pygame.QUIT:
                executando = False

            if evento.type == pygame.KEYDOWN:
                if evento.key == pygame.K_ESCAPE:
                    executando = False

        while (
            acumulador_tempo >= PERIODO_SIMULACAO
            and not simulacao.finalizado
        ):
            simulacao.atualizar_movimento(PERIODO_SIMULACAO)

            sensor = simulacao.gerar_sensor()

            if simulacao.mqtt_conectado:
                cliente.publish(
                    TOPICO_SENSORES,
                    json.dumps(sensor)
                )

            acumulador_tempo -= PERIODO_SIMULACAO

        if simulacao.finalizado and not finalizacao_enviada:
            cliente.publish(
                TOPICO_SENSORES,
                json.dumps({"finalizado": True})
            )

            finalizacao_enviada = True
            instante_finalizacao = time.monotonic()
            simulacao.mensagem_status = (
                "Percurso encerrado. Sensores finalizados."
            )

        tela.fill(COR_FUNDO)

        desenhar_areas_especiais(tela)
        desenhar_tunel(tela, simulacao)
        desenhar_robo(tela, simulacao)

        desenhar_painel(
            tela,
            fonte_titulo,
            fonte,
            fonte_pequena,
            simulacao
        )

        desenhar_legenda(tela, fonte_pequena)

        if simulacao.finalizado:
            desenhar_tela_final(
                tela,
                fonte_titulo,
                fonte,
                simulacao
            )

            if (
                instante_finalizacao is not None
                and time.monotonic() - instante_finalizacao >= 4.0
            ):
                executando = False

        pygame.display.flip()

    if not finalizacao_enviada:
        cliente.publish(
            TOPICO_SENSORES,
            json.dumps({"finalizado": True})
        )

        time.sleep(0.2)

    cliente.loop_stop()
    cliente.disconnect()

    pygame.quit()

    print("Simulacao grafica encerrada.")


if __name__ == "__main__":
    main()
