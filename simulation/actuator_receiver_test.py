
import json

import paho.mqtt.client as mqtt


BROKER = "localhost"
PORTA = 1883
TOPICO_ATUADORES = "atr/core/actuators"


def ao_conectar(cliente, dados_usuario, flags, codigo_motivo, propriedades):
    print(f"Conectado ao broker MQTT. Resultado: {codigo_motivo}")
    cliente.subscribe(TOPICO_ATUADORES)
    print(f"Aguardando atuadores no topico: {TOPICO_ATUADORES}")


def ao_receber_mensagem(cliente, dados_usuario, mensagem):
    payload = mensagem.payload.decode("utf-8")
    dados = json.loads(payload)

    if dados.get("finalizado", False):
        print("Finalizacao dos atuadores recebida.")
        cliente.disconnect()
        return

    aceleracao = dados["o_aceleracao"]
    camera_ligada = dados["o_liga_camera"]

    print(
        f"Atuadores recebidos: "
        f"aceleracao={aceleracao} "
        f"camera={camera_ligada}"
    )


def main() -> None:
    cliente = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

    cliente.on_connect = ao_conectar
    cliente.on_message = ao_receber_mensagem

    cliente.connect(BROKER, PORTA, 60)

    cliente.loop_forever()


if __name__ == "__main__":
    main()