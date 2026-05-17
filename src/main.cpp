// Ponto de entrada da versao modular.
//
// Implemente aqui apenas a montagem do sistema:
// - criar buffers compartilhados;
// - criar estados compartilhados;
// - iniciar as threads das tarefas;
// - chamar join() em todas as threads;
// - imprimir inicio e fim da execucao.
//
// Evite colocar a logica das tarefas neste arquivo. A logica deve ficar nos outros .cpp.


#include <iostream>
#include "time_utils.hpp"
#include "tasks.hpp"
#include <thread>

int main()
{
    SensorBuffer buffer;

    std::thread simulacao(SimulacaoSensores, std::ref(buffer));
    std::thread reconstrucao(ReconstrucaoSuperficie, std::ref(buffer));

    simulacao.join();
    reconstrucao.join();

    return 0;
}