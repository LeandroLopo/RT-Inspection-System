#pragma once

#include "types.hpp"

#include <queue>
#include <mutex>
#include <condition_variable>

struct SensorBuffer {
    std::queue<SensorData> fila_sensor;
    std::mutex mutex_sensor;
    std::condition_variable dado_disponivel_var;
    bool finalizado = false;
};

struct EncoderBuffer {
    std::queue<EncoderData> fila_encoder;
    std::mutex mutex_encoder;
    std::condition_variable encoder_disponivel_var;
    bool finalizado = false;
};

struct PositionBuffer {
    std::queue<PositionData> fila_posicao;
    std::mutex mutex_posicao;
    std::condition_variable posicao_disponivel_var;
    bool finalizado = false;
};

struct SurfaceBuffer {
    std::queue<SurfacePoint> fila_superficie;
    std::mutex mutex_superficie;
    std::condition_variable surface_point_var;
    bool finalizado = false;
};

struct CameraEvent {
    std::mutex mutex_camera;
    std::condition_variable camera_event_var;
    bool falha_detectada = false;
    bool finalizado = false;
    double timestamp = 0.0;
    double x = 0.0;
    double y = 0.0;
};
