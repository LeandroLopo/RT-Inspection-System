#include "log.hpp"
#include "shared_state.hpp"

#include <chrono>
#include <iostream>
#include <thread>

void ComandoNavegacao(SharedCommand &sharedCommand,
                       SharedRobotState &robotState,
                       SharedSystemControl &systemControl)
{
    auto proximaExecucao = std::chrono::steady_clock::now();

    int ciclo = 0;

    while (true) {
        proximaExecucao += std::chrono::milliseconds(80);

        {
            std::lock_guard<std::mutex> trava(systemControl.mutex_sistema);

            if (!systemControl.sistema_rodando) {
                break;
            }
        }

        RobotCommand comando;

        {
            std::lock_guard<std::mutex> trava(sharedCommand.mutex_comando);
            comando = sharedCommand.comando;
        }

        const bool modoAutomatico =
            comando.c_automatico && !comando.c_man;

        {
            std::lock_guard<std::mutex> trava(robotState.mutex_estado);
            robotState.estado.e_automatico = modoAutomatico;
        }

        if (ciclo % 10 == 0) {
            std::lock_guard<std::mutex> trava(coutMutex);

            std::cout << "ComandoNavegacao: automatico="
                      << modoAutomatico
                      << " direita=" << comando.c_direita
                      << " esquerda=" << comando.c_esquerda
                      << " parar=" << comando.c_para
                      << " sp_velocidade="
                      << comando.j_sp_velocidade
                      << std::endl;
        }

        ciclo++;

        std::this_thread::sleep_until(proximaExecucao);
    }
}