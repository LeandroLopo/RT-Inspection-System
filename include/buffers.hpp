#pragma once

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
