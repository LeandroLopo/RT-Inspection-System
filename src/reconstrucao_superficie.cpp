// Implemente aqui a tarefa ReconstrucaoSuperficie.
//
// Responsabilidade:
// - consumir SensorData do SensorBuffer usando condition_variable;
// - aplicar media movel no valor do LIDAR;
// - ler a posicao X atual do estado compartilhado;
// - gerar SurfacePoint com timestamp, x, y e confidence;
// - inserir SurfacePoint no SurfaceBuffer;
// - futuramente detectar variacoes bruscas e sinalizar CameraEvent.
//
// Primeiro objetivo:
// fazer o fluxo SensorBuffer -> ReconstrucaoSuperficie -> SurfaceBuffer funcionar.
