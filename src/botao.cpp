#include "botao.h"

GFButton botaoUp    (1);
GFButton botaoCenter(2);
GFButton botaoDown  (42);

void botaoInit() {
  botaoUp    .setPressHandler(botaoUpPressionado);
  botaoCenter.setPressHandler(botaoCenterPressionado);
  botaoDown  .setPressHandler(botaoDownPressionado);
  Serial.println("[Botao] Módulo inicializado.");
}

void botaoLoop() {
  botaoUp    .process();
  botaoCenter.process();
  botaoDown  .process();
}

void botaoUpPressionado(GFButton& botaoDoEvento) {
  sessaoRecuar();
}

void botaoCenterPressionado(GFButton& botaoDoEvento) {
  sessaoConfirmar();
}

void botaoDownPressionado(GFButton& botaoDoEvento) {
  sessaoAvancar();
}