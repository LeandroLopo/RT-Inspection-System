#include "buffers.hpp"
#include "log.hpp"
#include "shared_state.hpp"

#include <iostream>

void DistanciaPercorrida(EncoderBuffer &encoderBuffer, PositionBuffer &positionBuffer, SharedRobotState &robotState)
{
    bool primeiraLeitura = true;
    bool encoderAnterior = false;

    while (true)
    {
        EncoderData leitura;

        {
            std::unique_lock<std::mutex> trava(encoderBuffer.mutex_encoder);

            encoderBuffer.encoder_disponivel_var.wait(trava, [&encoderBuffer] {
                return !encoderBuffer.fila_encoder.empty() || encoderBuffer.finalizado;
            });

            if (encoderBuffer.fila_encoder.empty() && encoderBuffer.finalizado)
            {
                break;
            }

            leitura = encoderBuffer.fila_encoder.front();
            encoderBuffer.fila_encoder.pop();
        }

        bool houveMudanca = false;

        if (primeiraLeitura)
        {
            encoderAnterior = leitura.i_encoder;
            primeiraLeitura = false;
        }
        else if (leitura.i_encoder != encoderAnterior)
        {
            encoderAnterior = leitura.i_encoder;
            houveMudanca = true;
        }

        PositionData posicao;
        posicao.timestamp = leitura.timestamp;

        {
            std::lock_guard<std::mutex> trava(robotState.mutex_estado);
            if (houveMudanca)
            {
                robotState.estado.posicao_x += 1.0;
            }
            posicao.x = robotState.estado.posicao_x;
        }

        {
            std::lock_guard<std::mutex> trava(positionBuffer.mutex_posicao);
            positionBuffer.fila_posicao.push(posicao);
        }
        positionBuffer.posicao_disponivel_var.notify_one();

        if (houveMudanca)
        {
            {
                std::lock_guard<std::mutex> trava(coutMutex);
                std::cout << "DistanciaPercorrida: x=" << posicao.x
                          << " m timestamp=" << leitura.timestamp
                          << std::endl;
            }
        }
    }

    {
        std::lock_guard<std::mutex> trava(positionBuffer.mutex_posicao);
        positionBuffer.finalizado = true;
    }
    positionBuffer.posicao_disponivel_var.notify_one();
}
