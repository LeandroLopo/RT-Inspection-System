#pragma once

#include <mutex>

// Declare aqui uma funcao ou um mutex global para proteger saidas no terminal.
//
// Motivo:
// varias threads podem escrever no std::cout ao mesmo tempo, misturando as linhas.
//
// Sugestao:
// - criar um std::mutex compartilhado para logs;
// - ou criar uma funcao LogLinha(...) que trave o mutex antes de imprimir.

extern std::mutex coutMutex;
