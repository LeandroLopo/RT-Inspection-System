# RT-Inspection-System

Sistema modular em C++ para a Etapa 1 do trabalho de Automacao em Tempo Real.

## Arquitetura

A Etapa 1 usa um unico processo C++ com varias threads internas. A comunicacao entre tarefas ocorre por memoria compartilhada, usando buffers protegidos por `std::mutex` e `std::condition_variable`.

Fluxos principais:

```text
SimulacaoSensores -> SensorBuffer -> ReconstrucaoSuperficie
SimulacaoSensores -> EncoderBuffer -> DistanciaPercorrida -> SharedRobotState
ReconstrucaoSuperficie -> SurfaceBuffer -> ColetorDados -> surface_points.csv
ReconstrucaoSuperficie -> CameraEvent -> InspecaoCamera
ComandoNavegacao -> SharedCommand -> ControleNavegacao -> SharedActuatorData
```

MQTT, IPC e interfaces graficas ficam para a Etapa 2.

## Compilar

```bash
make
```

## Executar

```bash
make run
```

## Limpar

```bash
make clean
```

## Saidas

A execucao gera:

```text
surface_points.csv
```

Formato:

```csv
timestamp,x,y,confianca
```

## Estrutura

```text
include/   tipos, buffers, estados compartilhados e declaracoes
src/       implementacao das tarefas
Makefile   comandos de compilacao, execucao e limpeza
```
