# Roteiro de Apresentacao do Projeto

> **Duracao maxima:** 8 minutos  
> **Duracao planejada:** 7 minutos e 30 segundos

O roteiro deixa uma pequena margem para abrir ou fechar janelas e responder a interrupcoes durante a apresentacao.

## 0:00 - 0:40 | Abertura e objetivo

**Pessoa 1**

> Nosso projeto implementa um sistema de inspecao automatica de tuneis com tres processos: o nucleo em C++, a simulacao grafica em Python/Pygame e a operacao remota tambem em Python/Pygame. A comunicacao entre os processos e feita por MQTT. No nucleo, as tarefas rodam em paralelo usando `std::thread`, buffers compartilhados, `std::mutex` e `std::condition_variable`.

### Mostrar rapidamente

- [`README.md`](README.md)
- Arquitetura geral ou terminal com os comandos:

```bash
make run
python3 simulation/simulation_gui.py
python3 remote_operation/remote_gui.py
```

Alternativamente:

```bash
python3 run_all.py
```

---

## 0:40 - 1:40 | Estrutura geral do codigo

**Pessoa 1 abre:** [`src/main.cpp`](src/main.cpp)

### Explicacao

> O `main.cpp` e o orquestrador do nucleo. Ele inicializa a biblioteca Mosquitto, cria os buffers compartilhados, os estados globais protegidos por mutex e depois sobe cada tarefa como uma thread independente.

### Mostrar as threads

```cpp
std::thread comando(...)
std::thread controle(...)
std::thread sensoresMqtt(...)
std::thread comandosMqtt(...)
std::thread atuadoresMqtt(...)
std::thread estadoMqtt(...)
std::thread distancia(...)
std::thread reconstrucao(...)
std::thread coletor(...)
std::thread superficieMqtt(...)
std::thread camera(...)
```

### Conexao com a disciplina

> Aqui usamos diretamente o conteudo de processos e threads, programacao concorrente e divisao em tarefas periodicas. Cada tarefa tem uma responsabilidade isolada, o que facilita escalonamento e analise de tempo de resposta.

---

## 1:40 - 2:40 | Buffers, mutex e variaveis de condicao

**Pessoa 2 abre:**

- [`include/buffers.hpp`](include/buffers.hpp)
- [`include/shared_state.hpp`](include/shared_state.hpp)

### Explicacao

> As filas entre tarefas seguem o padrao produtor-consumidor. Por exemplo, o receptor MQTT de sensores produz dados no `SensorBuffer`, e a reconstrucao de superficie consome esses dados. Para evitar condicao de corrida, cada fila tem um `std::mutex`. Para evitar polling ativo, usamos `std::condition_variable`.

### Ponto importante para o professor

> A decisao de usar `condition_variable` em vez de ficar verificando a fila em loop reduz o uso de CPU e e mais adequada para automacao em tempo real, porque a tarefa consumidora acorda quando ha um dado novo ou uma sinalizacao de finalizacao.

### Mostrar o padrao

- `std::unique_lock`
- `.wait(...)`
- `notify_one()`
- `notify_all()`
- flags `finalizado`

### Conexao com a disciplina

- secao critica;
- mutex;
- produtor-consumidor;
- monitores e variaveis de condicao;
- sincronizacao entre threads.

---

## 2:40 - 3:40 | Comunicacao MQTT

**Pessoa 1 abre:**

- [`include/mqtt_topics.hpp`](include/mqtt_topics.hpp)
- [`src/mqtt_sensor_subscriber.cpp`](src/mqtt_sensor_subscriber.cpp)
- [`src/mqtt_actuator_publisher.cpp`](src/mqtt_actuator_publisher.cpp)

### Explicacao

> O MQTT foi usado como mecanismo de IPC entre processos. O broker roda em `localhost:1883`. O nucleo C++ usa `libmosquitto`, e as interfaces Python usam `paho-mqtt`.

### Topicos principais

```text
atr/sim/sensors       simulacao -> nucleo
atr/core/actuators    nucleo -> simulacao
atr/remote/commands   operacao remota -> nucleo
atr/core/state        nucleo -> operacao remota
atr/core/surface      nucleo -> operacao remota
```

### Formato JSON

> As mensagens sao JSON para facilitar a depuracao e a integracao entre C++ e Python. Por exemplo, a simulacao envia `i_encoder`, `i_lidar`, `velocidade` e `timestamp`. O nucleo devolve `o_aceleracao` e `o_liga_camera`.

### Conexao com a disciplina

- IPC;
- comunicacao assincrona;
- sockets e protocolo MQTT;
- I/O externo separado do nucleo de controle.

---

## 3:40 - 4:50 | Tarefas de navegacao e controle

**Pessoa 2 abre:**

- [`src/comando_navegacao.cpp`](src/comando_navegacao.cpp)
- [`src/controle_navegacao.cpp`](src/controle_navegacao.cpp)

### Explicacao

> `ComandoNavegacao` interpreta o modo automatico ou manual recebido pela operacao remota. Ele atualiza o estado `e_automatico`, que e mostrado na interface.

> `ControleNavegacao` e uma tarefa periodica de 80 ms. Ela implementa um controlador PID simples para transformar o setpoint de velocidade em `o_aceleracao`, limitado entre -100 e 100%.

### Mostrar

```cpp
kp, ki, kd
periodoSegundos = 0.08
std::this_thread::sleep_until(proximaExecucao)
```

### Ponto de tempo real

> Usamos `sleep_until` em vez de `sleep_for` para manter uma referencia periodica mais estavel. Isso se conecta ao conteudo de tarefas ciclicas, temporizadores e escalonamento.

### Comportamento durante a inspecao

> Quando `e_inspecao` esta ativo, o setpoint e limitado a 1 m/s, simulando navegacao mais cautelosa durante a analise da falha.

---

## 4:50 - 6:00 | Sensores, reconstrucao e inspecao

**Pessoa 1 abre:**

- [`src/distancia_percorrida.cpp`](src/distancia_percorrida.cpp)
- [`src/reconstrucao_superficie.cpp`](src/reconstrucao_superficie.cpp)
- [`src/inspecao_camera.cpp`](src/inspecao_camera.cpp)

### Explicacao

> `DistanciaPercorrida` consome o encoder. Como o encoder alterna o estado a cada metro, a tarefa detecta bordas e incrementa a posicao `x`.

> `ReconstrucaoSuperficie` junta posicao e LIDAR. Ela aplica uma media movel de janela 3 para reduzir ruido e gera pontos `(x, y)` da superficie.

### Mostrar

```cpp
std::queue<int> ultimasLeituras;
mediaMovel
variacao > limiteFalha
```

### Deteccao da falha

> A falha e detectada quando a variacao entre leituras passa do limite configuravel pela operacao remota. Quando isso acontece, a tarefa liga `e_inspecao`, aciona `o_liga_camera` e notifica a thread da camera.

**Pessoa 2 complementa em `inspecao_camera.cpp`:**

> A camera espera por evento usando `condition_variable`. Quando acionada, executa processamento pesado por cerca de 2 segundos, usando CPU de verdade, e nao apenas `sleep`, como pedido no enunciado.

### Conexao com a disciplina

- eventos;
- variaveis de condicao;
- tarefa bloqueada aguardando sinal;
- processamento pesado e impacto no escalonamento.

---

## 6:00 - 6:40 | Coletor e operacao remota

**Pessoa 2 abre:**

- [`src/coletor_dados.cpp`](src/coletor_dados.cpp)
- [`remote_operation/remote_gui.py`](remote_operation/remote_gui.py)

### Explicacao

> O coletor grava `surface_points.csv` com timestamp, posicao, distancia medida e confianca. A confianca aumenta conforme ha medicoes proximas, sendo calculada online durante a coleta.

> A operacao remota mostra modo, posicao, velocidade, camera, inspecao, aceleracao e grafico da superficie reconstruida. Ela tambem permite mudar o modo manual ou automatico, o setpoint e o limite de falha.

---

## 6:40 - 7:40 | Demonstracao pratica

### Ordem ideal

```bash
python3 run_all.py
```

Se preferir controlar os processos separadamente:

```bash
make run
python3 simulation/simulation_gui.py
python3 remote_operation/remote_gui.py
```

### Fala durante a demonstracao

> Aqui a simulacao publica sensores via MQTT. O nucleo recebe, calcula o controle e publica os atuadores. A interface remota recebe o estado e a superficie reconstruida. Quando o robo entra na regiao da falha, o LIDAR muda, a reconstrucao detecta uma variacao severa, a camera liga e o robo reduz a velocidade.

### Mostrar rapidamente

- robo andando na simulacao;
- grafico aparecendo na operacao remota;
- camera ligando na regiao da falha;
- alteracao do limite com `[` e `]`;
- modo manual com `M`, setas e espaco, se houver tempo.

---

## 7:40 - 8:00 | Fechamento

**Pessoa 1**

> Em resumo, o projeto aplica os principais topicos da disciplina: threads, secoes criticas, mutexes, variaveis de condicao, produtor-consumidor, tarefas periodicas, comunicacao assincrona via MQTT e integracao entre processos. A separacao em tarefas facilita entender o fluxo de tempo real e isolar responsabilidades.

**Pessoa 2**

> As principais decisoes foram usar MQTT para desacoplar os processos, mutexes para proteger estados compartilhados, variaveis de condicao para evitar polling e tarefas periodicas de 80 ms para controle, publicacao de estado e atuadores.

---

## Divisao sugerida entre a dupla

### Pessoa 1

- arquitetura geral;
- `main.cpp`;
- MQTT;
- reconstrucao e deteccao;
- demonstracao inicial.

### Pessoa 2

- buffers e sincronizacao;
- controle PID;
- camera e coletor;
- operacao remota;
- detalhes de sincronizacao nas respostas.

---

## Perguntas provaveis do professor

### 1. Por que usar `condition_variable`?

Para bloquear a thread consumidora ate haver um dado novo, evitando polling e reduzindo o uso de CPU.

### 2. Onde ha secao critica?

Nos acessos aos buffers e estados compartilhados: `SensorBuffer`, `EncoderBuffer`, `PositionBuffer`, `SurfaceBuffer`, `SharedRobotState`, `SharedCommand` e `SharedActuatorData`.

### 3. Qual e o papel do MQTT?

Fazer IPC entre processos independentes: nucleo C++, simulacao Python e operacao remota Python.

### 4. O que acontece quando uma falha e detectada?

A reconstrucao identifica uma variacao severa no LIDAR, ativa `e_inspecao`, liga `o_liga_camera`, limita a velocidade e notifica a tarefa da camera.

### 5. Onde entra tempo real?

Nas tarefas periodicas com periodo de 80 ms, no controle de velocidade, na publicacao MQTT recorrente e na preocupacao com bloqueios, sincronizacao e tempo de resposta.

### 6. Por que separar o sistema em varias threads?

Porque cada tarefa tem frequencia, responsabilidade e bloqueios proprios. Isso aproxima o modelo de um sistema embarcado multitarefa.
