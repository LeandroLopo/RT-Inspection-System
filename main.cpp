#include <chrono>
#include <mutex>
#include <thread>
#include <iostream>
#include <condition_variable>
#include <queue>

std::mutex coutMutex;

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

struct SurfaceBuffer
{
    std::queue<SurfacePoint> fila;
    std::mutex mutex;
    std::condition_variable dadoDisponivel;
    bool reconstrucaoTerminou = false;
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

void ColetorDados(SurfaceBuffer &buffer)
{
    while (true)
    {
        SurfacePoint ponto;

        {
            std::unique_lock<std::mutex> trava(buffer.mutex);

            buffer.dadoDisponivel.wait(trava, [&buffer] {
                return !buffer.fila.empty() || buffer.reconstrucaoTerminou;
            });

            if (buffer.fila.empty() && buffer.reconstrucaoTerminou)
            {
                break;
            }

            ponto = buffer.fila.front();
            buffer.fila.pop();
        }

        {
            std::lock_guard<std::mutex> trava(coutMutex);
            std::cout << "Coletor: timestamp=" << ponto.timestamp << "s"
                      << " x=" << ponto.x
                      << " y=" << ponto.y
                      << " confidence=" << ponto.confidence
                      << std::endl;
        }
    }
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

void ReconstrucaoSuperficie(SensorBuffer &sensorBuffer, SurfaceBuffer &surfaceBuffer)
{
    const int tamanhoJanela = 3;
    std::queue<int> ultimasLeituras;
    int somaLeituras = 0;
    bool primeiraLeituraEncoder = true;
    bool encoderAnterior = false;
    double posicaoX = 0.0;
    int quantidadePontos = 0;

    while (true)
    {
        SensorData leitura;

        {
            std::unique_lock<std::mutex> trava(sensorBuffer.mutex);

            sensorBuffer.dadoDisponivel.wait(trava, [&sensorBuffer] {
                return !sensorBuffer.fila.empty() || sensorBuffer.simulacaoTerminou;
            });

            if (sensorBuffer.fila.empty() && sensorBuffer.simulacaoTerminou)
            {
                break;
            }

            leitura = sensorBuffer.fila.front();
            sensorBuffer.fila.pop();
        }

        if (primeiraLeituraEncoder)
        {
            encoderAnterior = leitura.encoder;
            primeiraLeituraEncoder = false;
        }
        else if (leitura.encoder != encoderAnterior)
        {
            posicaoX += 1.0;
            encoderAnterior = leitura.encoder;
        }

        ultimasLeituras.push(leitura.lidar);
        somaLeituras += leitura.lidar;

        if (ultimasLeituras.size() > tamanhoJanela)
        {
            somaLeituras -= ultimasLeituras.front();
            ultimasLeituras.pop();
        }

        double mediaMovel = static_cast<double>(somaLeituras) / ultimasLeituras.size();
        quantidadePontos++;

        SurfacePoint ponto;
        ponto.timestamp = leitura.timestamp;
        ponto.x = posicaoX;
        ponto.y = mediaMovel;
        ponto.confidence = quantidadePontos < 5 ? quantidadePontos / 5.0 : 1.0;

        {
            std::lock_guard<std::mutex> trava(surfaceBuffer.mutex);
            surfaceBuffer.fila.push(ponto);
        }

        surfaceBuffer.dadoDisponivel.notify_one();

        {
            std::lock_guard<std::mutex> trava(coutMutex);
            std::cout << "Sensor: encoder=" << leitura.encoder
                      << " lidar=" << leitura.lidar
                      << " media_movel=" << mediaMovel
                      << " timestamp=" << leitura.timestamp << "s"
                      << std::endl;
        }
    }

    {
        std::lock_guard<std::mutex> trava(surfaceBuffer.mutex);
        surfaceBuffer.reconstrucaoTerminou = true;
    }

    surfaceBuffer.dadoDisponivel.notify_one();
}

int main()
{
    std::cout << "Começando..." << std::endl;

    SensorBuffer sensorBuffer;
    SurfaceBuffer surfaceBuffer;

    std::thread simulacao(SimulacaoSensores, std::ref(sensorBuffer));
    std::thread reconstrucao(ReconstrucaoSuperficie, std::ref(sensorBuffer), std::ref(surfaceBuffer));
    std::thread coletor(ColetorDados, std::ref(surfaceBuffer));

    simulacao.join();
    reconstrucao.join();
    coletor.join();

    std::cout << "Fim." << std::endl;

    return 0;
}
