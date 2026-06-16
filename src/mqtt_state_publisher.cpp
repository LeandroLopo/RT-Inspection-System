#include "log.hpp"
#include "mqtt_topics.hpp"
#include "shared_state.hpp"

#include <mosquitto.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

using json = nlohmann::json;

void PublicaEstadoMqtt(SharedRobotState &robotState,
                       SharedActuatorData &sharedActuatorData,
                       SharedSystemParameters &systemParameters,
                       SharedSystemControl &systemControl)
{
    struct mosquitto *cliente =
        mosquitto_new("rt_inspection_core_state", true, nullptr);

    if (cliente == nullptr) {
        std::lock_guard<std::mutex> trava(coutMutex);

        std::cerr << "Erro: nao foi possivel criar publicador MQTT de estado."
                  << std::endl;

        return;
    }

    const int resultadoConexao =
        mosquitto_connect(cliente, "localhost", 1883, 60);

    if (resultadoConexao != MOSQ_ERR_SUCCESS) {
        std::lock_guard<std::mutex> trava(coutMutex);

        std::cerr << "Erro ao conectar publicador de estado no MQTT: "
                  << mosquitto_strerror(resultadoConexao)
                  << std::endl;

        mosquitto_destroy(cliente);
        return;
    }

    {
        std::lock_guard<std::mutex> trava(coutMutex);

        std::cout << "Publicador MQTT de estado conectado. Topico: "
                  << MqttTopics::ESTADO
                  << std::endl;
    }

    auto proximaExecucao = std::chrono::steady_clock::now();

    while (true) {
        proximaExecucao += std::chrono::milliseconds(80);

        bool sistemaRodando;

        {
            std::lock_guard<std::mutex> trava(systemControl.mutex_sistema);
            sistemaRodando = systemControl.sistema_rodando;
        }

        if (!sistemaRodando) {
            break;
        }

        RobotState estado;
        ActuatorData atuadores;
        double limiteFalha;

        {
            std::lock_guard<std::mutex> trava(robotState.mutex_estado);
            estado = robotState.estado;
        }

        {
            std::lock_guard<std::mutex> trava(
                sharedActuatorData.mutex_atuadores
            );

            atuadores = sharedActuatorData.atuadores;
        }

        {
            std::lock_guard<std::mutex> trava(
                systemParameters.mutex_parametros
            );

            limiteFalha = systemParameters.limite_falha;
        }

        const json mensagem = {
            {"e_automatico", estado.e_automatico},
            {"e_inspecao", estado.e_inspecao},
            {"posicao_x", estado.posicao_x},
            {"velocidade", estado.velocidade},
            {"o_aceleracao", atuadores.o_aceleracao},
            {"o_liga_camera", atuadores.o_liga_camera},
            {"limite_falha", limiteFalha}
        };

        const std::string payload = mensagem.dump();

        const int resultadoPublicacao =
            mosquitto_publish(
                cliente,
                nullptr,
                MqttTopics::ESTADO,
                static_cast<int>(payload.size()),
                payload.c_str(),
                0,
                false
            );

        if (resultadoPublicacao != MOSQ_ERR_SUCCESS) {
            std::lock_guard<std::mutex> trava(coutMutex);

            std::cerr << "Erro ao publicar estado: "
                      << mosquitto_strerror(resultadoPublicacao)
                      << std::endl;
        }

        mosquitto_loop(cliente, 10, 1);

        std::this_thread::sleep_until(proximaExecucao);
    }

    const json mensagemFinal = {
        {"finalizado", true}
    };

    const std::string payloadFinal = mensagemFinal.dump();

    mosquitto_publish(
        cliente,
        nullptr,
        MqttTopics::ESTADO,
        static_cast<int>(payloadFinal.size()),
        payloadFinal.c_str(),
        0,
        false
    );

    mosquitto_loop(cliente, 100, 1);

    mosquitto_disconnect(cliente);
    mosquitto_destroy(cliente);

    {
        std::lock_guard<std::mutex> trava(coutMutex);

        std::cout << "Publicador MQTT de estado finalizado."
                  << std::endl;
    }
}