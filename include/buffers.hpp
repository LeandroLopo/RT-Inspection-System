#pragma once
#include "types.hpp"
#include <queue>
#include <mutex> 
#include <condition_variable>

// Defina aqui os buffers compartilhados entre tarefas.
// Cada buffer deve conter:
// - uma fila std::queue<T> para armazenar dados;
// - um std::mutex para proteger a fila;
// - uma std::condition_variable para acordar a tarefa consumidora;
// - uma flag de finalizacao para encerrar a thread sem travar.
//
// Sugestao de buffers:
// - SensorBuffer: SimulacaoSensores -> ReconstrucaoSuperficie.
// - EncoderBuffer: SimulacaoSensores -> DistanciaPercorrida.
// - SurfaceBuffer: ReconstrucaoSuperficie -> ColetorDados.
// - CameraEvent: ReconstrucaoSuperficie -> InspecaoCamera.

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
