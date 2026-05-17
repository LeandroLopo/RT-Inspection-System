#pragma once

#include "buffers.hpp"
#include "shared_state.hpp"

void SimulacaoSensores(SensorBuffer &sensorBuffer, EncoderBuffer &encoderBuffer);
void ReconstrucaoSuperficie(SensorBuffer &buffer,
                            PositionBuffer &positionBuffer,
                            SurfaceBuffer &surfaceBuffer,
                            SharedRobotState &robotState,
                            SharedActuatorData &sharedActuatorData,
                            CameraEvent &cameraEvent);
void ColetorDados(SurfaceBuffer &surfaceBuffer);
void DistanciaPercorrida(EncoderBuffer &encoderBuffer, PositionBuffer &positionBuffer, SharedRobotState &robotState);
void ComandoNavegacao(SharedCommand &sharedCommand, SharedRobotState &robotState, SharedSystemControl &systemControl);
void ControleNavegacao(SharedCommand &sharedCommand,
                       SharedRobotState &robotState,
                       SharedActuatorData &sharedActuatorData,
                       SharedSystemControl &systemControl);
void InspecaoCamera(CameraEvent &cameraEvent, SharedRobotState &robotState, SharedActuatorData &sharedActuatorData);
