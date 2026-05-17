# Arquitetura da Etapa 1

Este diagrama em Mermaid pode ser usado como base para a figura do relatorio.

```mermaid
flowchart LR
    subgraph Core["rt_inspection_core - processo C++ unico"]
        Sim["SimulacaoSensores<br/>thread de teste"]
        Cmd["ComandoNavegacao<br/>thread 80 ms"]
        Ctrl["ControleNavegacao<br/>thread 80 ms"]
        Dist["DistanciaPercorrida<br/>thread"]
        Rec["ReconstrucaoSuperficie<br/>thread"]
        Cam["InspecaoCamera<br/>thread por evento"]
        Col["ColetorDados<br/>thread"]

        SensorBuf[("SensorBuffer<br/>queue + mutex + condition_variable")]
        EncoderBuf[("EncoderBuffer<br/>queue + mutex + condition_variable")]
        SurfaceBuf[("SurfaceBuffer<br/>queue + mutex + condition_variable")]
        CameraEvt[("CameraEvent<br/>mutex + condition_variable")]

        RobotState[("SharedRobotState<br/>posicao, velocidade, estados")]
        CommandState[("SharedCommand<br/>comando simulado")]
        ActuatorState[("SharedActuatorData<br/>aceleracao, camera")]
    end

    Sim --> SensorBuf --> Rec
    Sim --> EncoderBuf --> Dist --> RobotState
    RobotState --> Rec
    Rec --> SurfaceBuf --> Col
    Rec --> CameraEvt --> Cam
    Rec --> ActuatorState
    Cmd --> CommandState --> Ctrl --> ActuatorState
    RobotState --> Ctrl
    Cam --> RobotState
    Cam --> ActuatorState
    Col --> CSV[/"surface_points.csv"/]

    Etapa2["Etapa 2: MQTT, IPC, GUI de simulacao e GUI de operacao remota"]
```

Legenda:

- Retangulos: tarefas implementadas como threads C++.
- Cilindros: buffers ou estados compartilhados em memoria.
- Setas: fluxo de dados ou sinalizacao entre tarefas.
- `SensorBuffer`, `EncoderBuffer` e `SurfaceBuffer`: padrao produtor-consumidor.
- `CameraEvent`: evento disparado quando a reconstrucao detecta falha.
