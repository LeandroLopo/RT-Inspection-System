#include "tasks.hpp"

#include <mosquitto.h>

#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

int main()
{
    const int resultadoMqtt = mosquitto_lib_init();

    if (resultadoMqtt != MOSQ_ERR_SUCCESS) {
        std::cerr << "Erro ao inicializar biblioteca MQTT: "
                  << mosquitto_strerror(resultadoMqtt)
                  << std::endl;

        return 1;
    }

    SensorBuffer sensorBuffer;
    EncoderBuffer encoderBuffer;
    PositionBuffer positionBuffer;

    SurfaceBuffer surfaceBuffer;
    SurfaceBuffer remoteSurfaceBuffer;

    CameraEvent cameraEvent;

    SharedRobotState robotState;
    SharedCommand sharedCommand;
    SharedActuatorData sharedActuatorData;
    SharedSystemParameters systemParameters;
    SharedSystemControl systemControl;

    {
        std::lock_guard<std::mutex> trava(sharedCommand.mutex_comando);

        sharedCommand.comando.c_automatico = true;
        sharedCommand.comando.c_man = false;
        sharedCommand.comando.c_direita = false;
        sharedCommand.comando.c_esquerda = false;
        sharedCommand.comando.c_para = false;
        sharedCommand.comando.j_sp_velocidade = 2;
    }

    {
        std::lock_guard<std::mutex> trava(robotState.mutex_estado);

        robotState.estado.e_automatico = true;
        robotState.estado.e_inspecao = false;
        robotState.estado.velocidade = 0.0;
        robotState.estado.posicao_x = 0.0;
    }

    {
        std::lock_guard<std::mutex> trava(
            systemParameters.mutex_parametros
        );

        systemParameters.limite_falha = 10.0;
    }

    std::thread comando(
        ComandoNavegacao,
        std::ref(sharedCommand),
        std::ref(robotState),
        std::ref(systemControl)
    );

    std::thread controle(
        ControleNavegacao,
        std::ref(sharedCommand),
        std::ref(robotState),
        std::ref(sharedActuatorData),
        std::ref(systemControl)
    );

    std::thread sensoresMqtt(
        RecebeSensoresMqtt,
        std::ref(sensorBuffer),
        std::ref(encoderBuffer),
        std::ref(robotState)
    );

    std::thread comandosMqtt(
        RecebeComandosMqtt,
        std::ref(sharedCommand),
        std::ref(systemParameters),
        std::ref(systemControl)
    );

    std::thread atuadoresMqtt(
        PublicaAtuadoresMqtt,
        std::ref(sharedActuatorData),
        std::ref(systemControl)
    );

    std::thread estadoMqtt(
        PublicaEstadoMqtt,
        std::ref(robotState),
        std::ref(sharedActuatorData),
        std::ref(systemParameters),
        std::ref(systemControl)
    );

    std::thread distancia(
        DistanciaPercorrida,
        std::ref(encoderBuffer),
        std::ref(positionBuffer),
        std::ref(robotState)
    );

    std::thread reconstrucao(
        ReconstrucaoSuperficie,
        std::ref(sensorBuffer),
        std::ref(positionBuffer),
        std::ref(surfaceBuffer),
        std::ref(remoteSurfaceBuffer),
        std::ref(robotState),
        std::ref(sharedActuatorData),
        std::ref(systemParameters),
        std::ref(cameraEvent)
    );

    std::thread coletor(
        ColetorDados,
        std::ref(surfaceBuffer)
    );

    std::thread superficieMqtt(
        PublicaSuperficieMqtt,
        std::ref(remoteSurfaceBuffer)
    );

    std::thread camera(
        InspecaoCamera,
        std::ref(cameraEvent),
        std::ref(robotState),
        std::ref(sharedActuatorData)
    );

    sensoresMqtt.join();
    distancia.join();
    reconstrucao.join();
    coletor.join();
    superficieMqtt.join();
    camera.join();

    {
        std::lock_guard<std::mutex> trava(systemControl.mutex_sistema);
        systemControl.sistema_rodando = false;
    }

    comando.join();
    controle.join();

    comandosMqtt.join();
    atuadoresMqtt.join();
    estadoMqtt.join();

    mosquitto_lib_cleanup();

    return 0;
}