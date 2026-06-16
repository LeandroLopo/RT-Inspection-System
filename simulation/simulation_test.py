import json
import time

import paho.mqtt.client as mqtt


BROKER = "localhost"
PORTA = 1883
TOPICO_SENSORES = "atr/sim/sensors"


def main() -> None:
    cliente = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    cliente.connect(BROKER, PORTA, 60)
    cliente.loop_start()

    estado_encoder = False

    print("Simulacao de teste iniciada.")

    for indice in range(10):
        estado_encoder = not estado_encoder

        lidar = 140 if indice == 5 else 100 + indice

        mensagem = {
            "i_encoder": estado_encoder,
            "i_lidar": lidar,
            "timestamp": round(indice * 0.5, 2)
        }

        cliente.publish(
            TOPICO_SENSORES,
            json.dumps(mensagem)
        )

        print(f"Sensor publicado: {mensagem}")

        time.sleep(0.5)

    mensagem_final = {
        "finalizado": True
    }

    cliente.publish(
        TOPICO_SENSORES,
        json.dumps(mensagem_final)
    )

    print("Mensagem de finalizacao publicada.")

    time.sleep(0.5)

    cliente.loop_stop()
    cliente.disconnect()


if __name__ == "__main__":
    main()