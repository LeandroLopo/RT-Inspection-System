import json
import math
import random
import time
from dataclasses import dataclass
from threading import Lock

import paho.mqtt.client as mqtt


BROKER = "localhost"
PORTA = 1883

TOPICO_SENSORES = "atr/sim/sensors"
TOPICO_ATUADORES = "atr/core/actuators"

PERIODO_SIMULACAO = 0.05       # 50 ms
COMPRIMENTO_TUNEL = 35.0       # metros
VELOCIDADE_MAXIMA = 3.0        # m/s

ACELERACAO_MAXIMA = 1.8        # m/s²
ATRITO = 0.35                   # resistencia proporcional a velocidade

DISTANCIA_NORMAL_TETO = 100.0  # centimetros
DESVIO_PADRAO_RUIDO = 0.8      # centimetros


@dataclass
class Atuadores:
    aceleracao: int = 0
    camera_ligada: bool = False


class SimulacaoTunel:
    def __init__(self) -> None:
        self.x = 0.0
        self.velocidade = 0.0

        self.estado_encoder = False
        self.ultimo_metro_encoder = 0

        self.atuadores = Atuadores()
        self.mutex_atuadores = Lock()

        self.camera_anterior = False
        self.inicio = time.monotonic()

    def distancia_teto(self, x: float) -> float:
        """
        Retorna a distancia entre o sensor LIDAR e o teto em centimetros.

        Trechos normais: aproximadamente 100 cm.
        Entre 12 m e 15 m: cavidade severa, aumentando a distancia.
        Entre 24 m e 26 m: pequena saliencia, sem necessariamente
        representar falha severa.
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
            0.0,
            min(self.velocidade, VELOCIDADE_MAXIMA)
        )

        self.x += self.velocidade * dt

        self.x = min(self.x, COMPRIMENTO_TUNEL)

        self.atualizar_encoder()

    def gerar_sensor(self) -> dict:
        distancia_real = self.distancia_teto(self.x)
        ruido = random.gauss(0.0, DESVIO_PADRAO_RUIDO)

        leitura_lidar = int(round(distancia_real + ruido))

        timestamp = round(time.monotonic() - self.inicio, 3)

        return {
            "i_encoder": self.estado_encoder,
            "i_lidar": leitura_lidar,
            "velocidade": round(self.velocidade, 3),
            "posicao_real": round(self.x, 3),
            "timestamp": timestamp
        }

    def receber_atuadores(self, mensagem: dict) -> None:
        if mensagem.get("finalizado", False):
            return

        aceleracao = int(mensagem.get("o_aceleracao", 0))
        camera_ligada = bool(mensagem.get("o_liga_camera", False))

        with self.mutex_atuadores:
            self.atuadores.aceleracao = aceleracao
            self.atuadores.camera_ligada = camera_ligada

        if camera_ligada != self.camera_anterior:
            estado = "LIGADA" if camera_ligada else "DESLIGADA"
            print(f"[CAMERA] Camera {estado} em x={self.x:.2f} m")
            self.camera_anterior = camera_ligada


simulacao = SimulacaoTunel()


def ao_conectar(cliente, dados_usuario, flags, codigo_motivo, propriedades):
    print(f"Simulacao conectada ao broker MQTT: {codigo_motivo}")

    cliente.subscribe(TOPICO_ATUADORES)

    print(f"Assinando atuadores em: {TOPICO_ATUADORES}")
    print(f"Publicando sensores em: {TOPICO_SENSORES}")


def ao_receber_mensagem(cliente, dados_usuario, mensagem):
    try:
        payload = mensagem.payload.decode("utf-8")
        dados = json.loads(payload)

        simulacao.receber_atuadores(dados)

    except (UnicodeDecodeError, json.JSONDecodeError, ValueError) as erro:
        print(f"Erro ao interpretar atuadores: {erro}")


def main() -> None:
    cliente = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

    cliente.on_connect = ao_conectar
    cliente.on_message = ao_receber_mensagem

    cliente.connect(BROKER, PORTA, 60)
    cliente.loop_start()

    print("Simulacao fisica iniciada.")
    print(f"Comprimento do tunel: {COMPRIMENTO_TUNEL} m")
    print("Falha principal posicionada entre 12 m e 15 m.")
    print()

    proxima_execucao = time.monotonic()
    contador = 0

    try:
        while simulacao.x < COMPRIMENTO_TUNEL:
            proxima_execucao += PERIODO_SIMULACAO

            simulacao.atualizar_movimento(PERIODO_SIMULACAO)

            sensor = simulacao.gerar_sensor()

            cliente.publish(
                TOPICO_SENSORES,
                json.dumps(sensor)
            )

            if contador % 10 == 0:
                with simulacao.mutex_atuadores:
                    aceleracao = simulacao.atuadores.aceleracao
                    camera = simulacao.atuadores.camera_ligada

                print(
                    f"x={sensor['posicao_real']:6.2f} m | "
                    f"v={sensor['velocidade']:4.2f} m/s | "
                    f"lidar={sensor['i_lidar']:3d} cm | "
                    f"encoder={int(sensor['i_encoder'])} | "
                    f"motor={aceleracao:4d} | "
                    f"camera={camera}"
                )

            contador += 1

            espera = proxima_execucao - time.monotonic()

            if espera > 0:
                time.sleep(espera)

    except KeyboardInterrupt:
        print("\nSimulacao interrompida pelo usuario.")

    finally:
        cliente.publish(
            TOPICO_SENSORES,
            json.dumps({"finalizado": True})
        )

        time.sleep(0.3)

        cliente.loop_stop()
        cliente.disconnect()

        print()
        print("Simulacao fisica finalizada.")
        print(f"Posicao final: {simulacao.x:.2f} m")
        print(f"Velocidade final: {simulacao.velocidade:.2f} m/s")


if __name__ == "__main__":
    main()