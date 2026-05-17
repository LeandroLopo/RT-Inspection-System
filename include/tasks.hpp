#pragma once
#include "buffers.hpp"
#include "shared_state.hpp"

// Declare aqui as funcoes das tarefas paralelas do sistema.
// A implementacao de cada tarefa deve ficar em seu respectivo arquivo .cpp dentro de src/.
//
// Sugestao de tarefas:
// - SimulacaoSensores: gera dados falsos de encoder e LIDAR para a Etapa 1.
// - ReconstrucaoSuperficie: consome sensores, aplica media movel e gera SurfacePoint.
// - ColetorDados: consome SurfacePoint e registra/imprime os dados.
// - DistanciaPercorrida: le encoder e atualiza posicao horizontal.
// - ComandoNavegacao: define modo automatico/manual e setpoint de velocidade.
// - ControleNavegacao: calcula aceleracao a partir do setpoint e da velocidade atual.
// - InspecaoCamera: aguarda falha e simula processamento pesado de camera.


void SimulacaoSensores(SensorBuffer &sensorBuffer, EncoderBuffer &encoderBuffer);
void ReconstrucaoSuperficie(SensorBuffer &buffer, SurfaceBuffer &surfaceBuffer, SharedRobotState &robotState, SharedActuatorData &sharedActuatorData, CameraEvent &cameraEvent);
void ColetorDados(SurfaceBuffer &surfaceBuffer);
void DistanciaPercorrida(EncoderBuffer &encoderBuffer, SharedRobotState &robotState);
void ComandoNavegacao(SharedCommand &sharedCommand, SharedRobotState &robotState, SharedSystemControl &systemControl);
void ControleNavegacao(SharedCommand &sharedCommand, SharedRobotState &robotState, SharedActuatorData &sharedActuatorData, SharedSystemControl &systemControl);
void InspecaoCamera(CameraEvent &cameraEvent, SharedRobotState &robotState, SharedActuatorData &sharedActuatorData);
