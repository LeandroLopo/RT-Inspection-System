# RT-Inspection-System

Sistema de inspeção automática de túneis para o trabalho final de Automação em Tempo Real.

O projeto é dividido em três processos principais:

- núcleo em C++ com as tarefas de navegação, controle, reconstrução, inspeção e coleta;
- simulação gráfica em Python/Pygame, que publica sensores e recebe atuadores por MQTT;
- operação remota em Python/Pygame, que envia comandos e exibe estado/superfície por MQTT.

## Dependências

No Ubuntu/Debian:

```bash
sudo apt install g++ make mosquitto libmosquitto-dev nlohmann-json3-dev python3 python3-pip
```

Dependências Python:

```bash
python3 -m pip install -r requirements.txt
```

O broker MQTT Mosquitto precisa estar ativo em `localhost:1883`.

## Compilar

```bash
make
```

## Executar

Opcao recomendada, em um unico comando:

```bash
python3 run_all.py
```

Se preferir executar manualmente, use tres terminais, nesta ordem:

```bash
make run
```

```bash
python3 simulation/simulation_gui.py
```

```bash
python3 remote_operation/remote_gui.py
```

O núcleo usa os seguintes tópicos MQTT:

```text
atr/sim/sensors       sensores publicados pela simulação
atr/core/actuators    atuadores publicados pelo núcleo
atr/remote/commands   comandos publicados pela operação remota
atr/core/state        estado publicado pelo núcleo
atr/core/surface      pontos de superfície publicados pelo núcleo
```

## Saídas

A execução gera `surface_points.csv` com:

```csv
timestamp,x,y,confianca
```

## Controles da Operação Remota

```text
A       modo automático
M       modo manual
Setas   movimento manual
Espaço  parar
+ / -   alterar setpoint de velocidade
[ / ]   alterar limite de falha
ESC     sair
```

## Limpeza

```bash
make clean
```
