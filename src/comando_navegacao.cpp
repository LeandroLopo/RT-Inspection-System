#include "log.hpp"
#include "shared_state.hpp"

#include <chrono>
#include <iostream>
#include <thread>

void ComandoNavegacao(SharedCommand &sharedCommand, SharedRobotState &robotState, SharedSystemControl &systemControl)
{
    auto proximaExecucao = std::chrono::steady_clock::now();
    int ciclo = 0;

    while (true)
    {
        proximaExecucao += std::chrono::milliseconds(80);

        {
            std::lock_guard<std::mutex> trava(systemControl.mutex_sistema);
            if (!systemControl.sistema_rodando)
            {
                break;
            }
        }

        {
            std::lock_guard<std::mutex> trava(sharedCommand.mutex_comando);
            sharedCommand.comando.c_automatico = true;
            sharedCommand.comando.c_man = false;
            sharedCommand.comando.c_direita = false;
            sharedCommand.comando.c_esquerda = false;
            sharedCommand.comando.c_para = false;
            sharedCommand.comando.j_sp_velocidade = 2;
        }

        {
            std::lock_guard<std::mutex> trava(robotState.mutex_estado);
            robotState.estado.e_automatico = true;
        }

        if (ciclo % 10 == 0)
        {
            std::lock_guard<std::mutex> trava(coutMutex);
            std::cout << "ComandoNavegacao: automatico=1 sp_velocidade=2"
                      << std::endl;
        }

        ciclo++;
        std::this_thread::sleep_until(proximaExecucao);
    }
}
