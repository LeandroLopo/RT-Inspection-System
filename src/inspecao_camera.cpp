#include "buffers.hpp"
#include "log.hpp"
#include "shared_state.hpp"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <thread>

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

        const auto inicioProcessamento = std::chrono::steady_clock::now();
        const auto fimProcessamento =
            inicioProcessamento + std::chrono::seconds(2);

        double acumulador = 0.0;
        std::uint64_t iteracoes = 0;

        while (std::chrono::steady_clock::now() < fimProcessamento)
        {
            for (int indice = 0; indice < 10000; indice++)
            {
                const double valor =
                    static_cast<double>(indice) * 0.001 + x + y;

                acumulador += std::sin(valor) * std::cos(valor * 0.5);
                iteracoes++;
            }
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
                      << acumulador
                      << " iteracoes=" << iteracoes
                      << std::endl;
        }
    }
}
