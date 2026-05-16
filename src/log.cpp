// Implemente aqui o suporte a log seguro entre threads.
//
// Problema a resolver:
// se duas threads escreverem no std::cout ao mesmo tempo, as mensagens podem sair misturadas.
//
// Solucao simples:
// proteger toda escrita no terminal com um std::mutex.
