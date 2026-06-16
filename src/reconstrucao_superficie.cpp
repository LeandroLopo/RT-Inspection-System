#include "buffers.hpp"
#include "log.hpp"
#include "shared_state.hpp"

#include <cmath>
#include <iostream>
#include <queue>

void ReconstrucaoSuperficie(SensorBuffer &buffer,
                             PositionBuffer &positionBuffer,
                             SurfaceBuffer &surfaceBuffer,
                             SurfaceBuffer &remoteSurfaceBuffer,
                             SharedRobotState &robotState,
                             SharedActuatorData &sharedActuatorData,
                             SharedSystemParameters &systemParameters,
                             CameraEvent &cameraEvent)
{
    const std::size_t tamanhoJanela = 3;

    std::queue<int> ultimasLeituras;

    int somaLeituras = 0;

    bool primeiraLeitura = true;
    double leituraAnterior = 0.0;
    bool falhaJaDetectada = false;

    while (true) {
        SensorData leitura;
        PositionData posicao;

        {
            std::unique_lock<std::mutex> trava(buffer.mutex_sensor);

            buffer.dado_disponivel_var.wait(
                trava,
                [&buffer] {
                    return !buffer.fila_sensor.empty()
                        || buffer.finalizado;
                }
            );

            if (buffer.fila_sensor.empty() && buffer.finalizado) {
                break;
            }

            leitura = buffer.fila_sensor.front();
            buffer.fila_sensor.pop();
        }

        {
            std::unique_lock<std::mutex> trava(
                positionBuffer.mutex_posicao
            );

            positionBuffer.posicao_disponivel_var.wait(
                trava,
                [&positionBuffer] {
                    return !positionBuffer.fila_posicao.empty()
                        || positionBuffer.finalizado;
                }
            );

            if (
                positionBuffer.fila_posicao.empty()
                && positionBuffer.finalizado
            ) {
                break;
            }

            posicao = positionBuffer.fila_posicao.front();
            positionBuffer.fila_posicao.pop();
        }

        ultimasLeituras.push(leitura.i_lidar);
        somaLeituras += leitura.i_lidar;

        if (ultimasLeituras.size() > tamanhoJanela) {
            somaLeituras -= ultimasLeituras.front();
            ultimasLeituras.pop();
        }

        const double mediaMovel =
            static_cast<double>(somaLeituras)
            / static_cast<double>(ultimasLeituras.size());

        SurfacePoint ponto;

        ponto.timestamp = leitura.timestamp;
        ponto.x = posicao.x;
        ponto.y = mediaMovel; 
        std::cout << "SUPERFICIE x=" << ponto.x  << " y=" << ponto.y << std::endl;
        ponto.confianca = 1.0;

        double limiteFalha;

        {
            std::lock_guard<std::mutex> trava(
                systemParameters.mutex_parametros
            );

            limiteFalha = systemParameters.limite_falha;
        }
if (!primeiraLeitura)
{
    const double variacao =
        std::abs(leitura.i_lidar - leituraAnterior);

    if (variacao > limiteFalha)
    {
        if (!falhaJaDetectada)
        {
            falhaJaDetectada = true;

            {
                std::lock_guard<std::mutex> trava(
                    robotState.mutex_estado
                );

                robotState.estado.e_inspecao = true;
            }

            {
                std::lock_guard<std::mutex> trava(
                    sharedActuatorData.mutex_atuadores
                );

                sharedActuatorData.atuadores.o_liga_camera = true;
            }

            {
                std::lock_guard<std::mutex> trava(
                    cameraEvent.mutex_camera
                );

                cameraEvent.falha_detectada = true;
                cameraEvent.timestamp = ponto.timestamp;
                cameraEvent.x = ponto.x;
                cameraEvent.y = ponto.y;
            }

            cameraEvent.camera_event_var.notify_one();

            {
                std::lock_guard<std::mutex> trava(coutMutex);

                std::cout << "Falha detectada: x="
                          << ponto.x
                          << " y=" << ponto.y
                          << " variacao=" << variacao
                          << " limite=" << limiteFalha
                          << std::endl;
            }
        }
    }
    else
    {
        falhaJaDetectada = false;
    }
}
      leituraAnterior = leitura.i_lidar;
      primeiraLeitura = false;

        {
            std::lock_guard<std::mutex> trava(
                surfaceBuffer.mutex_superficie
            );

            surfaceBuffer.fila_superficie.push(ponto);
        }

        surfaceBuffer.surface_point_var.notify_one();

        {
            std::lock_guard<std::mutex> trava(
                remoteSurfaceBuffer.mutex_superficie
            );

            remoteSurfaceBuffer.fila_superficie.push(ponto);
        }

        remoteSurfaceBuffer.surface_point_var.notify_one();

        {
            std::lock_guard<std::mutex> trava(coutMutex);

            std::cout << "Reconstrucao: encoder="
                      << leitura.i_encoder
                      << " lidar=" << leitura.i_lidar
                      << " media_movel=" << mediaMovel
                      << " limite_falha=" << limiteFalha
                      << " timestamp=" << leitura.timestamp
                      << std::endl;
        }
    }

    {
        std::lock_guard<std::mutex> trava(surfaceBuffer.mutex_superficie);
        surfaceBuffer.finalizado = true;
    }

    surfaceBuffer.surface_point_var.notify_all();

    {
        std::lock_guard<std::mutex> trava(
            remoteSurfaceBuffer.mutex_superficie
        );

        remoteSurfaceBuffer.finalizado = true;
    }

    remoteSurfaceBuffer.surface_point_var.notify_all();

    {
        std::lock_guard<std::mutex> trava(cameraEvent.mutex_camera);
        cameraEvent.finalizado = true;
    }

    cameraEvent.camera_event_var.notify_all();
}