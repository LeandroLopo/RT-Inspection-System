#pragma once
#include "buffers.hpp"

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


void SimulacaoSensores(SensorBuffer &buffer);
void ReconstrucaoSuperficie(SensorBuffer &buffer);
