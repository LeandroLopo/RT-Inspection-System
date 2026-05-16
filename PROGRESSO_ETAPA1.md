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
- media movel simples aplicada ao valor do LIDAR;
- sincronizacao usando `std::mutex`;
- espera por novos dados usando `std::condition_variable`;
- duas threads criadas no `main`.

Fluxo atual:

```text
SimulacaoSensores -> SensorBuffer -> ReconstrucaoSuperficie
```

## Arquitetura Planejada

Fluxo geral desejado:

```text
Operacao Remota
      |
      v
ComandoNavegacao
      |
      v
ControleNavegacao -> Atuadores
      |
      v
SimulacaoSensores -> SensorBuffer
      |                  |
      |                  v
      |           ReconstrucaoSuperficie -> CameraEvent
      |                  |                    |
      v                  v                    v
DistanciaPercorrida   SurfaceBuffer      InspecaoCamera
                         |
                         v
                    ColetorDados
```

Na Etapa 1, o objetivo e implementar o comportamento interno com threads e buffers. A operacao remota e a simulacao grafica podem ser simuladas por valores fixos ou impressao no terminal.

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

Ainda deve ser implementado.

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

Ainda deve ser implementado.

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

Status: ainda nao implementada.

Responsabilidade:

- consumir `SurfacePoint` do `SurfaceBuffer`;
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

### Passo 1: detectar falha no LIDAR

Adicionar anomalia na simulacao e detectar variacao brusca na reconstrucao.

Resultado esperado:

```text
Falha detectada em timestamp=...
```

### Passo 2: criar CameraEvent

Adicionar evento sincronizado entre `ReconstrucaoSuperficie` e `InspecaoCamera`.

Resultado esperado:

```text
Reconstrucao detectou falha
Camera iniciou inspecao
Camera terminou processamento
```

### Passo 3: implementar InspecaoCamera

Criar uma thread nova para camera.

Resultado esperado:

```text
Camera aguardando evento
Camera processando falha
```

### Passo 4: criar SurfaceBuffer

Adicionar buffer entre reconstrucao e coletor.

Resultado esperado:

```text
Reconstrucao produz SurfacePoint
ColetorDados consome SurfacePoint
```

### Passo 5: implementar ColetorDados

Gravar os pontos em CSV.

Resultado esperado:

```text
dados_superficie.csv
```

### Passo 6: implementar DistanciaPercorrida

Usar encoder para atualizar posicao.

Resultado esperado:

```text
Distancia percorrida: 1 m
Distancia percorrida: 2 m
```

### Passo 7: implementar comando e controle de navegacao

Criar setpoint de velocidade e controle proporcional simples.

Resultado esperado:

```text
SP velocidade=2.0
velocidade atual=...
aceleracao=...
```

### Passo 8: organizar relatorio da Etapa 1

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

- [x] Criar threads iniciais.
- [x] Criar `SensorData`.
- [x] Criar `SensorBuffer`.
- [x] Implementar produtor de sensores.
- [x] Implementar consumidor de sensores.
- [x] Usar `mutex` para proteger buffer.
- [x] Usar `condition_variable` para esperar dado novo.
- [x] Implementar media movel simples.
- [ ] Simular anomalia no LIDAR.
- [ ] Detectar falha por variacao brusca.
- [ ] Criar `CameraEvent`.
- [ ] Implementar `InspecaoCamera`.
- [ ] Criar `SurfaceBuffer`.
- [ ] Gerar `SurfacePoint`.
- [ ] Implementar `ColetorDados`.
- [ ] Gravar arquivo CSV.
- [ ] Implementar distancia percorrida usando encoder.
- [ ] Implementar comando de navegacao simulado.
- [ ] Implementar controle proporcional inicial.
- [ ] Montar figura da arquitetura.
- [ ] Escrever relatorio parcial.

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

Implementar:

```text
anomalia simulada no LIDAR
+ deteccao de falha
+ CameraEvent
+ InspecaoCamera
```

Esse incremento e importante porque demonstra dois tipos de sincronizacao:

- buffer produtor-consumidor;
- evento entre tarefas.

Esses dois mecanismos sao centrais para a Etapa 1.
