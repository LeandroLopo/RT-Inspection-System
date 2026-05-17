# Relatorio da Etapa 1 - Rascunho

Este arquivo serve como base de controle para escrever o relatorio em PDF da Etapa 1.

## Tipo de Arquitetura

Na Etapa 1 foi adotada uma arquitetura multitarefa em processo unico.

As tarefas internas do robo, indicadas em azul na Figura 3 do enunciado, foram modeladas como threads C++ dentro do processo principal. A comunicacao entre tarefas ocorre por memoria compartilhada, usando buffers do tipo produtor-consumidor protegidos por mutexes e variaveis de condicao.

Essa escolha reduz a complexidade da primeira etapa e permite demonstrar diretamente os mecanismos de sincronizacao exigidos no trabalho. A comunicacao por IPC/MQTT e as interfaces graficas foram deixadas para a Etapa 2, conforme especificado no enunciado.

Descricao curta:

```text
Sistema concorrente multitarefa, modular, baseado em threads, com comunicacao interna por buffers sincronizados em memoria compartilhada.
```

## Organizacao Geral

O sistema da Etapa 1 e organizado em um unico processo principal:

```text
rt_inspection_core
```

Dentro desse processo, cada tarefa principal do robo e implementada como uma thread:

```text
ComandoNavegacao
ControleNavegacao
DistanciaPercorrida
ReconstrucaoSuperficie
InspecaoCamera
ColetorDados
```

Alem dessas tarefas, existe uma tarefa auxiliar de teste:

```text
SimulacaoSensores
```

Essa tarefa substitui temporariamente os blocos externos de simulacao da Etapa 2. Ela gera dados falsos de sensores para permitir testar os buffers e a sincronizacao entre as tarefas internas.

## Justificativa da Arquitetura

As tarefas azuis da Figura 3 representam tarefas internas do sistema embarcado do robo. Como elas precisam trocar dados com baixa latencia e fazem parte do mesmo nucleo de controle, foi adotada comunicacao direta por memoria compartilhada.

Essa decisao evita o uso antecipado de IPC na Etapa 1. O IPC via MQTT sera usado na Etapa 2, quando forem implementados processos externos, como:

```text
Interface de simulacao
Interface de operacao remota
Broker MQTT
```

Assim, a arquitetura da Etapa 1 fica mais simples e diretamente focada nos conceitos da disciplina:

- execucao paralela com threads;
- regioes criticas;
- mutexes;
- variaveis de condicao;
- buffers compartilhados;
- troca de dados entre tarefas.

## Modularizacao do Codigo

O codigo foi organizado em arquivos separados para facilitar entendimento, manutencao e testes.

Estrutura planejada:

```text
RT-Inspection-System/
├── include/
│   ├── types.hpp
│   ├── buffers.hpp
│   ├── shared_state.hpp
│   ├── time_utils.hpp
│   ├── log.hpp
│   └── tasks.hpp
│
├── src/
│   ├── main.cpp
│   ├── time_utils.cpp
│   ├── log.cpp
│   ├── simulacao_sensores.cpp
│   ├── reconstrucao_superficie.cpp
│   ├── coletor_dados.cpp
│   ├── distancia_percorrida.cpp
│   ├── comando_navegacao.cpp
│   ├── controle_navegacao.cpp
│   └── inspecao_camera.cpp
```

Os arquivos da pasta `include/` armazenam tipos, estruturas compartilhadas e declaracoes de funcoes. Os arquivos da pasta `src/` armazenam as implementacoes das tarefas.

Essa organizacao permite associar cada arquivo `.cpp` a uma tarefa da arquitetura do sistema.

## Padrao Produtor-Consumidor

O principal padrao de comunicacao usado na Etapa 1 e o produtor-consumidor.

Exemplo 1:

```text
SimulacaoSensores -> SensorBuffer -> ReconstrucaoSuperficie
```

Nesse fluxo:

```text
SimulacaoSensores = produtora
SensorBuffer = buffer compartilhado
ReconstrucaoSuperficie = consumidora
```

Exemplo 2:

```text
DistanciaPercorrida -> PositionBuffer -> ReconstrucaoSuperficie
ReconstrucaoSuperficie -> SurfaceBuffer -> ColetorDados
```

Nesse fluxo:

```text
ReconstrucaoSuperficie = produtora
SurfaceBuffer = buffer compartilhado
ColetorDados = consumidora
```

Exemplo 3:

```text
SimulacaoSensores -> EncoderBuffer -> DistanciaPercorrida -> PositionBuffer -> ReconstrucaoSuperficie
```

Nesse fluxo, a simulacao publica leituras do encoder em um buffer proprio. A tarefa `DistanciaPercorrida` consome essas leituras, atualiza a posicao horizontal do robo em `SharedRobotState` e publica cada posicao em `PositionBuffer`.

## Buffers e Estados Compartilhados Implementados

Buffers com fila, mutex, variavel de condicao e flag de finalizacao:

- `SensorBuffer`: transporta `SensorData` da simulacao para a reconstrucao.
- `EncoderBuffer`: transporta `EncoderData` da simulacao para o calculo de distancia.
- `PositionBuffer`: transporta a posicao calculada para a reconstrucao.
- `SurfaceBuffer`: transporta `SurfacePoint` da reconstrucao para o coletor.
- `CameraEvent`: sinaliza falha da reconstrucao para a camera.

Estados compartilhados protegidos por mutex:

- `SharedRobotState`: posicao, velocidade, modo automatico e estado de inspecao.
- `SharedCommand`: comando simulado da operacao remota na Etapa 1.
- `SharedActuatorData`: aceleracao e acionamento da camera.
- `SharedSystemControl`: flag de encerramento das tarefas ciclicas.

## Tarefas Implementadas

- `SimulacaoSensores`: gera leituras falsas de LIDAR e encoder.
- `ReconstrucaoSuperficie`: aplica media movel no LIDAR, gera pontos de superficie e detecta falhas.
- `DistanciaPercorrida`: calcula `posicao_x` a partir da troca de estado do encoder e publica amostras de posicao.
- `ColetorDados`: consome pontos reconstruidos, calcula confianca online e grava CSV.
- `ComandoNavegacao`: gera comando simulado de modo automatico e setpoint de velocidade.
- `ControleNavegacao`: aplica controle PID simples e gera aceleracao.
- `InspecaoCamera`: aguarda evento de falha e executa processamento pesado usando CPU.

## Mecanismos de Sincronizacao

### Mutex

O `mutex` e usado para proteger dados compartilhados entre threads.

No caso dos buffers, a regiao critica e o acesso a fila:

```text
inserir item na fila
remover item da fila
verificar se a fila esta vazia
alterar flag de finalizacao
```

Regra usada:

```text
travar o mutex antes de acessar o dado compartilhado
copiar ou modificar apenas o necessario
destravar o mutex
processar os dados fora da regiao critica
```

### Condition Variable

A `condition_variable` e usada para evitar espera ocupada.

Em vez de uma thread consumidora ficar verificando repetidamente se ha dados na fila, ela fica bloqueada ate que a thread produtora insira um novo dado e envie uma notificacao.

Fluxo geral:

```text
1. Consumidor espera na condition_variable.
2. Produtor insere dado no buffer.
3. Produtor chama notify_one().
4. Consumidor acorda.
5. Consumidor remove dado da fila.
6. Consumidor processa o dado fora da regiao critica.
```

## O Que Fica Para a Etapa 2

Conforme o enunciado, os seguintes itens nao precisam ser implementados na Etapa 1:

- comunicacao MQTT;
- IPC entre processos;
- interface grafica de simulacao;
- interface grafica de operacao remota;
- sistema completo de visualizacao;
- integracao final com broker publisher/subscriber.

Esses elementos serao implementados na Etapa 2, quando o sistema passar a ter processos externos comunicando com o nucleo C++.

## Teste Executado

Comandos:

```bash
make clean
make
make run
```

Resultado observado:

- o sistema executa as threads principais;
- a simulacao gera uma anomalia no LIDAR;
- a reconstrucao detecta a falha por variacao brusca da media movel;
- a camera e acionada por evento e executa processamento pesado;
- o coletor grava `surface_points.csv`.

Exemplo de saida:

```text
Falha detectada: x=5 y=115.667 variacao=12.6667 limite=10
InspecaoCamera: iniciando inspecao em x=5 y=115.667 timestamp=...
InspecaoCamera: processamento finalizado resultado=...
```

Exemplo do CSV:

```csv
timestamp,x,y,confianca
1.886e-06,0,100,0.2
0.500096,1,100.5,0.4
1.00024,2,101,0.6
2.50054,5,115.667,0.6
```

## Texto Pronto Para Usar no Relatorio

```text
Na Etapa 1 foi adotada uma arquitetura multitarefa em processo unico. As tarefas internas do robo, indicadas em azul na Figura 3 do enunciado, foram modeladas como threads C++ dentro do processo principal. A comunicacao entre tarefas ocorre por memoria compartilhada, usando buffers do tipo produtor-consumidor protegidos por mutexes e variaveis de condicao.

Essa escolha reduz a complexidade da primeira etapa e permite demonstrar diretamente os mecanismos de sincronizacao exigidos no trabalho. A comunicacao por IPC/MQTT e as interfaces graficas foram deixadas para a Etapa 2, conforme especificado no enunciado.
```

## Pontos Para Conferir Antes de Fechar a Etapa 1

- [x] Incluir figura da arquitetura detalhada.
- [x] Explicar quais tarefas viraram threads.
- [x] Explicar quais buffers foram implementados.
- [x] Mostrar o fluxo `SimulacaoSensores -> SensorBuffer -> ReconstrucaoSuperficie`.
- [x] Mostrar o fluxo `ReconstrucaoSuperficie -> SurfaceBuffer -> ColetorDados`.
- [x] Explicar uso de `mutex`.
- [x] Explicar uso de `condition_variable`.
- [x] Incluir comando de compilacao.
- [x] Incluir exemplo de saida do teste.
- [x] Informar que MQTT, IPC e GUIs ficam para a Etapa 2.
