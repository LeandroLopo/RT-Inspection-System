# RT-Inspection-System

Sistema modular em C++ para a Etapa 1 do trabalho de Automacao em Tempo Real.

## Arquitetura

A arquitetura proposta cobre o projeto completo. O nucleo do robo fica em um processo C++ com varias threads internas. A comunicacao entre essas threads ocorre por memoria compartilhada, usando buffers protegidos por `std::mutex` e `std::condition_variable`.

Na Etapa 2, a simulacao grafica, a operacao remota e o broker MQTT entram como processos externos comunicando com o nucleo por IPC/MQTT. Na Etapa 1, `SimulacaoSensores` substitui temporariamente a simulacao grafica para testar os buffers internos.

Fluxos principais:

```text
SimulacaoSensores -> SensorBuffer -> ReconstrucaoSuperficie
SimulacaoSensores -> EncoderBuffer -> DistanciaPercorrida -> PositionBuffer -> ReconstrucaoSuperficie
DistanciaPercorrida -> SharedRobotState
ReconstrucaoSuperficie -> SurfaceBuffer -> ColetorDados -> surface_points.csv
ReconstrucaoSuperficie -> CameraEvent -> InspecaoCamera
ComandoNavegacao -> SharedCommand -> ControleNavegacao -> SharedActuatorData
```

MQTT, IPC e interfaces graficas estao definidos na arquitetura completa e ficam para implementacao na Etapa 2.

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
