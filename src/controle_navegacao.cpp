// Implemente aqui a tarefa ControleNavegacao.
//
// Responsabilidade:
// - ler o setpoint de velocidade;
// - ler a velocidade atual do robo;
// - calcular erro = setpoint - velocidadeAtual;
// - gerar o comando de aceleracao;
// - limitar a aceleracao entre -100 e 100.
//
// Comece com controle proporcional simples. Depois, se quiser, evolua para PID.

#include "log.hpp"
#include "shared_state.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>

void ControleNavegacao(SharedCommand &sharedCommand, SharedRobotState &robotState, SharedActuatorData &sharedActuatorData, SharedSystemControl &systemControl)
{
    const double kp = 25.0;
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

        RobotCommand comando;
        double velocidadeAtual = 0.0;
        bool emInspecao = false;

        {
            std::lock_guard<std::mutex> trava(sharedCommand.mutex_comando);
            comando = sharedCommand.comando;
        }

        {
            std::lock_guard<std::mutex> trava(robotState.mutex_estado);
            velocidadeAtual = robotState.estado.velocidade;
            emInspecao = robotState.estado.e_inspecao;
        }

        const double setpointVelocidade = emInspecao ? std::min(comando.j_sp_velocidade, 1) : comando.j_sp_velocidade;
        const double erro = setpointVelocidade - velocidadeAtual;
        const int aceleracao = static_cast<int>(std::clamp(kp * erro, -100.0, 100.0));

        {
            std::lock_guard<std::mutex> trava(sharedActuatorData.mutex_atuadores);
            sharedActuatorData.atuadores.o_aceleracao = aceleracao;
            sharedActuatorData.atuadores.o_liga_camera = emInspecao;
        }

        {
            std::lock_guard<std::mutex> trava(robotState.mutex_estado);
            robotState.estado.velocidade += aceleracao * 0.001;
            velocidadeAtual = robotState.estado.velocidade;
        }

        if (ciclo % 10 == 0)
        {
            std::lock_guard<std::mutex> trava(coutMutex);
            std::cout << "ControleNavegacao: erro=" << erro
                      << " aceleracao=" << aceleracao
                      << " velocidade=" << velocidadeAtual
                      << " inspecao=" << emInspecao
                      << std::endl;
        }

        ciclo++;
        std::this_thread::sleep_until(proximaExecucao);
    }
}
