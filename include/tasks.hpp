#pragma once

#include "buffers.hpp"
#include "shared_state.hpp"

void SimulacaoSensores(SensorBuffer &sensorBuffer,
                       EncoderBuffer &encoderBuffer);

void RecebeSensoresMqtt(SensorBuffer &sensorBuffer,
                         EncoderBuffer &encoderBuffer,
                         SharedRobotState &robotState);

void RecebeComandosMqtt(SharedCommand &sharedCommand,
                         SharedSystemParameters &systemParameters,
                         SharedSystemControl &systemControl);

void PublicaAtuadoresMqtt(SharedActuatorData &sharedActuatorData,
                           SharedSystemControl &systemControl);

void PublicaEstadoMqtt(SharedRobotState &robotState,
                       SharedActuatorData &sharedActuatorData,
                       SharedSystemParameters &systemParameters,
                       SharedSystemControl &systemControl);

void PublicaSuperficieMqtt(SurfaceBuffer &remoteSurfaceBuffer);

void ReconstrucaoSuperficie(SensorBuffer &buffer,
                             PositionBuffer &positionBuffer,
                             SurfaceBuffer &surfaceBuffer,
                             SurfaceBuffer &remoteSurfaceBuffer,
                             SharedRobotState &robotState,
                             SharedActuatorData &sharedActuatorData,
                             SharedSystemParameters &systemParameters,
                             CameraEvent &cameraEvent);

void ColetorDados(SurfaceBuffer &surfaceBuffer);

void DistanciaPercorrida(EncoderBuffer &encoderBuffer,
                          PositionBuffer &positionBuffer,
                          SharedRobotState &robotState);

void ComandoNavegacao(SharedCommand &sharedCommand,
                       SharedRobotState &robotState,
                       SharedSystemControl &systemControl);

void ControleNavegacao(SharedCommand &sharedCommand,
                        SharedRobotState &robotState,
                        SharedActuatorData &sharedActuatorData,
                        SharedSystemControl &systemControl);

void InspecaoCamera(CameraEvent &cameraEvent,
                    SharedRobotState &robotState,
                    SharedActuatorData &sharedActuatorData); 