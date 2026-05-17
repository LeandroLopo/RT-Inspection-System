// Ponto de entrada da versao modular.
//
// Implemente aqui apenas a montagem do sistema:
// - criar buffers compartilhados;
// - criar estados compartilhados;
// - iniciar as threads das tarefas;
// - chamar join() em todas as threads;
// - imprimir inicio e fim da execucao.
//
// Evite colocar a logica das tarefas neste arquivo. A logica deve ficar nos outros .cpp.


#include <iostream>
#include "time_utils.hpp"
#include "tasks.hpp"
#include <thread>

int main()
{
    SensorBuffer sensorBuffer;
    EncoderBuffer encoderBuffer;
    SurfaceBuffer surfaceBuffer;
    CameraEvent cameraEvent;
    SharedRobotState robotState;
    SharedCommand sharedCommand;
    SharedActuatorData sharedActuatorData;
    SharedSystemControl systemControl;

    std::thread comando(ComandoNavegacao, std::ref(sharedCommand), std::ref(robotState), std::ref(systemControl));
    std::thread controle(ControleNavegacao, std::ref(sharedCommand), std::ref(robotState), std::ref(sharedActuatorData), std::ref(systemControl));
    std::thread simulacao(SimulacaoSensores, std::ref(sensorBuffer), std::ref(encoderBuffer));
    std::thread distancia(DistanciaPercorrida, std::ref(encoderBuffer), std::ref(robotState));
    std::thread reconstrucao(ReconstrucaoSuperficie, std::ref(sensorBuffer), std::ref(surfaceBuffer), std::ref(robotState), std::ref(sharedActuatorData), std::ref(cameraEvent));
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
