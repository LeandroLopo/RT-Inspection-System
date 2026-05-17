// Implemente aqui a tarefa ColetorDados.
//
// Responsabilidade:
// - consumir SurfacePoint do SurfaceBuffer usando condition_variable;
// - calcular ou atualizar o nivel de confianca;
// - imprimir os pontos recebidos durante os testes;
// - depois gravar os pontos em arquivo CSV.
//
// Formato sugerido do CSV:
// timestamp,x,y,confidence

#include "buffers.hpp"
#include "log.hpp"

#include <fstream>
#include <iostream>

void ColetorDados(SurfaceBuffer &surfaceBuffer)
{
    std::ofstream arquivo("surface_points.csv");
    arquivo << "timestamp,x,y,confianca\n";
    
    while (true)
    {
        SurfacePoint ponto;

        {
            std::unique_lock<std::mutex> trava(surfaceBuffer.mutex_superficie);

            surfaceBuffer.surface_point_var.wait(trava, [&surfaceBuffer] {
                return !surfaceBuffer.fila_superficie.empty() || surfaceBuffer.finalizado;
            });

            if (surfaceBuffer.fila_superficie.empty() && surfaceBuffer.finalizado)
            {
                break;
            }

            ponto = surfaceBuffer.fila_superficie.front();
            surfaceBuffer.fila_superficie.pop();
        }

        {
            std::lock_guard<std::mutex> trava(coutMutex);
            std::cout << "Coletor: timestamp=" << ponto.timestamp
                      << " x=" << ponto.x
                      << " y=" << ponto.y
                      << " confianca=" << ponto.confianca
                      << std::endl;
        }

        arquivo << ponto.timestamp << ","
                << ponto.x << ","
                << ponto.y << ","
                << ponto.confianca << "\n";
    }
}
