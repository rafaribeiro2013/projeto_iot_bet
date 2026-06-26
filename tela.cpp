#include "globais.h"
#include "dados.h"
#include "desenhos.h"
#include "navegacao.h"

// --- Definicao dos objetos globais declarados como extern em globais.h ---
U8G2_FOR_ADAFRUIT_GFX fontes;
GxEPD2_290_T94_V2 modeloTela(10, 14, 15, 16);
GxEPD2_BW<GxEPD2_290_T94_V2, GxEPD2_290_T94_V2::HEIGHT> tela(modeloTela);

// --- Botoes ---
GFButton botao_up(1);
GFButton botao_down(2);
GFButton botao_left(3);
GFButton botao_right(4);
GFButton botao_mid(5);

// Controle de inatividade (volta para a tela inicial apos 1 min parado).
unsigned long instanteAnterior = 0;

void setup() {
  Serial.begin(115200);
  delay(500);

  // inicializacao da tela
  tela.init();
  tela.setRotation(3);
  tela.fillScreen(GxEPD_WHITE);
  tela.display(true);
  fontes.begin(tela);
  fontes.setForegroundColor(GxEPD_BLACK);

  // estado inicial + primeira renderizacao
  estado.tipo = INICIAL;
  estado.indice = 0;
  renderizarTelaAtual();
  Serial.println("Setup");

  // liga cada botao ao seu handler de navegacao
  botao_mid.setPressHandler(botaoMidPressionado);
  botao_up.setPressHandler(botaoUpPressionado);
  botao_down.setPressHandler(botaoDownPressionado);
  botao_left.setPressHandler(botaoLeftPressionado);
  botao_right.setPressHandler(botaoRightPressionado);

  botao_mid.setHoldHandler(botaoMidSegurado);

  instanteAnterior = millis();
}

void loop() {
  botao_mid.process();
  botao_up.process();
  botao_down.process();
  botao_left.process();
  botao_right.process();

  // Tela "Confirmado!": apos 10s volta sozinha para o menu.
  if (estado.tipo == CONFIRMADO && (millis() - instanteConfirmado >= 10000)) {
    irParaMenu();
  }

  // Volta para a tela inicial apos 1 min sem nenhuma interacao.
  unsigned long instanteAtual = millis();
  if (instanteAtual > instanteAnterior + 60000) {
    Serial.println("+1 min sem interacao");
    instanteAnterior = instanteAtual;
    pilhaTopo = 0;
    estado.tipo = INICIAL;
    estado.indice = 0;
    renderizarTelaAtual();
  }
}
