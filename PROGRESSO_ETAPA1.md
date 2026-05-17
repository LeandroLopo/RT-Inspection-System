# Progresso da Etapa 1

Este arquivo registra o progresso e o plano de implementacao da Etapa 1 do trabalho de ATR.

Prazo da Etapa 1: 18/05/2026.

## Objetivo da Etapa 1

A Etapa 1 deve focar em:

- definir a arquitetura completa do sistema;
- implementar as tarefas principais em paralelo;
- implementar buffers compartilhados entre tarefas;
- usar mecanismos de sincronizacao como `mutex` e `condition_variable`;
- demonstrar testes de troca de dados entre tarefas;
- documentar a arquitetura e os mecanismos de sincronizacao no relatorio.

Nesta etapa ainda nao e necessario implementar:

- MQTT;
- IPC entre processos;
- interface grafica da operacao remota;
- interface grafica da simulacao;
- sistema completo de visualizacao.

Esses pontos ficam para a Etapa 2.

## Estado Atual do Codigo

O arquivo principal atual e:

```text
main.cpp
```

O codigo ja possui:

- `SensorData`: estrutura com dados simulados de sensores;
- `SensorBuffer`: buffer compartilhado entre simulacao e reconstrucao;
- `SimulacaoSensores`: tarefa produtora de dados;
- `ReconstrucaoSuperficie`: tarefa consumidora de dados;
- `SurfaceBuffer`: buffer compartilhado entre reconstrucao e coletor;
- `ColetorDados`: tarefa consumidora de pontos reconstruidos;
- media movel simples aplicada ao valor do LIDAR;
- sincronizacao usando `std::mutex`;
- espera por novos dados usando `std::condition_variable`;
- tres threads criadas no `main`.

Fluxo atual:

```text
SimulacaoSensores -> SensorBuffer -> ReconstrucaoSuperficie
ReconstrucaoSuperficie -> SurfaceBuffer -> ColetorDados
```

## Arquitetura Planejada

A arquitetura sera dividida em dois niveis:

- tarefas internas do robo, implementadas como threads dentro de um processo C++;
- sistemas externos, implementados como processos separados na Etapa 2.

Essa decisao acompanha a Figura 3 do enunciado:

- blocos azuis: tarefas em C ou C++;
- blocos verdes: tarefas em qualquer linguagem;
- setas azuis: acesso direto a memoria;
- setas verdes: IPC;
- setas vermelhas: eventos;
- setas laranjas: I/O.

Na Etapa 1, devem ser implementadas as tarefas azuis e os buffers internos. As setas verdes, vermelhas e laranjas devem ser definidas na arquitetura, mas nao precisam ser implementadas ainda.

### Decisao Principal

Na Etapa 1, o sistema tera um unico processo principal:

```text
Processo: rt_inspection_core
Linguagem: C++
```

Dentro desse processo, as tarefas em azul serao implementadas como threads:

```text
rt_inspection_core
|
+-- thread ComandoNavegacao
+-- thread ControleNavegacao
+-- thread DistanciaPercorrida
+-- thread ReconstrucaoSuperficie
+-- thread InspecaoCamera
+-- thread ColetorDados
+-- thread SimulacaoSensoresTeste
```

A thread `SimulacaoSensoresTeste` nao e uma tarefa final da Figura 3. Ela existe na Etapa 1 apenas para gerar dados falsos e permitir testar os buffers enquanto a simulacao grafica da Etapa 2 ainda nao existe.

### Por Que Usar Threads Para As Tarefas Azuis

As tarefas azuis fazem parte do sistema embarcado do robo. Elas precisam trocar dados com baixa latencia e com controle direto de sincronizacao.

Por isso, elas ficam no mesmo processo C++ e usam:

- `std::thread` para execucao paralela;
- `std::mutex` para proteger dados compartilhados;
- `std::condition_variable` para acordar tarefas quando ha dado novo;
- buffers em memoria para comunicacao produtor-consumidor.

Vantagens dessa escolha:

- implementacao mais simples para a Etapa 1;
- menor custo de comunicacao que IPC;
- facilita demonstrar mutex, variavel de condicao e regiao critica;
- combina com as setas azuis da Figura 3, que representam acesso direto a memoria.

### Por Que Usar Processos Na Etapa 2

Na Etapa 2, alguns blocos devem virar processos separados porque sao sistemas externos ao controle embarcado:

```text
Processo 1: rt_inspection_core
    C++
    Tarefas de controle, navegacao, reconstrucao, camera e coleta.

Processo 2: simulation_gui
    Python com pygame, ou outra linguagem.
    Simula navegacao, tunel, sensores e atuadores.

Processo 3: remote_operation_gui
    Python, web, terminal interativo ou outra linguagem.
    Mostra estados do robo e envia comandos do operador.

Processo 4: mqtt_broker
    Exemplo: Mosquitto.
    Faz a comunicacao publisher/subscriber entre os processos.
```

Nesse ponto entra IPC, porque processos separados nao compartilham memoria diretamente.

### Quando Fazer IPC

IPC sera implementado na Etapa 2, quando existirem pelo menos dois processos independentes.

Nao faz sentido usar IPC entre threads internas do mesmo processo. Nesse caso, e melhor usar memoria compartilhada dentro do processo com mutex e condition variable.

Uso planejado de IPC:

```text
simulation_gui <-> rt_inspection_core
remote_operation_gui <-> rt_inspection_core
rt_inspection_core <-> mqtt_broker
```

Tecnologia escolhida para IPC:

```text
MQTT
```

Justificativa:

- o enunciado exige comunicacao MQTT na Etapa 2;
- MQTT combina com o modelo publisher/subscriber mostrado na Figura 3;
- permite que simulacao, operacao remota e robo rodem como processos independentes;
- facilita testar cada componente separadamente.

### O Que Sera Thread e O Que Sera Processo

| Modulo | Etapa 1 | Etapa 2 | Justificativa |
| --- | --- | --- | --- |
| ComandoNavegacao | Thread C++ | Thread C++ | Tarefa interna do robo, periodo aproximado de 80 ms. |
| ControleNavegacao | Thread C++ | Thread C++ | Tarefa interna do robo, controle de velocidade, periodo aproximado de 80 ms. |
| DistanciaPercorrida | Thread C++ | Thread C++ | Tarefa interna do robo, leitura de encoder, periodo aproximado de 20 ms. |
| ReconstrucaoSuperficie | Thread C++ | Thread C++ | Tarefa interna do robo, filtro de media movel, periodo aproximado de 100 ms. |
| InspecaoCamera | Thread C++ | Thread C++ | Tarefa interna que fica aguardando acionamento. |
| ColetorDados | Thread C++ | Thread C++ | Consome pontos reconstruidos e prepara dados para log/MQTT. |
| SimulacaoSensoresTeste | Thread C++ temporaria | Removida ou substituida | Apenas gera dados falsos para testar a Etapa 1. |
| Simulacao de Navegacao | Nao implementada como processo | Processo separado | Bloco verde da Figura 3, pode ser Python/pygame. |
| Simulacao do Tunel | Nao implementada como processo | Processo separado | Bloco verde da Figura 3, gera perfil do teto e LIDAR. |
| GUI de Simulacao | Nao implementada | Processo separado | Interface visual, I/O laranja, Etapa 2. |
| GUI Operacao Remota | Nao implementada | Processo separado | Interface do operador, Etapa 2. |
| MQTT broker | Nao implementado | Processo separado | IPC/pub-sub entre processos. |

### Tarefas Ciclicas

As tarefas com icone de ciclo na Figura 3 devem ser implementadas como loops periodicos.

Periodos sugeridos pela figura:

```text
ControleNavegacao: 80 ms
ComandoNavegacao: 80 ms
DistanciaPercorrida: 20 ms
ReconstrucaoSuperficie: 100 ms
```

Modelo recomendado:

```cpp
auto proximaExecucao = std::chrono::steady_clock::now();

while (sistemaRodando) {
    proximaExecucao += std::chrono::milliseconds(80);

    // executar tarefa

    std::this_thread::sleep_until(proximaExecucao);
}
```

Isso e melhor que apenas `sleep_for`, porque reduz o acumulo de atraso ao longo do tempo.

### Tarefas Por Evento

Algumas tarefas nao precisam executar periodicamente o tempo todo.

Exemplos:

```text
InspecaoCamera
ColetorDados
```

`InspecaoCamera` deve ficar bloqueada ate receber um sinal de falha.

`ColetorDados` pode ficar bloqueado ate existir um novo `SurfacePoint` no `SurfaceBuffer`.

Na Etapa 1, os eventos vermelhos da figura podem ficar apenas definidos/documentados. Se forem implementados como teste antecipado, devem ser tratados como adiantamento da Etapa 2.

### Arquitetura Da Etapa 1

Fluxo implementado na Etapa 1:

```text
                    +------------------------+
                    | rt_inspection_core C++ |
                    +------------------------+
                                |
        -----------------------------------------------------
        |             |              |            |          |
        v             v              v            v          v
ComandoNav     ControleNav    Distancia     Reconstrucao   Coletor
   |                |              |              |          ^
   |                |              |              v          |
   |                |              |        SurfaceBuffer ---+
   |                |              |
   |                v              |
   |          NavigationData       |
   |                               |
   +---------- CommandData --------+

SimulacaoSensoresTeste -> SensorBuffer -> ReconstrucaoSuperficie
SimulacaoSensoresTeste -> EncoderBuffer -> DistanciaPercorrida
```

Observacao:

Na Etapa 1, `SimulacaoSensoresTeste` substitui temporariamente os blocos verdes de simulacao. Ela serve apenas para alimentar as threads azuis com dados falsos.

### Arquitetura Da Etapa 2

Fluxo planejado da Etapa 2:

```text
                         +-------------------+
                         |   mqtt_broker     |
                         |    Mosquitto      |
                         +---------+---------+
                                   |
                 ------------------+------------------
                 |                 |                 |
                 v                 v                 v
        +----------------+ +----------------+ +----------------------+
        | simulation_gui | | rt_core C++    | | remote_operation_gui |
        | Python/pygame  | | threads        | | Python/web/terminal  |
        +----------------+ +----------------+ +----------------------+

simulation_gui:
    publica sensores
    assina atuadores

rt_inspection_core:
    assina sensores e comandos
    publica atuadores, estados e pontos de superficie

remote_operation_gui:
    publica comandos
    assina estados e pontos de superficie
```

### Topicos MQTT Planejados Para A Etapa 2

Topicos da simulacao para o robo:

```text
atr/sim/sensors
```

Exemplo de payload:

```json
{
  "i_encoder": true,
  "i_lidar": 103,
  "timestamp": 12.5
}
```

Topicos do robo para a simulacao:

```text
atr/core/actuators
```

Exemplo de payload:

```json
{
  "o_aceleracao": 45,
  "o_liga_camera": false
}
```

Topicos da operacao remota para o robo:

```text
atr/remote/commands
```

Exemplo de payload:

```json
{
  "c_automatico": true,
  "c_man": false,
  "c_direita": false,
  "c_esquerda": false,
  "c_para": false,
  "j_sp_velocidade": 2,
  "limite_falha": 10.0
}
```

Topicos do robo para a operacao remota:

```text
atr/core/state
atr/core/surface
```

Exemplo de estado:

```json
{
  "e_inspecao": false,
  "e_automatico": true,
  "posicao_x": 15.0,
  "velocidade": 1.8,
  "o_aceleracao": 30
}
```

Exemplo de ponto de superficie:

```json
{
  "timestamp": 12.5,
  "x": 15.0,
  "y": 103.2,
  "confidence": 0.8
}
```

### Como Justificar No Relatorio

Texto base para o relatorio:

```text
As tarefas de controle embarcado foram implementadas como threads dentro de um unico processo C++, pois possuem forte acoplamento temporal e compartilham dados com baixa latencia. A comunicacao entre essas tarefas ocorre por buffers protegidos por mutexes e variaveis de condicao.

Os sistemas externos, como a simulacao grafica e a operacao remota, foram planejados como processos separados para a Etapa 2. A comunicacao entre processos sera realizada via MQTT, seguindo o modelo publisher/subscriber indicado na arquitetura do enunciado.
```

## Estruturas de Dados

### SensorData

Representa uma leitura dos sensores do robo.

Deve conter:

```cpp
struct SensorData {
    bool encoder;
    int lidar;
    double timestamp;
};
```

Responsabilidade:

- carregar uma amostra do encoder;
- carregar uma amostra do LIDAR;
- registrar o instante da leitura.

### RobotCommand

Representa comandos de navegacao.

Deve conter:

```cpp
struct RobotCommand {
    bool automatico;
    int sp_velocidade;
};
```

Responsabilidade:

- indicar se o robo esta em modo automatico;
- armazenar o setpoint de velocidade desejado.

### ActuatorData

Ainda deve ser implementada.

Sugestao:

```cpp
struct ActuatorData {
    int aceleracao;
    bool ligaCamera;
};
```

Responsabilidade:

- armazenar a aceleracao enviada para a simulacao;
- armazenar o comando de ligar camera.

### RobotState

Ainda deve ser implementada.

Sugestao:

```cpp
struct RobotState {
    bool inspecao;
    bool automatico;
    double velocidadeAtual;
    double posicaoX;
};
```

Responsabilidade:

- representar o estado atual do robo;
- permitir que outras tarefas saibam modo, velocidade, posicao e estado de inspecao.

### SurfacePoint

Representa um ponto reconstruido da superficie do teto.

Ja existe no codigo.

Responsabilidade:

- armazenar timestamp;
- armazenar posicao `x`;
- armazenar altura/distancia `y`;
- armazenar nivel de confianca.

## Buffers e Eventos

### SensorBuffer

Ja implementado.

Fluxo:

```text
SimulacaoSensores produz SensorData
ReconstrucaoSuperficie consome SensorData
```

Componentes:

- `std::queue<SensorData>`: fila de leituras;
- `std::mutex`: protege o acesso a fila;
- `std::condition_variable`: acorda a tarefa consumidora quando chega dado;
- `bool simulacaoTerminou`: indica fim da simulacao.

Dica:

O mutex deve ser usado somente para acessar a fila. Processamentos demorados devem acontecer fora da regiao protegida.

### SurfaceBuffer

Implementado.

Fluxo:

```text
ReconstrucaoSuperficie produz SurfacePoint
ColetorDados consome SurfacePoint
```

Deve seguir o mesmo modelo do `SensorBuffer`.

Componentes sugeridos:

```cpp
struct SurfaceBuffer {
    std::queue<SurfacePoint> fila;
    std::mutex mutex;
    std::condition_variable dadoDisponivel;
    bool reconstrucaoTerminou = false;
};
```

### CameraEvent

Ainda nao e obrigatorio na Etapa 1.

Fluxo:

```text
ReconstrucaoSuperficie detecta falha
InspecaoCamera acorda
```

Sugestao:

```cpp
struct CameraEvent {
    std::mutex mutex;
    std::condition_variable evento;
    bool falhaDetectada = false;
    bool finalizar = false;
};
```

Esse evento nao precisa ser uma fila no inicio. Ele apenas sinaliza que existe uma falha para inspecionar.

Observacao importante:

Na Figura 3, esse tipo de interacao aparece como seta vermelha, ou seja, comunicacao via evento. O enunciado diz que as setas vermelhas nao precisam ser implementadas na Etapa 1. Portanto, para a Etapa 1, basta definir esse evento na arquitetura. A implementacao pode ficar para a Etapa 2 ou ser feita como adiantamento, caso sobre tempo.

## Funcoes e Responsabilidades

### SimulacaoSensores

Status: parcialmente implementada.

Responsabilidade:

- gerar leituras falsas de encoder e LIDAR;
- alternar o estado do encoder;
- simular valores de LIDAR;
- inserir `SensorData` no `SensorBuffer`;
- acordar a tarefa `ReconstrucaoSuperficie`.

Proximas melhorias:

- simular anomalia no teto;
- simular ruido no LIDAR;
- usar posicao e velocidade simuladas;
- gerar encoder a cada 1 metro percorrido.

Dica de anomalia simples:

```cpp
if (i == 5) {
    leitura.lidar = 140;
} else {
    leitura.lidar = 100 + i;
}
```

### ReconstrucaoSuperficie

Status: parcialmente implementada.

Responsabilidade:

- consumir dados de `SensorBuffer`;
- aplicar media movel no LIDAR;
- detectar variacoes bruscas na superficie;
- gerar pontos `SurfacePoint`;
- enviar pontos para o `SurfaceBuffer`;
- sinalizar `CameraEvent` quando houver falha.

Ja implementado:

- consumo sincronizado do `SensorBuffer`;
- media movel com janela de 3 leituras.

Proximo passo:

- comparar a media atual com a media anterior;
- detectar falha quando a diferenca passar de um limite.

Exemplo de regra:

```text
se abs(mediaAtual - mediaAnterior) > limiteFalha:
    falha detectada
```

Dica:

Use inicialmente:

```cpp
const double limiteFalha = 10.0;
```

Depois esse limite pode ser configurado pela operacao remota.

### InspecaoCamera

Status: ainda nao implementada.

Responsabilidade:

- ficar bloqueada aguardando evento de falha;
- acordar quando `ReconstrucaoSuperficie` detectar anomalia;
- simular camera ligada;
- executar processamento pesado;
- voltar a esperar novo evento.

Importante:

O enunciado pede para simular processamento pesado usando CPU, nao apenas `sleep`.

Sugestao:

```cpp
double resultado = 0.0;
for (int i = 0; i < 10000000; i++) {
    resultado += std::sin(i);
}
```

### ColetorDados

Status: parcialmente implementada.

Responsabilidade:

- consumir `SurfacePoint` do `SurfaceBuffer`;
- imprimir os pontos reconstruidos recebidos;
- calcular ou atualizar nivel de confianca;
- gravar dados em arquivo `.csv`;
- futuramente disponibilizar dados via MQTT.

Formato sugerido do arquivo:

```csv
timestamp,x,y,confidence
0.5,1.0,100.2,0.5
1.0,2.0,101.0,0.6
```

Dica:

Comece com uma confianca simples:

```text
confidence = min(1.0, quantidadeDeMedicoes / 5.0)
```

Depois melhore usando proximidade entre medicoes.

### DistanciaPercorrida

Status: ainda nao implementada.

Responsabilidade:

- ler mudancas no encoder;
- contar distancia percorrida;
- atualizar posicao estimada do robo.

Regra simples:

```text
se encoderAtual != encoderAnterior:
    distancia += 1.0
```

Dica:

Como o enunciado define que o encoder troca de estado a cada metro, essa regra e suficiente para a primeira versao.

### ComandoNavegacao

Status: ainda nao implementada.

Responsabilidade:

- receber comandos do operador;
- decidir modo manual ou automatico;
- gerar setpoint de velocidade para o controlador.

Na Etapa 1:

- simular comandos por valores fixos;
- por exemplo, iniciar em automatico com velocidade 2 m/s.

Na Etapa 2:

- receber comandos pela interface remota via MQTT.

### ControleNavegacao

Status: ainda nao implementada.

Responsabilidade:

- ler setpoint de velocidade;
- comparar com velocidade atual;
- calcular aceleracao;
- enviar aceleracao para os atuadores.

Comece com controle proporcional:

```text
erro = setpoint - velocidadeAtual
aceleracao = Kp * erro
```

Depois evolua para PID:

```text
P = Kp * erro
I = I + Ki * erro * dt
D = Kd * (erro - erroAnterior) / dt
saida = P + I + D
```

Dica:

Limite a aceleracao entre -100 e 100.

## Ordem Recomendada de Implementacao

### Passo 1: criar SurfaceBuffer

Adicionar buffer entre `ReconstrucaoSuperficie` e `ColetorDados`.

Resultado esperado:

```text
Reconstrucao produz SurfacePoint
ColetorDados consome SurfacePoint
```

### Passo 2: gerar SurfacePoint

Fazer `ReconstrucaoSuperficie` transformar cada leitura filtrada em um ponto da superficie.

Resultado esperado:

```text
timestamp=...
x=...
y=...
confidence=...
```

### Passo 3: implementar ColetorDados

Fazer `ColetorDados` consumir `SurfacePoint` do `SurfaceBuffer`.

Resultado esperado:

```text
Coletor recebeu SurfacePoint
```

### Passo 4: implementar DistanciaPercorrida

Usar encoder para atualizar posicao horizontal `x`.

Resultado esperado:

```text
Distancia percorrida: 1 m
Distancia percorrida: 2 m
```

### Passo 5: implementar ComandoNavegacao simulado

Criar uma tarefa ciclica que define modo automatico/manual e setpoint de velocidade.

Resultado esperado:

```text
Modo automatico
SP velocidade=2.0
```

### Passo 6: implementar ControleNavegacao proporcional

Ler setpoint, comparar com velocidade atual e calcular aceleracao.

Resultado esperado:

```text
erro=...
aceleracao=...
```

### Passo 7: criar InspecaoCamera como tarefa azul

Criar a thread da camera. Na Etapa 1, ela pode ficar em espera controlada ou executar um teste simples, pois o evento vermelho completo nao e obrigatorio ainda.

Resultado esperado:

```text
InspecaoCamera iniciada
InspecaoCamera aguardando sinal futuro
```

### Passo 8: opcionalmente detectar falha no LIDAR

Adicionar anomalia na simulacao de teste e detectar variacao brusca na reconstrucao.

Resultado esperado:

```text
Falha detectada em timestamp=...
```

### Passo 9: opcionalmente criar CameraEvent

Adicionar evento sincronizado entre `ReconstrucaoSuperficie` e `InspecaoCamera`. Esta parte prepara a Etapa 2.

Resultado esperado:

```text
Reconstrucao detectou falha
Camera iniciou inspecao
Camera terminou processamento
```

### Passo 10: organizar relatorio da Etapa 1

Documentar:

- diagrama da arquitetura;
- lista de tarefas;
- lista de buffers;
- quais dados passam em cada buffer;
- quais mutexes protegem quais recursos;
- quais condition variables disparam quais eventos;
- resultados dos testes.

## Comando de Compilacao

Usar:

```bash
g++ -std=c++17 main.cpp -pthread -o rt_inspection
```

Se estiver compilando a partir da pasta acima:

```bash
g++ -std=c++17 RT-Inspection-System/main.cpp -pthread -o /tmp/rt_inspection
```

## Checklist da Etapa 1

- [x] Decidir arquitetura de threads e processos.
- [x] Definir que as tarefas azuis serao threads C++.
- [x] Definir que simulacao, operacao remota e MQTT serao processos da Etapa 2.
- [x] Definir IPC planejado para Etapa 2 via MQTT.
- [x] Criar threads iniciais.
- [x] Criar `SensorData`.
- [x] Criar `SensorBuffer`.
- [x] Implementar produtor de sensores.
- [x] Implementar consumidor de sensores.
- [x] Usar `mutex` para proteger buffer.
- [x] Usar `condition_variable` para esperar dado novo.
- [x] Implementar media movel simples.
- [x] Simular anomalia no LIDAR.
- [x] Detectar falha por variacao brusca.
- [x] Criar thread `InspecaoCamera` como tarefa azul, mesmo que ainda sem evento real.
- [x] Criar `CameraEvent` como item opcional ou preparacao para a Etapa 2.
- [x] Criar `SurfaceBuffer`.
- [x] Gerar `SurfacePoint`.
- [x] Implementar `ColetorDados`.
- [x] Gravar arquivo CSV.
- [x] Implementar distancia percorrida usando encoder.
- [x] Implementar comando de navegacao simulado.
- [x] Implementar controle proporcional inicial.
- [x] Montar figura da arquitetura.
- [x] Escrever relatorio parcial.

## Conceitos Importantes Para Explicar no Relatorio

### Mutex

Usado para proteger dados compartilhados entre threads.

Exemplo:

```cpp
std::lock_guard<std::mutex> trava(buffer.mutex);
buffer.fila.push(leitura);
```

Enquanto uma thread esta dentro desse trecho, outra thread nao deve acessar a mesma fila.

### Condition Variable

Usada para evitar espera ocupada.

Em vez de a thread consumidora ficar testando a fila repetidamente, ela dorme ate o produtor avisar que existe dado novo.

Exemplo:

```cpp
buffer.dadoDisponivel.wait(trava, [&buffer] {
    return !buffer.fila.empty() || buffer.simulacaoTerminou;
});
```

### Regiao Critica

E o trecho de codigo que acessa um dado compartilhado.

Regra pratica:

```text
travar antes de acessar dado compartilhado
destravar assim que terminar o acesso
processar dados fora do mutex
```

### Produtor e Consumidor

Padrao usado quando uma tarefa gera dados e outra tarefa processa esses dados.

Exemplo atual:

```text
SimulacaoSensores = produtor
ReconstrucaoSuperficie = consumidor
```

Esse mesmo padrao sera reutilizado para:

```text
ReconstrucaoSuperficie -> ColetorDados
```

## Proximo Incremento Recomendado

Implementar primeiro o caminho obrigatorio de buffers da Etapa 1:

```text
ReconstrucaoSuperficie
-> SurfaceBuffer
-> ColetorDados
```

Esse incremento demonstra o padrao produtor-consumidor entre duas tarefas azuis usando memoria compartilhada, mutex e variavel de condicao.

Depois implementar:

```text
DistanciaPercorrida
+ CommandData
+ ControleNavegacao proporcional simples
```

Implementacao opcional, se sobrar tempo:

```text
anomalia simulada no LIDAR
+ deteccao de falha
+ CameraEvent
+ InspecaoCamera
```

Essa parte opcional ja prepara as setas vermelhas de evento da Etapa 2.
