import json
import time
from dataclasses import dataclass, field
from threading import Lock

import paho.mqtt.client as mqtt
import pygame


# ============================================================
# Configuracoes MQTT
# ============================================================

BROKER = "localhost"
PORTA = 1883

TOPICO_COMANDOS = "atr/remote/commands"
TOPICO_ESTADO = "atr/core/state"
TOPICO_SUPERFICIE = "atr/core/surface"


# ============================================================
# Configuracoes graficas
# ============================================================

LARGURA_JANELA = 1200
ALTURA_JANELA = 720

COR_FUNDO = (18, 23, 31)
COR_PAINEL = (29, 37, 48)
COR_PAINEL_SECUNDARIO = (35, 44, 57)

COR_TEXTO = (234, 239, 244)
COR_TEXTO_SECUNDARIO = (156, 171, 186)

COR_VERDE = (66, 214, 122)
COR_VERMELHO = (231, 76, 76)
COR_AMARELO = (240, 190, 70)
COR_AZUL = (76, 164, 232)
COR_CINZA = (102, 112, 126)

COR_GRAFICO = (65, 192, 226)
COR_LINHA_LIMITE = (236, 106, 106)


# ============================================================
# Estado local
# ============================================================

@dataclass
class ComandoRemoto:
    c_automatico: bool = True
    c_man: bool = False
    c_direita: bool = False
    c_esquerda: bool = False
    c_para: bool = False
    j_sp_velocidade: int = 2
    limite_falha: float = 5.0


@dataclass
class EstadoRecebido:
    e_automatico: bool = True
    e_inspecao: bool = False
    posicao_x: float = 0.0
    velocidade: float = 0.0
    o_aceleracao: int = 0
    o_liga_camera: bool = False
    limite_falha: float = 5.0


@dataclass
class DadosInterface:
    comando: ComandoRemoto = field(default_factory=ComandoRemoto)
    estado: EstadoRecebido = field(default_factory=EstadoRecebido)

    pontos_superficie: dict[float, float] = field(default_factory=dict)

    conectado: bool = False
    finalizado: bool = False
    status: str = "Aguardando conexao MQTT..."

    mutex: Lock = field(default_factory=Lock)


dados = DadosInterface()


# ============================================================
# MQTT
# ============================================================

def publicar_comando(cliente: mqtt.Client) -> None:
    with dados.mutex:
        mensagem = {
            "c_automatico": dados.comando.c_automatico,
            "c_man": dados.comando.c_man,
            "c_direita": dados.comando.c_direita,
            "c_esquerda": dados.comando.c_esquerda,
            "c_para": dados.comando.c_para,
            "j_sp_velocidade": dados.comando.j_sp_velocidade,
            "limite_falha": dados.comando.limite_falha
        }

    cliente.publish(
        TOPICO_COMANDOS,
        json.dumps(mensagem)
    )


def ao_conectar(
    cliente: mqtt.Client,
    dados_usuario,
    flags,
    codigo_motivo,
    propriedades
) -> None:
    with dados.mutex:
        dados.conectado = True
        dados.status = "Interface conectada ao nucleo."

    cliente.subscribe(TOPICO_ESTADO)
    cliente.subscribe(TOPICO_SUPERFICIE)

    publicar_comando(cliente)

    print(f"Operacao remota conectada ao broker MQTT: {codigo_motivo}")
    print(f"Publicando comandos em: {TOPICO_COMANDOS}")
    print(f"Recebendo estado em: {TOPICO_ESTADO}")
    print(f"Recebendo superficie em: {TOPICO_SUPERFICIE}")


def ao_desconectar(
    cliente: mqtt.Client,
    dados_usuario,
    flags_desconexao,
    codigo_motivo,
    propriedades
) -> None:
    with dados.mutex:
        dados.conectado = False
        dados.status = "Conexao MQTT encerrada."


def ao_receber_mensagem(
    cliente: mqtt.Client,
    dados_usuario,
    mensagem: mqtt.MQTTMessage
) -> None:
    try:
        payload = mensagem.payload.decode("utf-8")
        conteudo = json.loads(payload)

        if mensagem.topic == TOPICO_ESTADO:
            if conteudo.get("finalizado", False):
                with dados.mutex:
                    dados.finalizado = True
                    dados.status = "Execucao finalizada pelo nucleo."
                return

            with dados.mutex:
                dados.estado.e_automatico = bool(
                    conteudo.get("e_automatico", True)
                )

                dados.estado.e_inspecao = bool(
                    conteudo.get("e_inspecao", False)
                )

                dados.estado.posicao_x = float(
                    conteudo.get("posicao_x", 0.0)
                )

                dados.estado.velocidade = float(
                    conteudo.get("velocidade", 0.0)
                )

                dados.estado.o_aceleracao = int(
                    conteudo.get("o_aceleracao", 0)
                )

                dados.estado.o_liga_camera = bool(
                    conteudo.get("o_liga_camera", False)
                )

                dados.estado.limite_falha = float(
                    conteudo.get("limite_falha", 10.0)
                )

        elif mensagem.topic == TOPICO_SUPERFICIE:
            if conteudo.get("finalizado", False):
                return

            x = float(conteudo.get("x", 0.0))
            y = float(conteudo.get("y", 0.0))

            with dados.mutex:
                dados.pontos_superficie[round(x, 2)] = y

    except (UnicodeDecodeError, json.JSONDecodeError, ValueError) as erro:
        with dados.mutex:
            dados.status = f"Erro MQTT: {erro}"


# ============================================================
# Desenho
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


def desenhar_cartao(
    tela: pygame.Surface,
    fonte: pygame.font.Font,
    titulo: str,
    valor: str,
    x: int,
    y: int,
    largura: int,
    cor_valor: tuple[int, int, int] = COR_TEXTO
) -> None:
    pygame.draw.rect(
        tela,
        COR_PAINEL_SECUNDARIO,
        pygame.Rect(x, y, largura, 78),
        border_radius=8
    )

    desenhar_texto(
        tela,
        fonte,
        titulo,
        (x + 14, y + 12),
        COR_TEXTO_SECUNDARIO
    )

    desenhar_texto(
        tela,
        fonte,
        valor,
        (x + 14, y + 43),
        cor_valor
    )


def desenhar_botao(
    tela: pygame.Surface,
    fonte: pygame.font.Font,
    texto: str,
    retangulo: pygame.Rect,
    ativo: bool = False
) -> None:
    cor = COR_AZUL if ativo else COR_PAINEL_SECUNDARIO

    pygame.draw.rect(
        tela,
        cor,
        retangulo,
        border_radius=7
    )

    largura_texto = fonte.size(texto)[0]
    altura_texto = fonte.size(texto)[1]

    desenhar_texto(
        tela,
        fonte,
        texto,
        (
            retangulo.centerx - largura_texto // 2,
            retangulo.centery - altura_texto // 2
        )
    )


def desenhar_grafico(
    tela: pygame.Surface,
    fonte: pygame.font.Font,
    pontos: list[tuple[float, float]]
) -> None:
    area = pygame.Rect(35, 308, 765, 342)

    pygame.draw.rect(
        tela,
        COR_PAINEL,
        area,
        border_radius=10
    )

    desenhar_texto(
        tela,
        fonte,
        "Superficie reconstruida pelo LIDAR",
        (55, 325)
    )

    grafico = pygame.Rect(65, 372, 700, 230)

    pygame.draw.rect(
        tela,
        (21, 28, 38),
        grafico,
        border_radius=6
    )

    pygame.draw.line(
        tela,
        COR_CINZA,
        (grafico.left, grafico.bottom),
        (grafico.right, grafico.bottom),
        2
    )

    pygame.draw.line(
        tela,
        COR_CINZA,
        (grafico.left, grafico.top),
        (grafico.left, grafico.bottom),
        2
    )

    for metro in range(0, 36, 5):
        x_tela = grafico.left + int((metro / 35.0) * grafico.width)

        pygame.draw.line(
            tela,
            COR_CINZA,
            (x_tela, grafico.bottom),
            (x_tela, grafico.bottom + 6),
            1
        )

        desenhar_texto(
            tela,
            fonte,
            str(metro),
            (x_tela - 7, grafico.bottom + 9),
            COR_TEXTO_SECUNDARIO
        )

    for distancia in [80, 100, 120, 140]:
        y_tela = grafico.bottom - int(((distancia - 80) / 60.0) * grafico.height)

        pygame.draw.line(
            tela,
            (42, 51, 64),
            (grafico.left, y_tela),
            (grafico.right, y_tela),
            1
        )

        desenhar_texto(
            tela,
            fonte,
            str(distancia),
            (grafico.left - 38, y_tela - 8),
            COR_TEXTO_SECUNDARIO
        )

    if len(pontos) >= 2:
        pontos_tela = []

        for x, y in pontos:
            x_limitado = max(0.0, min(x, 35.0))
            y_limitado = max(80.0, min(y, 140.0))

            x_tela = grafico.left + int(
                (x_limitado / 35.0) * grafico.width
            )

            y_tela = grafico.bottom - int(
                ((y_limitado - 80.0) / 60.0) * grafico.height
            )

            pontos_tela.append((x_tela, y_tela))

        pygame.draw.lines(
            tela,
            COR_GRAFICO,
            False,
            pontos_tela,
            3
        )


def desenhar_interface(
    tela: pygame.Surface,
    fonte_titulo: pygame.font.Font,
    fonte: pygame.font.Font,
    fonte_pequena: pygame.font.Font
) -> None:
    with dados.mutex:
        estado = EstadoRecebido(
            e_automatico=dados.estado.e_automatico,
            e_inspecao=dados.estado.e_inspecao,
            posicao_x=dados.estado.posicao_x,
            velocidade=dados.estado.velocidade,
            o_aceleracao=dados.estado.o_aceleracao,
            o_liga_camera=dados.estado.o_liga_camera,
            limite_falha=dados.estado.limite_falha
        )

        comando = ComandoRemoto(
            c_automatico=dados.comando.c_automatico,
            c_man=dados.comando.c_man,
            c_direita=dados.comando.c_direita,
            c_esquerda=dados.comando.c_esquerda,
            c_para=dados.comando.c_para,
            j_sp_velocidade=dados.comando.j_sp_velocidade,
            limite_falha=dados.comando.limite_falha
        )
    with dados.mutex:
        pontos = sorted(dados.pontos_superficie.items())        
        conectado = dados.conectado
        status = dados.status

    tela.fill(COR_FUNDO)

    pygame.draw.rect(
        tela,
        COR_PAINEL,
        pygame.Rect(0, 0, LARGURA_JANELA, 72)
    )

    desenhar_texto(
        tela,
        fonte_titulo,
        "RT Inspection System - Operacao Remota",
        (28, 20)
    )

    status_mqtt = "MQTT conectado" if conectado else "MQTT desconectado"
    cor_mqtt = COR_VERDE if conectado else COR_VERMELHO

    desenhar_texto(
        tela,
        fonte_pequena,
        status_mqtt,
        (1015, 27),
        cor_mqtt
    )

    modo = "AUTOMATICO" if estado.e_automatico else "MANUAL"
    cor_modo = COR_VERDE if estado.e_automatico else COR_AMARELO

    desenhar_cartao(
        tela, fonte, "Modo", modo, 35, 99, 175, cor_modo
    )

    desenhar_cartao(
        tela,
        fonte,
        "Posição",
        f"{estado.posicao_x:.2f} m",
        225,
        99,
        175
    )

    desenhar_cartao(
        tela,
        fonte,
        "Velocidade",
        f"{estado.velocidade:.2f} m/s",
        415,
        99,
        175
    )

    cor_camera = COR_VERMELHO if estado.o_liga_camera else COR_CINZA

    desenhar_cartao(
        tela,
        fonte,
        "Camera",
        "LIGADA" if estado.o_liga_camera else "DESLIGADA",
        605,
        99,
        175,
        cor_camera
    )

    cor_inspecao = COR_VERMELHO if estado.e_inspecao else COR_VERDE

    desenhar_cartao(
        tela,
        fonte,
        "Inspecao",
        "ATIVA" if estado.e_inspecao else "NORMAL",
        795,
        99,
        175,
        cor_inspecao
    )

    desenhar_cartao(
        tela,
        fonte,
        "Aceleracao",
        str(estado.o_aceleracao),
        985,
        99,
        175
    )

    desenhar_texto(
        tela,
        fonte,
        f"Setpoint atual: {comando.j_sp_velocidade} m/s",
        (45, 220)
    )

    desenhar_texto(
        tela,
        fonte,
        f"Limite de falha: {comando.limite_falha:.1f} cm",
        (275, 220)
    )

    desenhar_texto(
        tela,
        fonte_pequena,
        status,
        (45, 264),
        COR_TEXTO_SECUNDARIO
    )

    desenhar_grafico(tela, fonte_pequena, pontos)

    pygame.draw.rect(
        tela,
        COR_PAINEL,
        pygame.Rect(830, 308, 330, 342),
        border_radius=10
    )

    desenhar_texto(
        tela,
        fonte,
        "Comandos",
        (852, 330)
    )

    botao_automatico = pygame.Rect(852, 378, 132, 45)
    botao_manual = pygame.Rect(1000, 378, 132, 45)

    desenhar_botao(
        tela,
        fonte_pequena,
        "A - Automatico",
        botao_automatico,
        comando.c_automatico
    )

    desenhar_botao(
        tela,
        fonte_pequena,
        "M - Manual",
        botao_manual,
        comando.c_man
    )

    botao_esquerda = pygame.Rect(852, 445, 82, 46)
    botao_parar = pygame.Rect(950, 445, 82, 46)
    botao_direita = pygame.Rect(1050, 445, 82, 46)

    desenhar_botao(
        tela,
        fonte,
        "<",
        botao_esquerda,
        comando.c_esquerda
    )

    desenhar_botao(
        tela,
        fonte_pequena,
        "PARAR",
        botao_parar,
        comando.c_para
    )

    desenhar_botao(
        tela,
        fonte,
        ">",
        botao_direita,
        comando.c_direita
    )

    instrucoes = [
        "A: modo automatico",
        "M: modo manual",
        "Setas: movimento manual",
        "ESPACO: parar",
        "+ / -: alterar setpoint",
        "[ / ]: alterar limite",
        "ESC: sair"
    ]

    for indice, linha in enumerate(instrucoes):
        desenhar_texto(
            tela,
            fonte_pequena,
            linha,
            (852, 530 + indice * 17),
            COR_TEXTO_SECUNDARIO
        )


# ============================================================
# Tratamento de comandos do teclado
# ============================================================

def processar_tecla_pressionada(
    tecla: int,
    cliente: mqtt.Client
) -> None:
    with dados.mutex:
        if tecla == pygame.K_a:
            dados.comando.c_automatico = True
            dados.comando.c_man = False
            dados.comando.c_direita = False
            dados.comando.c_esquerda = False
            dados.comando.c_para = False
            dados.status = "Modo automatico solicitado."

        elif tecla == pygame.K_m:
            dados.comando.c_automatico = False
            dados.comando.c_man = True
            dados.comando.c_direita = False
            dados.comando.c_esquerda = False
            dados.comando.c_para = False
            dados.status = "Modo manual solicitado."

        elif tecla == pygame.K_RIGHT:
            dados.comando.c_automatico = False
            dados.comando.c_man = True
            dados.comando.c_direita = True
            dados.comando.c_esquerda = False
            dados.comando.c_para = False
            dados.status = "Acelerando no modo manual."

        elif tecla == pygame.K_LEFT:
            dados.comando.c_automatico = False
            dados.comando.c_man = True
            dados.comando.c_direita = False
            dados.comando.c_esquerda = True
            dados.comando.c_para = False
            dados.status = "Freando no modo manual."

        elif tecla == pygame.K_SPACE:
            dados.comando.c_automatico = False
            dados.comando.c_man = True
            dados.comando.c_direita = False
            dados.comando.c_esquerda = False
            dados.comando.c_para = True
            dados.status = "Parada manual solicitada."

        elif tecla in (pygame.K_PLUS, pygame.K_KP_PLUS, pygame.K_EQUALS):
            dados.comando.j_sp_velocidade = min(
                3,
                dados.comando.j_sp_velocidade + 1
            )

            dados.status = (
                f"Setpoint alterado para "
                f"{dados.comando.j_sp_velocidade} m/s."
            )

        elif tecla in (pygame.K_MINUS, pygame.K_KP_MINUS):
            dados.comando.j_sp_velocidade = max(
                0,
                dados.comando.j_sp_velocidade - 1
            )

            dados.status = (
                f"Setpoint alterado para "
                f"{dados.comando.j_sp_velocidade} m/s."
            )

        elif tecla == pygame.K_LEFTBRACKET:
            dados.comando.limite_falha = max(
                1.0,
                dados.comando.limite_falha - 1.0
            )

            dados.status = (
                f"Limite de falha alterado para "
                f"{dados.comando.limite_falha:.1f} cm."
            )

        elif tecla == pygame.K_RIGHTBRACKET:
            dados.comando.limite_falha = min(
                50.0,
                dados.comando.limite_falha + 1.0
            )

            dados.status = (
                f"Limite de falha alterado para "
                f"{dados.comando.limite_falha:.1f} cm."
            )

    publicar_comando(cliente)


def processar_tecla_solta(
    tecla: int,
    cliente: mqtt.Client
) -> None:
    if tecla not in (pygame.K_RIGHT, pygame.K_LEFT):
        return

    with dados.mutex:
        if dados.comando.c_man:
            dados.comando.c_direita = False
            dados.comando.c_esquerda = False
            dados.comando.c_para = False
            dados.status = "Modo manual sem comando de aceleracao."

    publicar_comando(cliente)


# ============================================================
# Programa principal
# ============================================================

def main() -> None:
    pygame.init()

    tela = pygame.display.set_mode(
        (LARGURA_JANELA, ALTURA_JANELA)
    )

    pygame.display.set_caption(
        "RT Inspection System - Operacao Remota"
    )

    relogio = pygame.time.Clock()

    fonte_titulo = pygame.font.SysFont("arial", 27, bold=True)
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
        with dados.mutex:
            dados.status = (
                f"Erro MQTT: {erro}. Verifique o broker Mosquitto."
            )

    executando = True

    while executando:
        relogio.tick(60)

        for evento in pygame.event.get():
            if evento.type == pygame.QUIT:
                executando = False

            elif evento.type == pygame.KEYDOWN:
                if evento.key == pygame.K_ESCAPE:
                    executando = False
                else:
                    processar_tecla_pressionada(
                        evento.key,
                        cliente
                    )

            elif evento.type == pygame.KEYUP:
                processar_tecla_solta(
                    evento.key,
                    cliente
                )

        desenhar_interface(
            tela,
            fonte_titulo,
            fonte,
            fonte_pequena
        )

        pygame.display.flip()

    cliente.loop_stop()
    cliente.disconnect()

    pygame.quit()


if __name__ == "__main__":
    main()