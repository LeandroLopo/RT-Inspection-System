#include "buffers.hpp"
#include "log.hpp"
#include "mqtt_topics.hpp"

#include <mosquitto.h>
#include <nlohmann/json.hpp>

#include <iostream>
#include <string>

using json = nlohmann::json;

void PublicaSuperficieMqtt(SurfaceBuffer &remoteSurfaceBuffer)
{
    struct mosquitto *cliente =
        mosquitto_new("rt_inspection_core_surface", true, nullptr);

    if (cliente == nullptr) {
        std::lock_guard<std::mutex> trava(coutMutex);

        std::cerr << "Erro: nao foi possivel criar publicador MQTT de superficie."
                  << std::endl;

        return;
    }

    const int resultadoConexao =
        mosquitto_connect(cliente, "localhost", 1883, 60);

    if (resultadoConexao != MOSQ_ERR_SUCCESS) {
        std::lock_guard<std::mutex> trava(coutMutex);

        std::cerr << "Erro ao conectar publicador de superficie no MQTT: "
                  << mosquitto_strerror(resultadoConexao)
                  << std::endl;

        mosquitto_destroy(cliente);
        return;
    }

    {
        std::lock_guard<std::mutex> trava(coutMutex);

        std::cout << "Publicador MQTT de superficie conectado. Topico: "
                  << MqttTopics::SUPERFICIE
                  << std::endl;
    }

    while (true) {
        SurfacePoint ponto;

        {
            std::unique_lock<std::mutex> trava(
                remoteSurfaceBuffer.mutex_superficie
            );

            remoteSurfaceBuffer.surface_point_var.wait(
                trava,
                [&remoteSurfaceBuffer] {
                    return !remoteSurfaceBuffer.fila_superficie.empty()
                        || remoteSurfaceBuffer.finalizado;
                }
            );

            if (
                remoteSurfaceBuffer.fila_superficie.empty()
                && remoteSurfaceBuffer.finalizado
            ) {
                break;
            }

            ponto = remoteSurfaceBuffer.fila_superficie.front();
            remoteSurfaceBuffer.fila_superficie.pop();
        }

        const json mensagem = {
            {"timestamp", ponto.timestamp},
            {"x", ponto.x},
            {"y", ponto.y},
            {"confianca", ponto.confianca}
        };

        const std::string payload = mensagem.dump();

        const int resultadoPublicacao =
            mosquitto_publish(
                cliente,
                nullptr,
                MqttTopics::SUPERFICIE,
                static_cast<int>(payload.size()),
                payload.c_str(),
                0,
                false
            );

        if (resultadoPublicacao != MOSQ_ERR_SUCCESS) {
            std::lock_guard<std::mutex> trava(coutMutex);

            std::cerr << "Erro ao publicar ponto da superficie: "
                      << mosquitto_strerror(resultadoPublicacao)
                      << std::endl;
        }

        mosquitto_loop(cliente, 10, 1);
    }

    const json mensagemFinal = {
        {"finalizado", true}
    };

    const std::string payloadFinal = mensagemFinal.dump();

    mosquitto_publish(
        cliente,
        nullptr,
        MqttTopics::SUPERFICIE,
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

        std::cout << "Publicador MQTT de superficie finalizado."
                  << std::endl;
    }
}