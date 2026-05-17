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
#include <iostream>

void ReconstrucaoSuperficie(SensorBuffer &buffer, SurfaceBuffer &surfaceBuffer) {
    const int tamanhoJanela = 3;
    std::queue<int> ultimasLeituras;
    int somaLeituras = 0;

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
        ponto.x = 0.0;
        ponto.y = media_movel;
        ponto.confianca = 1.0;

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
}
