#pragma once

// Defina aqui estados compartilhados que nao sao naturalmente uma fila.
// Use mutex para proteger leituras e escritas desses dados.
//
// Sugestao:
// - SharedRobotState: posicao X, velocidade atual, modo automatico e estado de inspecao.
// - SharedCommand: comando atual vindo da operacao remota ou simulado na Etapa 1.
// - SharedActuatorData: aceleracao calculada e comando para ligar camera.
//
// Regra pratica:
// trave o mutex, copie o estado para uma variavel local, destrave e processe fora da regiao critica.
