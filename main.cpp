#include <chrono>
#include <functional>
#include <mutex>
#include <thread>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <condition_variable>
#include <queue>

void *HelloWorld(void *arg)
{
    std::cout << "Hello world!" << std::endl;
    return 0;
}

struct SensorData
{
    bool encoder;
    int lidar;
    double timestamp;
};

struct RobotCommand
{
    bool automatico;
    int sp_velocidade;
};

struct SurfacePoint
{
    double timestamp;
    double x;
    double y;
    double confidence;
};

struct SensorBuffer
{
    std::queue<SensorData> fila;
    std::mutex mutex;
    std::condition_variable dadoDisponivel;
    bool simulacaoTerminou = false;
};

struct ActuatorData {
    int aceleracao; // -100 a 100
    bool ligaCamera;
};

struct RobotState {
    bool inspecao;
    bool automatico;
    double velocidadeAtual;
    double posicaoX;
};

struct SurfacePoint {
    double timestamp;
    double x;
    double y;
    double confidence;
};


void ComandoNavegacao()
{
}

void ControleNavegacao()
{
}

void DistanciaPercorrida()
{
}

void InspecaoCamera()
{
}

void ColetorDados()
{
}


double TempoAtualSegundos()
{
    using Clock = std::chrono::steady_clock;

    static const auto inicio = Clock::now();
    const auto agora = Clock::now();
    const auto tempo = agora - inicio;

    return std::chrono::duration<double>(tempo).count();
}

void SimulacaoSensores(SensorBuffer &buffer)
{
    bool estadoEncoder = false;

    for (int i = 0; i < 10; i++)
    {
        estadoEncoder = !estadoEncoder;

        SensorData leitura;
        leitura.encoder = estadoEncoder;
        leitura.lidar = 100 + i;
        leitura.timestamp = TempoAtualSegundos();

        {
            std::lock_guard<std::mutex> trava(buffer.mutex);
            buffer.fila.push(leitura);
        }

        buffer.dadoDisponivel.notify_one();

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    {
        std::lock_guard<std::mutex> trava(buffer.mutex);
        buffer.simulacaoTerminou = true;
    }

    buffer.dadoDisponivel.notify_one();
}

void ReconstrucaoSuperficie(SensorBuffer &buffer)
{
    const int tamanhoJanela = 3;
    std::queue<int> ultimasLeituras;
    int somaLeituras = 0;

    while (true)
    {
        SensorData leitura;

        {
            std::unique_lock<std::mutex> trava(buffer.mutex);

            buffer.dadoDisponivel.wait(trava, [&buffer] {
                return !buffer.fila.empty() || buffer.simulacaoTerminou;
            });

            if (buffer.fila.empty() && buffer.simulacaoTerminou)
            {
                break;
            }

            leitura = buffer.fila.front();
            buffer.fila.pop();
        }

        ultimasLeituras.push(leitura.lidar);
        somaLeituras += leitura.lidar;

        if (ultimasLeituras.size() > tamanhoJanela)
        {
            somaLeituras -= ultimasLeituras.front();
            ultimasLeituras.pop();
        }

        double mediaMovel = static_cast<double>(somaLeituras) / ultimasLeituras.size();

        std::cout << "Sensor: encoder=" << leitura.encoder
                  << " lidar=" << leitura.lidar
                  << " media_movel=" << mediaMovel
                  << " timestamp=" << leitura.timestamp << "s"
                  << std::endl;
    }
}

int main()
{
    std::cout << "Começando..." << std::endl;

    SensorBuffer buffer;

    std::thread simulacao(SimulacaoSensores, std::ref(buffer));
    std::thread reconstrucao(ReconstrucaoSuperficie, std::ref(buffer));

    simulacao.join();
    reconstrucao.join();

    std::cout << "Fim." << std::endl;

    return 0;
}
