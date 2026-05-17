#pragma once

#include "types.hpp"

#include <mutex>

struct SharedRobotState {
    RobotState estado;
    std::mutex mutex_estado;
};

struct SharedCommand {
    RobotCommand comando;
    std::mutex mutex_comando;
};

struct SharedActuatorData {
    ActuatorData atuadores;
    std::mutex mutex_atuadores;
};

struct SharedSystemControl {
    bool sistema_rodando = true;
    std::mutex mutex_sistema;
};
