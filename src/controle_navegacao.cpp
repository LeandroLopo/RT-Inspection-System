#include "log.hpp"
#include "shared_state.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>

void ControleNavegacao(SharedCommand &sharedCommand,
                        SharedRobotState &robotState,
                        SharedActuatorData &sharedActuatorData,
                        SharedSystemControl &systemControl)
{
    const double kp = 25.0;
    const double ki = 1.5;
    const double kd = 4.0;

    const double periodoSegundos = 0.08;
    const double limiteIntegral = 20.0;

    double erroAnterior = 0.0;
    double integralErro = 0.0;

    bool automaticoAnterior = true;

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
        double velocidadeAtual;
        bool emInspecao;

        {
            std::lock_guard<std::mutex> trava(sharedCommand.mutex_comando);
            comando = sharedCommand.comando;
        }

        {
            std::lock_guard<std::mutex> trava(robotState.mutex_estado);
            velocidadeAtual = robotState.estado.velocidade;
            emInspecao = robotState.estado.e_inspecao;
        }

        const bool modoAutomatico =
            comando.c_automatico && !comando.c_man;

        if (modoAutomatico != automaticoAnterior) {
            integralErro = 0.0;
            erroAnterior = 0.0;
            automaticoAnterior = modoAutomatico;
        }

        int aceleracao = 0;

        if (modoAutomatico) {
            const double setpointVelocidade =
                emInspecao
                    ? std::min(
                        static_cast<double>(comando.j_sp_velocidade),
                        0.0
                    )
                    : static_cast<double>(comando.j_sp_velocidade);

            const double erro =
                setpointVelocidade - velocidadeAtual;

            integralErro = std::clamp(
                integralErro + erro * periodoSegundos,
                -limiteIntegral,
                limiteIntegral
            );

            const double derivadaErro =
                (erro - erroAnterior) / periodoSegundos;

            erroAnterior = erro;

            const double saidaPid =
                kp * erro
                + ki * integralErro
                + kd * derivadaErro;

            aceleracao = static_cast<int>(
                std::clamp(saidaPid, -100.0, 100.0)
            );
        }
        else {
            integralErro = 0.0;
            erroAnterior = 0.0;

            if (comando.c_para) {
                  aceleracao = 0;
                {
                    std::lock_guard<std::mutex> trava(sharedCommand.mutex_comando);
                    sharedCommand.comando.j_sp_velocidade = 0;
                }
            }
            else if (comando.c_direita) {
                aceleracao = 70;
            }
            else if (comando.c_esquerda) {
                aceleracao = -70;
            }
            else {
                aceleracao = 0;
            }
        }

        {
            std::lock_guard<std::mutex> trava(
                sharedActuatorData.mutex_atuadores
            );

            sharedActuatorData.atuadores.o_aceleracao = aceleracao;
            sharedActuatorData.atuadores.o_liga_camera = emInspecao;
        }

        if (ciclo % 10 == 0) {
            std::lock_guard<std::mutex> trava(coutMutex);

            std::cout << "ControleNavegacao: modo="
                      << (modoAutomatico ? "automatico" : "manual")
                      << " velocidade=" << velocidadeAtual
                      << " aceleracao=" << aceleracao
                      << " inspecao=" << emInspecao
                      << std::endl;
        }

        ciclo++;

        std::this_thread::sleep_until(proximaExecucao);
    }
}