#pragma once

struct SensorData {
    bool i_encoder = false;
    int i_lidar = 0;
    double timestamp = 0.0;
};

struct EncoderData {
    bool i_encoder = false;
    double timestamp = 0.0;
};

struct PositionData {
    double timestamp = 0.0;
    double x = 0.0;
};

struct SurfacePoint {
    double timestamp = 0.0;
    double x = 0.0;
    double y = 0.0;
    double confianca = 0.0;
};

struct RobotCommand {
    int j_sp_velocidade = 0;
    bool c_automatico = false;
    bool c_man = false;
    bool c_direita = false;
    bool c_esquerda = false;
    bool c_para = false;
};

struct RobotState {
    bool e_automatico = true;
    bool e_inspecao = false;
    double posicao_x = 0.0;
    double velocidade = 0.0;
};

struct ActuatorData {
    int o_aceleracao = 0;
    bool o_liga_camera = false;
};
