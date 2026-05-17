# Arquitetura proposta do projeto

Este diagrama registra a arquitetura completa proposta para o projeto. Na Etapa 1 foi implementado o núcleo C++ com threads e buffers internos. Na Etapa 2, os processos externos de simulação, operação remota e broker MQTT devem ser integrados ao núcleo.

```mermaid
flowchart TB
    SimExt["GUI de Simulacao<br/>processo externo<br/>Etapa 2"]
    Broker[("Broker MQTT<br/>publisher/subscriber")]
    Remote["GUI de Operacao Remota<br/>processo externo<br/>Etapa 2"]

    subgraph Core["rt_inspection_core - processo C++"]
        Threads["Threads internas<br/>Comando, Controle PID, Distancia,<br/>Reconstrucao, Camera, Coletor"]
        Buffers[("Buffers e estados compartilhados<br/>mutex + condition_variable")]
        CSV[/"surface_points.csv<br/>saida local Etapa 1"/]
        Threads <--> Buffers
        Threads --> CSV
    end

    SimExt <--> |sensores e atuadores| Broker
    Remote <--> |comandos e telemetria| Broker
    Broker <--> |IPC/MQTT planejado| Core
```

## Detalhamento do núcleo implementado na Etapa 1

```mermaid
flowchart LR
    subgraph Core["rt_inspection_core - processo C++ unico"]
        Sim["SimulacaoSensores<br/>thread de teste"]
        Cmd["ComandoNavegacao<br/>thread 80 ms"]
        Ctrl["ControleNavegacao PID<br/>thread 80 ms"]
        Dist["DistanciaPercorrida<br/>thread"]
        Rec["ReconstrucaoSuperficie<br/>thread"]
        Cam["InspecaoCamera<br/>thread por evento"]
        Col["ColetorDados<br/>thread"]

        SensorBuf[("SensorBuffer<br/>queue + mutex + condition_variable")]
        EncoderBuf[("EncoderBuffer<br/>queue + mutex + condition_variable")]
        PositionBuf[("PositionBuffer<br/>queue + mutex + condition_variable")]
        SurfaceBuf[("SurfaceBuffer<br/>queue + mutex + condition_variable")]
        CameraEvt[("CameraEvent<br/>mutex + condition_variable")]

        RobotState[("SharedRobotState<br/>posicao, velocidade, estados")]
        CommandState[("SharedCommand<br/>comando simulado")]
        ActuatorState[("SharedActuatorData<br/>aceleracao, camera")]
    end

    Sim --> SensorBuf --> Rec
    Sim --> EncoderBuf --> Dist --> PositionBuf --> Rec
    Dist --> RobotState
    Rec --> SurfaceBuf --> Col
    Rec --> CameraEvt --> Cam
    Rec --> ActuatorState
    Cmd --> CommandState --> Ctrl --> ActuatorState
    RobotState --> Ctrl
    Cam --> RobotState
    Cam --> ActuatorState
    Col --> CSV[/"surface_points.csv"/]

    Etapa2["Processos externos planejados: MQTT, GUI de simulacao e GUI de operacao remota"]
```

Legenda:

- Retangulos: tarefas implementadas como threads C++.
- Cilindros: buffers ou estados compartilhados em memoria.
- Setas: fluxo de dados ou sinalizacao entre tarefas.
- `SensorBuffer`, `EncoderBuffer`, `PositionBuffer` e `SurfaceBuffer`: padrao produtor-consumidor.
- `CameraEvent`: evento disparado quando a reconstrucao detecta falha.
