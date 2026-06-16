#include "log.hpp"
#include "mqtt_topics.hpp"
#include "shared_state.hpp"

#include <mosquitto.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <iostream>
#include <string>

using json = nlohmann::json;

namespace {

struct MqttCommandContext {
    SharedCommand *sharedCommand = nullptr;
    SharedSystemParameters *systemParameters = nullptr;
};

void AoReceberComando(struct mosquitto *,
                       void *userdata,
                       const struct mosquitto_message *message)
{
    auto *contexto = static_cast<MqttCommandContext *>(userdata);

    if (message == nullptr || message->payload == nullptr) {
        return;
    }

    const std::string payload(
        static_cast<char *>(message->payload),
        static_cast<std::size_t>(message->payloadlen)
    );

    try {
        const json mensagem = json::parse(payload);

        RobotCommand comando;

        comando.c_automatico = mensagem.value("c_automatico", true);
        comando.c_man = mensagem.value("c_man", false);
        comando.c_direita = mensagem.value("c_direita", false);
        comando.c_esquerda = mensagem.value("c_esquerda", false);
        comando.c_para = mensagem.value("c_para", false);

        comando.j_sp_velocidade = std::clamp(
            mensagem.value("j_sp_velocidade", 2),
            0,
            3
        );

        const double limiteFalha = std::clamp(
            mensagem.value("limite_falha", 10.0),
            1.0,
            50.0
        );

        {
            std::lock_guard<std::mutex> trava(
                contexto->sharedCommand->mutex_comando
            );

            contexto->sharedCommand->comando = comando;
        }

        {
            std::lock_guard<std::mutex> trava(
                contexto->systemParameters->mutex_parametros
            );

            contexto->systemParameters->limite_falha = limiteFalha;
        }

        {
            std::lock_guard<std::mutex> trava(coutMutex);

            std::cout << "Comando MQTT recebido: automatico="
                      << comando.c_automatico
                      << " manual=" << comando.c_man
                      << " direita=" << comando.c_direita
                      << " esquerda=" << comando.c_esquerda
                      << " parar=" << comando.c_para
                      << " setpoint=" << comando.j_sp_velocidade
                      << " limite_falha=" << limiteFalha
                      << std::endl;
        }
    }
    catch (const std::exception &erro) {
        std::lock_guard<std::mutex> trava(coutMutex);

        std::cerr << "Erro ao interpretar comando MQTT: "
                  << erro.what()
                  << " payload=" << payload
                  << std::endl;
    }
}

} // namespace

void RecebeComandosMqtt(SharedCommand &sharedCommand,
                         SharedSystemParameters &systemParameters,
                         SharedSystemControl &systemControl)
{
    MqttCommandContext contexto;

    contexto.sharedCommand = &sharedCommand;
    contexto.systemParameters = &systemParameters;

    struct mosquitto *cliente =
        mosquitto_new("rt_inspection_core_commands", true, &contexto);

    if (cliente == nullptr) {
        std::lock_guard<std::mutex> trava(coutMutex);

        std::cerr << "Erro: nao foi possivel criar cliente MQTT de comandos."
                  << std::endl;

        return;
    }

    mosquitto_message_callback_set(cliente, AoReceberComando);

    const int resultadoConexao =
        mosquitto_connect(cliente, "localhost", 1883, 60);

    if (resultadoConexao != MOSQ_ERR_SUCCESS) {
        std::lock_guard<std::mutex> trava(coutMutex);

        std::cerr << "Erro ao conectar receptor de comandos no MQTT: "
                  << mosquitto_strerror(resultadoConexao)
                  << std::endl;

        mosquitto_destroy(cliente);
        return;
    }

    const int resultadoInscricao =
        mosquitto_subscribe(cliente, nullptr, MqttTopics::COMANDOS, 0);

    if (resultadoInscricao != MOSQ_ERR_SUCCESS) {
        std::lock_guard<std::mutex> trava(coutMutex);

        std::cerr << "Erro ao assinar topico de comandos: "
                  << mosquitto_strerror(resultadoInscricao)
                  << std::endl;

        mosquitto_disconnect(cliente);
        mosquitto_destroy(cliente);
        return;
    }

    {
        std::lock_guard<std::mutex> trava(coutMutex);

        std::cout << "Receptor MQTT de comandos conectado. Topico: "
                  << MqttTopics::COMANDOS
                  << std::endl;
    }

    while (true) {
        {
            std::lock_guard<std::mutex> trava(systemControl.mutex_sistema);

            if (!systemControl.sistema_rodando) {
                break;
            }
        }

        const int resultadoLoop = mosquitto_loop(cliente, 100, 1);

        if (resultadoLoop != MOSQ_ERR_SUCCESS) {
            std::lock_guard<std::mutex> trava(coutMutex);

            std::cerr << "Erro no loop MQTT de comandos: "
                      << mosquitto_strerror(resultadoLoop)
                      << std::endl;

            break;
        }
    }

    mosquitto_disconnect(cliente);
    mosquitto_destroy(cliente);

    {
        std::lock_guard<std::mutex> trava(coutMutex);

        std::cout << "Receptor MQTT de comandos finalizado."
                  << std::endl;
    }
}