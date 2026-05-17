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
#include <iostream>

void ReconstrucaoSuperficie(SensorBuffer &buffer) {
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

        double mediaMovel =
            static_cast<double>(somaLeituras) / ultimasLeituras.size();

        std::cout << "Reconstrucao: encoder=" << leitura.i_encoder
                  << " lidar=" << leitura.i_lidar
                  << " media_movel=" << mediaMovel
                  << " timestamp=" << leitura.timestamp
                  << std::endl;
    }
}
