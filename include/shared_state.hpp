#pragma once

#include "types.hpp"

#include <mutex>

// Defina aqui estados compartilhados que nao sao naturalmente uma fila.
// Use mutex para proteger leituras e escritas desses dados.
//
// Sugestao:
// - SharedRobotState: posicao X, velocidade atual, modo automatico e estado de inspecao.
// - SharedCommand: comando atual vindo da operacao remota ou simulado na Etapa 1.
// - SharedActuatorData: aceleracao calculada e comando para ligar camera.
//
// Regra pratica:
// trave o mutex, copie o estado para uma variavel local, destrave e processe fora da regiao critica.

struct SharedRobotState {
    RobotState estado{true, false, 0.0, 0.0};
    std::mutex mutex_estado;
};

struct SharedCommand {
    RobotCommand comando{0, false, false, false, false, false};
    std::mutex mutex_comando;
};

struct SharedActuatorData {
    ActuatorData atuadores{0, false};
    std::mutex mutex_atuadores;
};

struct SharedSystemControl {
    bool sistema_rodando = true;
    std::mutex mutex_sistema;
};
