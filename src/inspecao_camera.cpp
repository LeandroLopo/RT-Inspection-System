// Implemente aqui a tarefa InspecaoCamera.
//
// Responsabilidade:
// - ficar bloqueada aguardando CameraEvent;
// - acordar quando a reconstrucao detectar uma falha;
// - simular camera ligada;
// - executar processamento pesado usando CPU, nao apenas sleep;
// - voltar a aguardar outro evento.
//
// Exemplo de processamento pesado:
// executar um loop grande usando funcoes matematicas como std::sin.

#include "buffers.hpp"
#include "log.hpp"
#include "shared_state.hpp"

#include <cmath>
#include <iostream>

void InspecaoCamera(CameraEvent &cameraEvent, SharedRobotState &robotState, SharedActuatorData &sharedActuatorData)
{
    while (true)
    {
        double timestamp = 0.0;
        double x = 0.0;
        double y = 0.0;

        {
            std::unique_lock<std::mutex> trava(cameraEvent.mutex_camera);

            cameraEvent.camera_event_var.wait(trava, [&cameraEvent] {
                return cameraEvent.falha_detectada || cameraEvent.finalizado;
            });

            if (!cameraEvent.falha_detectada && cameraEvent.finalizado)
            {
                break;
            }

            timestamp = cameraEvent.timestamp;
            x = cameraEvent.x;
            y = cameraEvent.y;
            cameraEvent.falha_detectada = false;
        }

        {
            std::lock_guard<std::mutex> trava(coutMutex);
            std::cout << "InspecaoCamera: iniciando inspecao em x=" << x
                      << " y=" << y
                      << " timestamp=" << timestamp
                      << std::endl;
        }

        volatile double resultado = 0.0;
        for (int i = 0; i < 8000000; i++)
        {
            resultado += std::sin(i * 0.001) * std::cos(i * 0.0005);
        }

        {
            std::lock_guard<std::mutex> trava(sharedActuatorData.mutex_atuadores);
            sharedActuatorData.atuadores.o_liga_camera = false;
        }

        {
            std::lock_guard<std::mutex> trava(robotState.mutex_estado);
            robotState.estado.e_inspecao = false;
        }

        {
            std::lock_guard<std::mutex> trava(coutMutex);
            std::cout << "InspecaoCamera: processamento finalizado resultado="
                      << resultado
                      << std::endl;
        }
    }
}
