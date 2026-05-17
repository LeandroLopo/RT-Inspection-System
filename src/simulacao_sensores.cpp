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
#include "buffers.hpp"
#include "time_utils.hpp"
#include "types.hpp"
#include <thread>

void SimulacaoSensores (SensorBuffer &buffer){
    bool estado_encoder = false;
    for (int i = 0;  i < 10; i++){
        estado_encoder = !estado_encoder;
        
        SensorData leitura; 
        leitura.i_encoder = estado_encoder; 
        leitura.i_lidar = 100 + i; 
        leitura.timestamp = TempoAtualSegundos();

        {
            std::lock_guard<std::mutex> trava (buffer.mutex_sensor);
            buffer.fila_sensor.push (leitura); //inserção do dado de sensor na fila
        }

        buffer.dado_disponivel_var.notify_one(); //evita espera ocupada e acorda a thread só quando há dado novo.
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    {
        std::lock_guard<std::mutex> trava (buffer.mutex_sensor);
        buffer.finalizado = true;
    }

    buffer.dado_disponivel_var.notify_one();

    

}   