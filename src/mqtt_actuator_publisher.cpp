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

void PublicaAtuadoresMqtt(SharedActuatorData &sharedActuatorData,
                           SharedSystemControl &systemControl)
{
    struct mosquitto *cliente =
        mosquitto_new("rt_inspection_core_actuators", true, nullptr);

    if (cliente == nullptr) {
        std::lock_guard<std::mutex> trava(coutMutex);
        std::cerr << "Erro: nao foi possivel criar cliente MQTT de atuadores."
                  << std::endl;
        return;
    }

    const int resultadoConexao =
        mosquitto_connect(cliente, "localhost", 1883, 60);

    if (resultadoConexao != MOSQ_ERR_SUCCESS) {
        std::lock_guard<std::mutex> trava(coutMutex);
        std::cerr << "Erro ao conectar publicador de atuadores no MQTT: "
                  << mosquitto_strerror(resultadoConexao)
                  << std::endl;

        mosquitto_destroy(cliente);
        return;
    }

    {
        std::lock_guard<std::mutex> trava(coutMutex);
        std::cout << "Publicador MQTT de atuadores conectado. Topico: "
                  << MqttTopics::ATUADORES
                  << std::endl;
    }

    auto proximaExecucao = std::chrono::steady_clock::now();

    while (true) {
        proximaExecucao += std::chrono::milliseconds(80);

        bool sistemaRodando = true;

        {
            std::lock_guard<std::mutex> trava(systemControl.mutex_sistema);
            sistemaRodando = systemControl.sistema_rodando;
        }

        if (!sistemaRodando) {
            break;
        }

        ActuatorData atuadores;

        {
            std::lock_guard<std::mutex> trava(sharedActuatorData.mutex_atuadores);
            atuadores = sharedActuatorData.atuadores;
        }

        const json mensagem = {
            {"o_aceleracao", atuadores.o_aceleracao},
            {"o_liga_camera", atuadores.o_liga_camera}
        };

        const std::string payload = mensagem.dump();

        const int resultadoPublicacao =
            mosquitto_publish(
                cliente,
                nullptr,
                MqttTopics::ATUADORES,
                static_cast<int>(payload.size()),
                payload.c_str(),
                0,
                false
            );

        if (resultadoPublicacao != MOSQ_ERR_SUCCESS) {
            std::lock_guard<std::mutex> trava(coutMutex);
            std::cerr << "Erro ao publicar atuadores: "
                      << mosquitto_strerror(resultadoPublicacao)
                      << std::endl;
        }

        /*
         * Processa o envio da mensagem para o broker.
         * Como este cliente apenas publica, um loop curto e suficiente.
         */
        const int resultadoLoop = mosquitto_loop(cliente, 10, 1);

        if (resultadoLoop != MOSQ_ERR_SUCCESS) {
            std::lock_guard<std::mutex> trava(coutMutex);
            std::cerr << "Erro no loop MQTT de atuadores: "
                      << mosquitto_strerror(resultadoLoop)
                      << std::endl;
            break;
        }

        std::this_thread::sleep_until(proximaExecucao);
    }

    const json mensagemFinal = {
        {"finalizado", true}
    };

    const std::string payloadFinal = mensagemFinal.dump();

    mosquitto_publish(
        cliente,
        nullptr,
        MqttTopics::ATUADORES,
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
        std::cout << "Publicador MQTT de atuadores finalizado."
                  << std::endl;
    }
}