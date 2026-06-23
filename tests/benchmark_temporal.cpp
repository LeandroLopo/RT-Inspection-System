#include "tasks.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;
using json = nlohmann::json;

struct Resumo {
    double mediaUs;
    double p95Us;
    double maximoUs;
};

Resumo Resumir(std::vector<double> amostras)
{
    std::sort(amostras.begin(), amostras.end());

    const double soma = std::accumulate(
        amostras.begin(), amostras.end(), 0.0
    );
    const std::size_t indiceP95 = static_cast<std::size_t>(
        std::ceil(amostras.size() * 0.95)
    ) - 1;

    return {
        soma / static_cast<double>(amostras.size()),
        amostras[indiceP95],
        amostras.back()
    };
}

template <typename Funcao>
Resumo MedirPorAmostra(int rodadas, int quantidade, Funcao funcao)
{
    std::vector<double> tempos;
    tempos.reserve(static_cast<std::size_t>(rodadas));

    for (int rodada = 0; rodada < rodadas; rodada++) {
        const auto inicio = Clock::now();
        funcao(quantidade);
        const auto fim = Clock::now();

        const double microssegundos =
            std::chrono::duration<double, std::micro>(fim - inicio).count();

        tempos.push_back(microssegundos / quantidade);
    }

    return Resumir(tempos);
}

Resumo MedirDistancia(int rodadas, int quantidade)
{
    return MedirPorAmostra(rodadas, quantidade, [](int n) {
        EncoderBuffer entrada;
        PositionBuffer saida;
        SharedRobotState estado;
        estado.estado.velocidade = 1.0;

        for (int i = 0; i < n; i++) {
            entrada.fila_encoder.push({i % 2 == 0, i * 0.08});
        }
        entrada.finalizado = true;

        DistanciaPercorrida(entrada, saida, estado);
    });
}

Resumo MedirReconstrucao(int rodadas, int quantidade)
{
    return MedirPorAmostra(rodadas, quantidade, [](int n) {
        SensorBuffer sensores;
        PositionBuffer posicoes;
        SurfaceBuffer csv;
        SurfaceBuffer mqtt;
        SharedRobotState estado;
        SharedActuatorData atuadores;
        SharedSystemParameters parametros;
        CameraEvent camera;

        for (int i = 0; i < n; i++) {
            sensores.fila_sensor.push({
                i % 2 == 0,
                100 + ((i % 200 == 100) ? 28 : 0),
                i * 0.08
            });
            posicoes.fila_posicao.push({i * 0.08, i * 0.08});
        }

        sensores.finalizado = true;
        posicoes.finalizado = true;

        ReconstrucaoSuperficie(
            sensores,
            posicoes,
            csv,
            mqtt,
            estado,
            atuadores,
            parametros,
            camera
        );
    });
}

Resumo MedirColetor(int rodadas, int quantidade)
{
    return MedirPorAmostra(rodadas, quantidade, [](int n) {
        SurfaceBuffer entrada;

        for (int i = 0; i < n; i++) {
            entrada.fila_superficie.push({
                i * 0.08,
                i * 0.08,
                100.0 + std::sin(i * 0.01),
                0.0
            });
        }
        entrada.finalizado = true;

        ColetorDados(entrada);
    });
}

Resumo MedirJson(int rodadas, int quantidade)
{
    return MedirPorAmostra(rodadas, quantidade, [](int n) {
        for (int i = 0; i < n; i++) {
            const json mensagem = {
                {"i_encoder", i % 2 == 0},
                {"i_lidar", 100 + i % 30},
                {"velocidade", 1.25},
                {"timestamp", i * 0.08}
            };

            const std::string payload = mensagem.dump();
            const json recebido = json::parse(payload);

            if (recebido.at("i_lidar").get<int>() < 0) {
                std::abort();
            }
        }
    });
}

double MedirCamera()
{
    CameraEvent evento;
    SharedRobotState estado;
    SharedActuatorData atuadores;

    evento.falha_detectada = true;
    evento.finalizado = true;
    evento.x = 13.0;
    evento.y = 128.0;

    const auto inicio = Clock::now();
    InspecaoCamera(evento, estado, atuadores);
    const auto fim = Clock::now();

    return std::chrono::duration<double, std::milli>(fim - inicio).count();
}

void Imprimir(const std::string &nome, const Resumo &resumo)
{
    std::cout << std::left << std::setw(28) << nome
              << std::right << std::fixed << std::setprecision(3)
              << std::setw(12) << resumo.mediaUs
              << std::setw(12) << resumo.p95Us
              << std::setw(12) << resumo.maximoUs
              << '\n';
}

} // namespace

int main()
{
    constexpr int rodadas = 20;
    constexpr int quantidade = 2000;

    std::ofstream nulo("/dev/null");
    std::streambuf *saidaOriginal = std::cout.rdbuf(nulo.rdbuf());

    const Resumo distancia = MedirDistancia(rodadas, quantidade);
    const Resumo reconstrucao = MedirReconstrucao(rodadas, quantidade);
    const Resumo coletor = MedirColetor(rodadas, quantidade);
    const Resumo json = MedirJson(rodadas, quantidade);
    const double cameraMs = MedirCamera();

    std::cout.rdbuf(saidaOriginal);

    std::cout << "Benchmark temporal (20 rodadas, 2000 amostras)\n";
    std::cout << std::left << std::setw(28) << "Trecho"
              << std::right << std::setw(12) << "media(us)"
              << std::setw(12) << "p95(us)"
              << std::setw(12) << "max(us)" << '\n';

    Imprimir("DistanciaPercorrida", distancia);
    Imprimir("ReconstrucaoSuperficie", reconstrucao);
    Imprimir("ColetorDados", coletor);
    Imprimir("JSON dump + parse", json);

    std::cout << "InspecaoCamera: " << std::fixed << std::setprecision(3)
              << cameraMs << " ms\n";

    return 0;
}
