#include "tasks.hpp"

#include <functional>
#include <mutex>
#include <thread>

int main()
{
    SensorBuffer sensorBuffer;
    EncoderBuffer encoderBuffer;
    PositionBuffer positionBuffer;
    SurfaceBuffer surfaceBuffer;
    CameraEvent cameraEvent;
    SharedRobotState robotState;
    SharedCommand sharedCommand;
    SharedActuatorData sharedActuatorData;
    SharedSystemControl systemControl;

    {
        std::lock_guard<std::mutex> trava(sharedCommand.mutex_comando);
        sharedCommand.comando.c_automatico = true;
        sharedCommand.comando.j_sp_velocidade = 2;
    }

    {
        std::lock_guard<std::mutex> trava(robotState.mutex_estado);
        robotState.estado.e_automatico = true;
    }

    std::thread comando(ComandoNavegacao, std::ref(sharedCommand), std::ref(robotState), std::ref(systemControl));
    std::thread controle(ControleNavegacao, std::ref(sharedCommand), std::ref(robotState), std::ref(sharedActuatorData),
                         std::ref(systemControl));
    std::thread simulacao(SimulacaoSensores, std::ref(sensorBuffer), std::ref(encoderBuffer));
    std::thread distancia(DistanciaPercorrida, std::ref(encoderBuffer), std::ref(positionBuffer), std::ref(robotState));
    std::thread reconstrucao(ReconstrucaoSuperficie, std::ref(sensorBuffer), std::ref(positionBuffer), std::ref(surfaceBuffer),
                             std::ref(robotState), std::ref(sharedActuatorData), std::ref(cameraEvent));
    std::thread coletor(ColetorDados, std::ref(surfaceBuffer));
    std::thread camera(InspecaoCamera, std::ref(cameraEvent), std::ref(robotState), std::ref(sharedActuatorData));

    simulacao.join();
    distancia.join();
    reconstrucao.join();
    coletor.join();
    camera.join();

    {
        std::lock_guard<std::mutex> trava(systemControl.mutex_sistema);
        systemControl.sistema_rodando = false;
    }

    comando.join();
    controle.join();

    return 0;
}
