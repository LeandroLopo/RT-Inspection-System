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
    SensorBuffer sensorBuffer;
    SurfaceBuffer surfaceBuffer;

    std::thread simulacao(SimulacaoSensores, std::ref(sensorBuffer));
    std::thread reconstrucao(ReconstrucaoSuperficie, std::ref(sensorBuffer), std::ref(surfaceBuffer));
    std::thread coletor(ColetorDados, std::ref(surfaceBuffer));

    simulacao.join();
    reconstrucao.join();
    coletor.join();

    return 0;
}
