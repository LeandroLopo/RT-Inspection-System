// Implemente aqui a tarefa ReconstrucaoSuperficie.
//
// Responsabilidade:
// - consumir SensorData do SensorBuffer usando condition_variable;
// - aplicar media movel no valor do LIDAR;
// - ler a posicao X atual do estado compartilhado;
// - gerar SurfacePoint com timestamp, x, y e confidence;
// - inserir SurfacePoint no SurfaceBuffer;
// - futuramente detectar variacoes bruscas e sinalizar CameraEvent.
//
// Primeiro objetivo:
// fazer o fluxo SensorBuffer -> ReconstrucaoSuperficie -> SurfaceBuffer funcionar.
#include "buffers.hpp"
#include "log.hpp"
#include "shared_state.hpp"
#include <cmath>
#include <iostream>

void ReconstrucaoSuperficie(SensorBuffer &buffer, SurfaceBuffer &surfaceBuffer, SharedRobotState &robotState, SharedActuatorData &sharedActuatorData, CameraEvent &cameraEvent) {
    const int tamanhoJanela = 3;
    const double limiteFalha = 10.0;
    std::queue<int> ultimasLeituras;
    int somaLeituras = 0;
    bool primeiraMedia = true;
    double mediaAnterior = 0.0;
    bool falhaJaDetectada = false;

      while (true)
    {
        SensorData leitura;

        {
            std::unique_lock<std::mutex> trava(buffer.mutex_sensor);

            buffer.dado_disponivel_var.wait(trava, [&buffer] {
                return !buffer.fila_sensor.empty() || buffer.finalizado;
            });

            if (buffer.fila_sensor.empty() && buffer.finalizado)
            {
                break;
            }

            leitura = buffer.fila_sensor.front();
            buffer.fila_sensor.pop();
        }

        ultimasLeituras.push(leitura.i_lidar);
        somaLeituras += leitura.i_lidar;

        if (ultimasLeituras.size() > tamanhoJanela)
        {
            somaLeituras -= ultimasLeituras.front();
            ultimasLeituras.pop();
        }

        double media_movel =
            static_cast<double>(somaLeituras) / ultimasLeituras.size();

        SurfacePoint ponto;

        ponto.timestamp = leitura.timestamp;
        {
            std::lock_guard<std::mutex> trava(robotState.mutex_estado);
            ponto.x = robotState.estado.posicao_x;
        }
        ponto.y = media_movel;
        ponto.confianca = 1.0;

        if (!primeiraMedia)
        {
            const double variacao = std::abs(media_movel - mediaAnterior);

            if (variacao > limiteFalha && !falhaJaDetectada)
            {
                falhaJaDetectada = true;

                {
                    std::lock_guard<std::mutex> trava(robotState.mutex_estado);
                    robotState.estado.e_inspecao = true;
                }

                {
                    std::lock_guard<std::mutex> trava(sharedActuatorData.mutex_atuadores);
                    sharedActuatorData.atuadores.o_liga_camera = true;
                }

                {
                    std::lock_guard<std::mutex> trava(cameraEvent.mutex_camera);
                    cameraEvent.falha_detectada = true;
                    cameraEvent.timestamp = ponto.timestamp;
                    cameraEvent.x = ponto.x;
                    cameraEvent.y = ponto.y;
                }

                cameraEvent.camera_event_var.notify_one();

                {
                    std::lock_guard<std::mutex> trava(coutMutex);
                    std::cout << "Falha detectada: x=" << ponto.x
                              << " y=" << ponto.y
                              << " variacao=" << variacao
                              << " limite=" << limiteFalha
                              << std::endl;
                }
            }
        }

        mediaAnterior = media_movel;
        primeiraMedia = false;

        {
            std::lock_guard<std::mutex> trava (surfaceBuffer.mutex_superficie);
            surfaceBuffer.fila_superficie.push (ponto); //inserção do dado de sensor na fila
        }

        surfaceBuffer.surface_point_var.notify_one(); //evita espera ocupada e acorda a thread só quando há dado novo.

        {
            std::lock_guard<std::mutex> trava(coutMutex);
            std::cout << "Reconstrucao: encoder=" << leitura.i_encoder
                      << " lidar=" << leitura.i_lidar
                      << " media_movel=" << media_movel
                      << " timestamp=" << leitura.timestamp
                      << std::endl;
        }

    }

    {
        std::lock_guard<std::mutex> trava(surfaceBuffer.mutex_superficie);
        surfaceBuffer.finalizado = true;
    }
    surfaceBuffer.surface_point_var.notify_one();

    {
        std::lock_guard<std::mutex> trava(cameraEvent.mutex_camera);
        cameraEvent.finalizado = true;
    }
    cameraEvent.camera_event_var.notify_one();
}
