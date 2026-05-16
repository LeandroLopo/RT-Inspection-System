// Implemente aqui a tarefa SimulacaoSensores.
//
// Responsabilidade:
// - gerar leituras falsas de encoder e LIDAR;
// - alternar o encoder como bool;
// - inserir SensorData no SensorBuffer;
// - opcionalmente inserir dados de encoder em um EncoderBuffer separado;
// - chamar notify_one() para acordar as tarefas consumidoras;
// - sinalizar fim da simulacao ao terminar.
//
// Comece com 10 leituras e um sleep_for de 500 ms entre elas.
