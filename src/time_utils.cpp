// Implemente aqui as funcoes de tempo declaradas em include/time_utils.hpp.
//
// Primeiro passo recomendado:
// criar TempoAtualSegundos() usando std::chrono::steady_clock.
//
// Essa funcao sera usada para preencher timestamps em SensorData e SurfacePoint.

#include "time_utils.hpp"

#include <chrono>

double TempoAtualSegundos()
{
    using Clock = std::chrono::steady_clock;

    static const auto inicio = Clock::now();
    const auto agora = Clock::now();
    const auto tempoDecorrido = agora - inicio;

    return std::chrono::duration<double>(tempoDecorrido).count();
}
