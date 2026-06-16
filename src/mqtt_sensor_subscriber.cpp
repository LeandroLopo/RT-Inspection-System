#include "buffers.hpp"
#include "log.hpp"
#include "mqtt_topics.hpp"
#include "shared_state.hpp"
#include "types.hpp"

#include <mosquitto.h>
#include <nlohmann/json.hpp>

#include <atomic>
#include <iostream>
#include <string>

using json = nlohmann::json;

namespace {

struct MqttSensorContext {
    SensorBuffer *sensorBuffer = nullptr;
    EncoderBuffer *encoderBuffer = nullptr;
    SharedRobotState *robotState = nullptr;
    std::atomic<bool> finalizado{false};
};

void FinalizaBuffers(MqttSensorContext &contexto)
{
    {
        std::lock_guard<std::mutex> trava(contexto.sensorBuffer->mutex_sensor);
        contexto.sensorBuffer->finalizado = true;
    }
    contexto.sensorBuffer->dado_disponivel_var.notify_all();

    {
        std::lock_guard<std::mutex> trava(contexto.encoderBuffer->mutex_encoder);
        contexto.encoderBuffer->finalizado = true;
    }
    contexto.encoderBuffer->encoder_disponivel_var.notify_all();

    contexto.finalizado = true;
}

void AoReceberMensagem(struct mosquitto *,
                        void *userdata,
                        const struct mosquitto_message *message)
{
    auto *contexto = static_cast<MqttSensorContext *>(userdata);

    if (message == nullptr || message->payload == nullptr) {
        return;
    }

    const std::string payload(
        static_cast<char *>(message->payload),
        static_cast<std::size_t>(message->payloadlen)
    );

    try {
        const json mensagem = json::parse(payload);

        if (mensagem.value("finalizado", false)) {
            {
                std::lock_guard<std::mutex> trava(coutMutex);
                std::cout << "MQTT: finalizacao dos sensores recebida."
                          << std::endl;
            }

            FinalizaBuffers(*contexto);
            return;
        }

        SensorData leitura;
        leitura.i_encoder = mensagem.at("i_encoder").get<bool>();
        leitura.i_lidar = mensagem.at("i_lidar").get<int>();
        leitura.timestamp = mensagem.at("timestamp").get<double>();

        EncoderData encoder;
        encoder.i_encoder = leitura.i_encoder;
        encoder.timestamp = leitura.timestamp;

        const double velocidadeMedida =
            mensagem.value("velocidade", 0.0);

        {
            std::lock_guard<std::mutex> trava(contexto->robotState->mutex_estado);
            contexto->robotState->estado.velocidade = velocidadeMedida;
        }

        {
            std::lock_guard<std::mutex> trava(contexto->sensorBuffer->mutex_sensor);
            contexto->sensorBuffer->fila_sensor.push(leitura);
        }
        contexto->sensorBuffer->dado_disponivel_var.notify_one();

        {
            std::lock_guard<std::mutex> trava(contexto->encoderBuffer->mutex_encoder);
            contexto->encoderBuffer->fila_encoder.push(encoder);
        }
        contexto->encoderBuffer->encoder_disponivel_var.notify_one();

        {
            std::lock_guard<std::mutex> trava(coutMutex);
            std::cout << "MQTT sensor recebido: encoder="
                      << leitura.i_encoder
                      << " lidar=" << leitura.i_lidar
                      << " velocidade=" << velocidadeMedida
                      << " timestamp=" << leitura.timestamp
                      << std::endl;
        }
    }
    catch (const std::exception &erro) {
        std::lock_guard<std::mutex> trava(coutMutex);
        std::cerr << "Erro ao interpretar mensagem MQTT: "
                  << erro.what()
                  << " payload=" << payload
                  << std::endl;
    }
}

} // namespace

void RecebeSensoresMqtt(SensorBuffer &sensorBuffer,
                         EncoderBuffer &encoderBuffer,
                         SharedRobotState &robotState)
{
    MqttSensorContext contexto;
    contexto.sensorBuffer = &sensorBuffer;
    contexto.encoderBuffer = &encoderBuffer;
    contexto.robotState = &robotState;

    struct mosquitto *cliente =
        mosquitto_new("rt_inspection_core_sensors", true, &contexto);

    if (cliente == nullptr) {
        std::cerr << "Erro: nao foi possivel criar cliente MQTT de sensores."
                  << std::endl;
        FinalizaBuffers(contexto);
        return;
    }

    mosquitto_message_callback_set(cliente, AoReceberMensagem);

    const int resultadoConexao =
        mosquitto_connect(cliente, "localhost", 1883, 60);

    if (resultadoConexao != MOSQ_ERR_SUCCESS) {
        std::cerr << "Erro ao conectar receptor de sensores no MQTT: "
                  << mosquitto_strerror(resultadoConexao)
                  << std::endl;

        FinalizaBuffers(contexto);
        mosquitto_destroy(cliente);
        return;
    }

    const int resultadoInscricao =
        mosquitto_subscribe(cliente, nullptr, MqttTopics::SENSORES, 0);

    if (resultadoInscricao != MOSQ_ERR_SUCCESS) {
        std::cerr << "Erro ao assinar topico MQTT de sensores: "
                  << mosquitto_strerror(resultadoInscricao)
                  << std::endl;

        FinalizaBuffers(contexto);
        mosquitto_disconnect(cliente);
        mosquitto_destroy(cliente);
        return;
    }

    {
        std::lock_guard<std::mutex> trava(coutMutex);
        std::cout << "MQTT conectado. Aguardando sensores no topico: "
                  << MqttTopics::SENSORES
                  << std::endl;
    }

    while (!contexto.finalizado) {
        const int resultadoLoop = mosquitto_loop(cliente, 100, 1);

        if (resultadoLoop != MOSQ_ERR_SUCCESS) {
            std::cerr << "Erro no loop MQTT de sensores: "
                      << mosquitto_strerror(resultadoLoop)
                      << std::endl;

            FinalizaBuffers(contexto);
            break;
        }
    }

    mosquitto_disconnect(cliente);
    mosquitto_destroy(cliente);
}