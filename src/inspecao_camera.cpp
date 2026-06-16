#include "buffers.hpp"
#include "log.hpp"
#include "shared_state.hpp"

#include <cmath>
#include <iostream>
#include <thread>
#include <chrono>

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

        std::this_thread::sleep_for(std::chrono::seconds(2));
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
                      << std::endl;
        }
    }
}
