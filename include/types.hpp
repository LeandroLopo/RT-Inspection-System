#pragma once

// Defina aqui as estruturas de dados simples usadas por varias tarefas.
// Sugestao:
// - SensorData: leitura do encoder, LIDAR e timestamp.
// - SurfacePoint: ponto reconstruido do teto com timestamp, x, y e confianca.
// - RobotCommand: modo automatico/manual e setpoint de velocidade.
// - RobotState: estado atual do robo, incluindo posicao, velocidade e inspecao.
// - ActuatorData: aceleracao e comando para ligar a camera.


struct SensorData {
    bool i_encoder; // troca de estado a cada metro percorrido pelo robô
    int i_lidar; // distância no eixo y, com relação à altura do robô
    double timestamp; //
};

struct EncoderData {
    bool i_encoder;
    double timestamp;
};

struct SurfacePoint {
    double timestamp;
    double x;
    double y;
    double confianca; // Quanto mais medições próximas, maior o nível de confiança
};

struct RobotCommand {
    int j_sp_velocidade; // Setpoint de velocidade 
    bool c_automatico; // passar o robô para o modo automático (true). O reset desse comando
    bool c_man; // Comando para passar o robô para o modo manual (true).
    bool c_direita; /// Comando para acelerar o robô para a direita.
    bool c_esquerda; 
    bool c_para;
};

struct RobotState {
    bool e_automatico; // 0=manual 1=automático
    bool e_inspecao; 
    double posicao_x;
    double velocidade;
   
};

struct ActuatorData {
    int o_aceleracao; //Determina a aceleração do veículo em percentual (-100 a 100%)
    bool o_liga_camera; // ligar a câmera e realizar fotos da falha detectada na superfície
};
