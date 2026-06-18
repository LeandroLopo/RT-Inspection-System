import shutil
import signal
import socket
import subprocess
import sys
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parent
BROKER_HOST = "localhost"
BROKER_PORT = 1883


def broker_disponivel() -> bool:
    try:
        with socket.create_connection((BROKER_HOST, BROKER_PORT), timeout=1.0):
            return True
    except OSError:
        return False


def iniciar_broker_se_necessario() -> subprocess.Popen | None:
    if broker_disponivel():
        print("Broker MQTT ja esta ativo em localhost:1883.")
        return None

    mosquitto = shutil.which("mosquitto")

    if mosquitto is None:
        raise RuntimeError(
            "Broker MQTT indisponivel. Instale/inicie o Mosquitto "
            "ou rode: mosquitto -p 1883"
        )

    print("Iniciando broker MQTT local em localhost:1883...")

    processo = subprocess.Popen(
        [mosquitto, "-p", str(BROKER_PORT)],
        cwd=ROOT,
        start_new_session=True
    )

    for _ in range(30):
        if broker_disponivel():
            return processo

        if processo.poll() is not None:
            break

        time.sleep(0.1)

    encerrar_processo(processo)
    raise RuntimeError("Nao foi possivel iniciar o broker MQTT local.")


def iniciar_processo(comando: list[str]) -> subprocess.Popen:
    print("Executando:", " ".join(comando))

    return subprocess.Popen(
        comando,
        cwd=ROOT,
        start_new_session=True
    )


def encerrar_processo(processo: subprocess.Popen | None) -> None:
    if processo is None or processo.poll() is not None:
        return

    try:
        os_killpg(processo.pid, signal.SIGTERM)
        processo.wait(timeout=3.0)
    except subprocess.TimeoutExpired:
        os_killpg(processo.pid, signal.SIGKILL)
        processo.wait()


def os_killpg(pid: int, sig: signal.Signals) -> None:
    if hasattr(signal, "SIGTERM"):
        import os

        try:
            os.killpg(pid, sig)
        except ProcessLookupError:
            pass


def solicitar_encerramento(signum, frame) -> None:
    raise KeyboardInterrupt


def main() -> int:
    processos: list[subprocess.Popen] = []
    broker = None

    signal.signal(signal.SIGTERM, solicitar_encerramento)

    try:
        subprocess.run(["make"], cwd=ROOT, check=True)

        broker = iniciar_broker_se_necessario()

        processos.append(iniciar_processo(["build/rt_inspection"]))
        time.sleep(1.0)

        processos.append(
            iniciar_processo([sys.executable, "simulation/simulation_gui.py"])
        )
        processos.append(
            iniciar_processo([sys.executable, "remote_operation/remote_gui.py"])
        )

        print("Sistema iniciado. Pressione Ctrl+C para encerrar.")

        while True:
            for processo in processos:
                if processo.poll() is not None:
                    return processo.returncode or 0

            time.sleep(0.5)

    except KeyboardInterrupt:
        print("\nEncerrando processos...")
        return 0

    finally:
        for processo in reversed(processos):
            encerrar_processo(processo)

        encerrar_processo(broker)


if __name__ == "__main__":
    raise SystemExit(main())
