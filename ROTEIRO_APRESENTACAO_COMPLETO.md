# Roteiro Completo da Apresentação

Este documento acompanha o arquivo **Apresentação Rodrigo e Leandro.pptx** e considera o limite máximo de **8 minutos**.

## Divisão da dupla

| Parte | Responsável | Tempo sugerido |
|---|---|---:|
| Slide 1 | Leandro | 0:00–0:25 |
| Slides 2 a 6 | Rodrigo | 0:25–3:20 |
| Slides 7 a 10 | Leandro | 3:20–6:40 |
| Demonstração e fechamento | Leandro, com apoio de Rodrigo | 6:40–7:50 |

O roteiro totaliza aproximadamente **7 minutos e 50 segundos**, deixando uma margem mínima para troca de janelas.

## Preparação antes de entrar na sala

- Deixar o PowerPoint aberto no slide 1.
- Deixar o VS Code aberto com os arquivos citados pelos slides.
- Deixar um terminal na raiz do projeto.
- Confirmar que `python3 run_all.py` inicia o sistema completo.
- Fechar outros programas que possam gerar notificações.
- Ensaiar as transições entre Rodrigo e Leandro.
- Não tentar explicar todas as linhas: mostrar apenas a estrutura que sustenta cada decisão.

---

# Slide 1 — Sistema de Inspeção Automática de Túneis

**Responsável:** Leandro  
**Tempo:** 0:00–0:25

## Objetivo do slide

Apresentar o projeto, os integrantes e a divisão geral do sistema.

## Fala sugerida

> Boa tarde. Nós somos Leandro Lopo e Rodrigo Martins. Nosso trabalho é um sistema de inspeção automática de túneis, desenvolvido com um núcleo concorrente em C++, duas interfaces em Python e comunicação entre processos por MQTT. O robô simulado percorre o túnel, mede o teto com LIDAR, reconstrói a superfície e aciona uma inspeção por câmera quando detecta uma anomalia. O Rodrigo vai começar apresentando o funcionamento geral e as tarefas periódicas do sistema.

## Ponto principal

- Não aprofundar a arquitetura neste slide.
- Usar a última frase para passar naturalmente a fala ao Rodrigo.

---

# Slide 2 — Visão Geral do Sistema de Inspeção

**Responsável:** Rodrigo  
**Tempo:** 0:25–0:55

## Objetivo do slide

Explicar o problema resolvido e o fluxo completo do sistema.

## Fala sugerida

> O problema consiste em inspecionar o teto de um túnel sem expor uma pessoa ao ambiente. Na simulação, o robô recebe aceleração do núcleo e devolve encoder, velocidade e distância do LIDAR. O núcleo usa esses sensores para controlar a navegação, calcular a posição e reconstruir o perfil do teto. Quando uma variação severa é detectada, a câmera é acionada e a velocidade é limitada. Em paralelo, o estado e os pontos reconstruídos são enviados para a interface remota.

## Fluxo para apontar no slide

```text
Simulação -> sensores -> núcleo C++ -> atuadores
Operação remota -> comandos -> núcleo C++
Núcleo C++ -> estado e superfície -> operação remota
```

## Relação com a disciplina

- processos independentes;
- programação concorrente;
- comunicação entre processos;
- tarefas periódicas e orientadas a evento.

---

# Slide 3 — Fundamentos Temporais e Sincronização de Log

**Responsável:** Rodrigo  
**Tempo:** 0:55–1:25

**Arquivos:** [`src/time_utils.cpp`](src/time_utils.cpp) e [`src/log.cpp`](src/log.cpp)

## Fala sugerida

> Para timestamps usamos `std::chrono::steady_clock`. Esse relógio é monotônico, então não retrocede se o relógio do sistema operacional for corrigido. Guardamos um instante inicial estático e retornamos sempre o tempo decorrido em segundos. Para os logs, criamos um `coutMutex` global. Como várias threads escrevem no terminal, cada impressão entra em uma seção crítica protegida por `lock_guard`, evitando que mensagens diferentes fiquem misturadas.

## Mostrar no código

```cpp
using Clock = std::chrono::steady_clock;
static const auto inicio = Clock::now();
```

```cpp
std::mutex coutMutex;
```

## Ponto técnico importante

`steady_clock` evita alterações do relógio de parede, mas não elimina sozinho atrasos de escalonamento. A periodicidade das tarefas é tratada com `sleep_until`.

---

# Slide 4 — Geração de Dados e Interface de Comando

**Responsável:** Rodrigo  
**Tempo:** 1:25–2:00

**Arquivos:** [`src/simulacao_sensores.cpp`](src/simulacao_sensores.cpp) e [`src/comando_navegacao.cpp`](src/comando_navegacao.cpp)

## Fala sugerida

> `SimulacaoSensores` foi criada na primeira etapa para testar o núcleo sem depender da interface gráfica. Ela alterna o encoder, gera leituras do LIDAR e injeta uma anomalia no ciclo 5. Na versão completa, essa função foi substituída no fluxo principal pela simulação gráfica que publica sensores via MQTT. Já `ComandoNavegacao` executa a cada 80 milissegundos, copia os comandos compartilhados sob mutex e determina se o robô está em modo automático ou manual.

## Mostrar no código

```cpp
leitura.i_lidar = (i == 5) ? 140 : 100 + i;
```

```cpp
proximaExecucao += std::chrono::milliseconds(80);
std::this_thread::sleep_until(proximaExecucao);
```

## Decisão de implementação

> Usamos `sleep_until` com um próximo instante absoluto. Assim, o tempo gasto no corpo da tarefa não é simplesmente somado ao período a cada ciclo, reduzindo a deriva acumulada em comparação com um `sleep_for` ao final de cada iteração.

---

# Slide 5 — Malha de Controle de Navegação

**Responsável:** Rodrigo  
**Tempo:** 2:00–2:40

**Arquivo:** [`src/controle_navegacao.cpp`](src/controle_navegacao.cpp)

## Fala sugerida

> A tarefa de controle também é periódica, com período de 80 milissegundos. No modo automático, calculamos o erro entre o setpoint e a velocidade medida. Em seguida atualizamos os termos proporcional, integral e derivativo. A integral é limitada entre menos 20 e 20 para reduzir o efeito de `windup`, e a saída final é limitada entre menos 100 e 100 por representar o percentual de aceleração aceito pelo atuador. Durante uma inspeção, o setpoint é limitado a 1 metro por segundo. No modo manual, os comandos produzem acelerações fixas para direita, esquerda ou parada.

## Mostrar no código

```cpp
const double kp = 25.0;
const double ki = 1.5;
const double kd = 4.0;
```

```cpp
integralErro = std::clamp(
    integralErro + erro * periodoSegundos,
    -limiteIntegral,
    limiteIntegral
);
```

```cpp
aceleracao = static_cast<int>(
    std::clamp(saidaPid, -100.0, 100.0)
);
```

## Relação com tempo real

- leitura periódica do estado;
- cálculo com tempo limitado e previsível;
- escrita protegida do atuador compartilhado;
- período comum de 80 ms com simulação e publicação MQTT.

---

# Slide 6 — Roteamento de Telemetria Contínua via MQTT

**Responsável:** Rodrigo  
**Tempo:** 2:40–3:20

**Arquivos:** [`src/mqtt_actuator_publisher.cpp`](src/mqtt_actuator_publisher.cpp) e [`src/mqtt_state_publisher.cpp`](src/mqtt_state_publisher.cpp)

## Fala sugerida

> A comunicação externa usa um broker Mosquitto em `localhost`, porta 1883. Essas duas tarefas são publicadores periódicos. A cada 80 milissegundos elas copiam o estado compartilhado sob mutex, liberam a trava, montam uma mensagem JSON e publicam no tópico correspondente. Atuadores são enviados para a simulação, enquanto estado, velocidade, inspeção e limite de falha são enviados para a operação remota. O `mosquitto_loop` com timeout curto processa a rede sem manter a tarefa bloqueada por um intervalo longo.

## Mostrar no código

```cpp
mosquitto_connect(cliente, "localhost", 1883, 60);
```

```cpp
mosquitto_loop(cliente, 10, 1);
```

## Tópicos citados

```text
atr/core/actuators
atr/core/state
```

## Transição para Leandro

> Até aqui mostramos as tarefas cíclicas e a comunicação externa. Agora o Leandro vai mostrar como elas são instanciadas e sincronizadas no núcleo concorrente.

---

# Slide 7 — Arquitetura Concorrente e Gestão de Memória

**Responsável:** Leandro  
**Tempo:** 3:20–4:10

**Arquivo:** [`src/main.cpp`](src/main.cpp)

## Fala sugerida

> O `main.cpp` funciona como o orquestrador do núcleo. Primeiro inicializamos a biblioteca Mosquitto e criamos todos os buffers, estados compartilhados e eventos. Esses objetos permanecem vivos durante toda a execução. Depois criamos onze threads e injetamos as dependências por referência com `std::ref`. Isso evita cópias dos buffers e garante que produtores e consumidores acessem as mesmas estruturas protegidas.

> Nos blocos de inicialização usamos escopos curtos com `std::lock_guard`. Esse padrão segue RAII: o mutex é liberado automaticamente ao sair do escopo, inclusive se ocorrer uma exceção. Isso reduz o risco de esquecer um `unlock`. Ele não elimina todos os deadlocks; por isso também mantemos as seções críticas pequenas e normalmente adquirimos apenas um mutex por vez.

> Ao final, o `main` faz `join` primeiro nas tarefas dependentes dos sensores. Depois altera `sistema_rodando` para falso e encerra as tarefas periódicas e MQTT restantes de forma coordenada.

## Mostrar no código

```cpp
std::thread controle(
    ControleNavegacao,
    std::ref(sharedCommand),
    std::ref(robotState),
    std::ref(sharedActuatorData),
    std::ref(systemControl)
);
```

```cpp
{
    std::lock_guard<std::mutex> trava(robotState.mutex_estado);
    robotState.estado.velocidade = 0.0;
}
```

## Conceitos da disciplina

- threads e memória compartilhada;
- injeção de dependências por referência;
- seção crítica e exclusão mútua;
- RAII para aquisição e liberação de recursos;
- encerramento coordenado com `join`.

---

# Slide 8 — Pipeline de Dados e Filtragem Digital

**Responsável:** Leandro  
**Tempo:** 4:10–5:10

**Arquivos:** [`src/distancia_percorrida.cpp`](src/distancia_percorrida.cpp) e [`src/reconstrucao_superficie.cpp`](src/reconstrucao_superficie.cpp)

## Fala sugerida

> O pipeline usa o padrão produtor-consumidor. O receptor MQTT insere leituras em filas de sensor e encoder. `DistanciaPercorrida` espera no `EncoderBuffer` usando `condition_variable`. A thread fica bloqueada sem consumir CPU e acorda quando há dado novo ou finalização. Como o encoder muda de estado a cada metro, detectamos a borda e atualizamos a posição conforme o sinal da velocidade.

> A reconstrução espera por uma leitura de sensor e pela posição correspondente. Mantemos uma fila com as três últimas leituras do LIDAR e calculamos uma média móvel online. Isso reduz o ruído antes de gerar o ponto de superfície com timestamp, `x` e `y`.

> A detecção compara a leitura atual com a anterior. Se a variação superar o limite configurado pela operação remota, atualizamos o estado de inspeção, ligamos a câmera e usamos `notify_one` para acordar a única thread consumidora do evento. Na finalização usamos `notify_all`, pois todas as tarefas eventualmente bloqueadas precisam ser liberadas.

## Mostrar no código

```cpp
encoderBuffer.encoder_disponivel_var.wait(
    trava,
    [&encoderBuffer] {
        return !encoderBuffer.fila_encoder.empty()
            || encoderBuffer.finalizado;
    }
);
```

```cpp
const std::size_t tamanhoJanela = 3;
```

```cpp
if (variacao > limiteFalha) {
    cameraEvent.camera_event_var.notify_one();
}
```

## Decisões de sincronização

- `unique_lock` é usado no `wait` porque a variável de condição precisa liberar e readquirir o mutex;
- `lock_guard` é usado nas seções críticas simples;
- a condição do `wait` protege contra despertares espúrios;
- flags `finalizado` evitam que consumidores fiquem bloqueados no encerramento.

---

# Slide 9 — Carga Computacional e Persistência de Dados

**Responsável:** Leandro  
**Tempo:** 5:10–5:55

**Arquivos:** [`src/inspecao_camera.cpp`](src/inspecao_camera.cpp) e [`src/coletor_dados.cpp`](src/coletor_dados.cpp)

## Fala sugerida

> A tarefa da câmera também é orientada a evento. Ela permanece bloqueada em uma variável de condição até a reconstrução detectar uma falha. Quando acorda, copia os dados do evento, libera o mutex e executa durante aproximadamente dois segundos um laço com operações trigonométricas. O resultado é acumulado e usado no log, garantindo trabalho computacional real e simulando uma inferência de visão sem bloquear os outros mutexes.

> Paralelamente, o coletor consome os pontos reconstruídos e grava `surface_points.csv`. Para cada ponto, ele conta medições anteriores próximas em até 2 metros no eixo `x` e 15 centímetros no eixo `y`. A confiança cresce em passos de 0,2 até o máximo de 1. Dessa forma, a análise de confiança acontece online, antes da persistência em disco.

## Mostrar no código

```cpp
while (std::chrono::steady_clock::now() < fimProcessamento) {
    acumulador += std::sin(valor) * std::cos(valor * 0.5);
    iteracoes++;
}
```

```cpp
ponto.confianca = std::min(1.0, pontosProximos / 5.0);
```

## Relação com escalonabilidade

> Essa tarefa de câmera representa uma carga CPU-bound concorrendo com tarefas periódicas. O coletor também cresce em custo conforme aumenta o histórico, pois compara cada novo ponto com os anteriores. Esses são os principais módulos a observar em uma análise futura de tempo de resposta e escalabilidade.

---

# Slide 10 — Comunicação Assíncrona e Resiliência de Rede

**Responsável:** Leandro  
**Tempo:** 5:55–6:40

**Arquivos:**

- [`src/mqtt_sensor_subscriber.cpp`](src/mqtt_sensor_subscriber.cpp)
- [`src/mqtt_command_subscriber.cpp`](src/mqtt_command_subscriber.cpp)
- [`src/mqtt_surface_publisher.cpp`](src/mqtt_surface_publisher.cpp)

## Fala sugerida

> Os subscribers usam callbacks da `libmosquitto`. Quando uma mensagem chega, validamos ponteiro e payload, fazemos o parsing JSON dentro de `try-catch` e só então atualizamos o estado compartilhado. Uma mensagem inválida gera log de erro, mas não derruba a aplicação. Os valores enviados pela operação remota também são limitados com `std::clamp`, como o setpoint entre zero e três e o limite de falha entre um e cinquenta.

> Na saída da reconstrução usamos um `fan-out` local: cada ponto é copiado para dois buffers independentes. Um é consumido pelo coletor de CSV e o outro pelo publicador MQTT da superfície. Assim, persistência e telemetria têm consumidores separados e uma operação não mantém o mutex da outra durante I/O.

> A comunicação foi configurada com QoS zero e broker local, priorizando simplicidade e baixa latência para a demonstração. Não implementamos TLS ou autenticação, então o ponto forte aqui é robustez de parsing e desacoplamento, e não segurança criptográfica.

## Mostrar no código

```cpp
try {
    const json mensagem = json::parse(payload);
    // atualiza os estados
}
catch (const std::exception &erro) {
    // registra o erro sem encerrar o processo
}
```

```cpp
surfaceBuffer.fila_superficie.push(ponto);
remoteSurfaceBuffer.fila_superficie.push(ponto);
```

## Encerramento da parte teórica

> Com isso fechamos o caminho completo: dados entram via MQTT, são processados por tarefas sincronizadas, geram controle, inspeção e persistência, e retornam como telemetria. Agora vamos mostrar esse fluxo funcionando.

---

# Demonstração prática

**Responsável principal:** Leandro  
**Apoio:** Rodrigo  
**Tempo:** 6:40–7:35

## Comando recomendado

```bash
python3 run_all.py
```

O script compila o núcleo, verifica ou inicia o broker Mosquitto e abre o núcleo, a simulação e a operação remota.

## Fala durante a demonstração

> À esquerda temos a simulação física e à direita a operação remota. A simulação recebe a aceleração calculada pelo PID e publica encoder, LIDAR e velocidade. O núcleo reconstrói a superfície e publica os pontos para o gráfico remoto. Quando o robô chega à região de falha, a leitura do LIDAR varia acima do limite, a câmera fica ativa e a velocidade é reduzida durante a inspeção.

## O que mostrar

1. Status MQTT conectado nas duas interfaces.
2. Robô acelerando no modo automático.
3. Posição e velocidade mudando na operação remota.
4. Gráfico da superfície sendo construído.
5. Câmera ativada na região de falha.
6. Arquivo `surface_points.csv`, somente se ainda houver tempo.

## Plano de contingência

Se `run_all.py` falhar, usar três terminais já preparados:

```bash
make run
python3 simulation/simulation_gui.py
python3 remote_operation/remote_gui.py
```

Se a demonstração gráfica não abrir, executar o teste curto:

```bash
python3 simulation/simulation_test.py
```

e mostrar os logs do núcleo e o CSV gerado.

---

# Fechamento

**Responsável:** Leandro  
**Tempo:** 7:35–7:50

## Fala sugerida

> Em resumo, o projeto aplica os principais conteúdos da disciplina: threads, tarefas periódicas, memória compartilhada, seções críticas, mutexes, variáveis de condição, produtor-consumidor e IPC por MQTT. A separação entre núcleo, simulação e operação remota permitiu testar cada parte de forma independente e integrar o sistema completo com responsabilidades bem definidas.

---

# Correções recomendadas no PowerPoint

Antes da apresentação, convém ajustar quatro expressões para que o slide corresponda ao código atual.

1. **Slide 3:** trocar “evitar Drift” por “evitar alterações do relógio de parede”. `steady_clock` é monotônico, mas não elimina atrasos de escalonamento.
2. **Slide 7:** trocar “sem risco de Deadlocks” por “liberação automática do mutex”. RAII reduz erros de liberação, mas não impede todos os deadlocks.
3. **Slide 8:** trocar “evento em broadcast” por “evento com `notify_one`”. O código usa `notify_all` apenas na finalização.
4. **Slide 9:** remover a referência a `volatile`. O código atual mantém um acumulador usado no log.
5. **Slide 10:** trocar “Edge Security” por “Resiliência de parsing” e “nuvem” por “operação remota”. O projeto não implementa TLS, autenticação ou serviço em nuvem.

---

# Perguntas prováveis e respostas curtas

## Por que `condition_variable` em vez de polling?

Porque a thread fica bloqueada sem consumir CPU e só acorda quando existe dado ou quando o buffer é finalizado.

## Por que `unique_lock` no `wait`?

Porque a variável de condição precisa liberar o mutex enquanto espera e readquiri-lo antes de retornar. `lock_guard` não oferece essa flexibilidade.

## O `lock_guard` impede deadlock?

Não. Ele garante liberação automática do mutex. Evitamos deadlocks mantendo escopos curtos e evitando adquirir vários mutexes simultaneamente.

## Por que usar duas filas de superfície?

Para separar os consumidores: uma fila alimenta o CSV e outra alimenta o publicador MQTT. Assim cada consumidor retira dados de sua própria fila.

## Por que QoS 0 no MQTT?

Porque o sistema prioriza baixa latência e publica dados com alta frequência. Para uma aplicação real crítica, seria necessário avaliar QoS maior, persistência, autenticação e TLS.

## O sistema é hard real-time?

Não. Ele aplica conceitos de tempo real e tarefas periódicas, mas roda em Linux comum sem escalonador de tempo real, prioridades configuradas ou análise formal de pior caso. Portanto, é um sistema de tempo real por melhor esforço.

## Como a câmera afeta o sistema?

Ela cria carga de CPU por aproximadamente dois segundos, mas executa em uma thread separada e não mantém mutexes durante o processamento pesado.

## Onde há seções críticas?

Nos buffers, no estado do robô, nos comandos, nos atuadores, nos parâmetros do sistema, no controle de finalização e no terminal compartilhado.

## Como o encerramento evita threads bloqueadas?

Os buffers têm uma flag `finalizado`. Ao encerrá-los, o produtor chama `notify_all`, permitindo que consumidores acordem, verifiquem a condição e saiam do laço.

## Qual foi a principal decisão arquitetural?

Separar as funções em tarefas concorrentes no núcleo e usar MQTT para desacoplar o núcleo C++ das interfaces Python.

---

# Resumo da responsabilidade individual

## Leandro

- apresentação geral no slide 1;
- criação e coordenação das threads no `main.cpp`;
- buffers, sincronização e pipeline produtor-consumidor;
- reconstrução, evento de câmera e coleta;
- comunicação MQTT de entrada, comandos e superfície;
- demonstração e fechamento.

## Rodrigo

- visão geral funcional;
- temporização e sincronização dos logs;
- simulador inicial e comandos de navegação;
- controlador PID e limitações;
- publicação periódica de atuadores e estado via MQTT.

Cada integrante deve conhecer também o fluxo completo, porque as perguntas do professor podem cruzar os limites dessa divisão.
