// Implemente aqui a tarefa DistanciaPercorrida.
//
// Responsabilidade:
// - consumir leituras do encoder;
// - comparar o estado atual com o estado anterior;
// - somar 1 metro quando o encoder trocar de estado;
// - atualizar a posicao X no estado compartilhado.
//
// Regra inicial:
// se encoderAtual != encoderAnterior, entao posicaoX += 1.0.

#include "buffers.hpp"
#include "log.hpp"
#include "shared_state.hpp"

#include <iostream>

void DistanciaPercorrida(EncoderBuffer &encoderBuffer, SharedRobotState &robotState)
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

        if (primeiraLeitura)
        {
            encoderAnterior = leitura.i_encoder;
            primeiraLeitura = false;
            continue;
        }

        if (leitura.i_encoder != encoderAnterior)
        {
            double posicaoAtualizada = 0.0;

            {
                std::lock_guard<std::mutex> trava(robotState.mutex_estado);
                robotState.estado.posicao_x += 1.0;
                posicaoAtualizada = robotState.estado.posicao_x;
            }

            encoderAnterior = leitura.i_encoder;

            {
                std::lock_guard<std::mutex> trava(coutMutex);
                std::cout << "DistanciaPercorrida: x=" << posicaoAtualizada
                          << " m timestamp=" << leitura.timestamp
                          << std::endl;
            }
        }
    }
}
