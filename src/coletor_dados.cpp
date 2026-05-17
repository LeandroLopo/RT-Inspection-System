#include "buffers.hpp"
#include "log.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>

void ColetorDados(SurfaceBuffer &surfaceBuffer)
{
    std::ofstream arquivo("surface_points.csv");
    arquivo << "timestamp,x,y,confianca\n";
    std::vector<SurfacePoint> historico;

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

        int pontosProximos = 1;
        for (const SurfacePoint &anterior : historico)
        {
            const bool proximoEmX = std::abs(anterior.x - ponto.x) <= 2.0;
            const bool proximoEmY = std::abs(anterior.y - ponto.y) <= 15.0;

            if (proximoEmX && proximoEmY)
            {
                pontosProximos++;
            }
        }

        ponto.confianca = std::min(1.0, pontosProximos / 5.0);
        historico.push_back(ponto);

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
