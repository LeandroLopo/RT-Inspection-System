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
#include <chrono>
#include <thread>

void SimulacaoSensores(SensorBuffer &sensorBuffer, EncoderBuffer &encoderBuffer){
    bool estado_encoder = false;
    for (int i = 0;  i < 10; i++){
        estado_encoder = !estado_encoder;
        
        SensorData leitura; 
        leitura.i_encoder = estado_encoder; 
        leitura.i_lidar = (i == 5) ? 140 : 100 + i;
        leitura.timestamp = TempoAtualSegundos();

        EncoderData encoder;
        encoder.i_encoder = leitura.i_encoder;
        encoder.timestamp = leitura.timestamp;

        {
            std::lock_guard<std::mutex> trava(encoderBuffer.mutex_encoder);
            encoderBuffer.fila_encoder.push(encoder);
        }

        encoderBuffer.encoder_disponivel_var.notify_one();

        {
            std::lock_guard<std::mutex> trava (sensorBuffer.mutex_sensor);
            sensorBuffer.fila_sensor.push (leitura); //inserção do dado de sensor na fila
        }

        sensorBuffer.dado_disponivel_var.notify_one(); //evita espera ocupada e acorda a thread só quando há dado novo.
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    {
        std::lock_guard<std::mutex> trava(sensorBuffer.mutex_sensor);
        sensorBuffer.finalizado = true;
    }

    sensorBuffer.dado_disponivel_var.notify_one();

    {
        std::lock_guard<std::mutex> trava(encoderBuffer.mutex_encoder);
        encoderBuffer.finalizado = true;
    }

    encoderBuffer.encoder_disponivel_var.notify_one();

}   
