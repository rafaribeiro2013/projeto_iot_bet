#include "globais.h"
#include "modelo.h"
#include "estado_dados.h"
#include "wifi_module.h"
#include "mqtt_module.h"
#include "rfid_module.h"
#include "apostas.h"
#include "cardapio.h"
#include "dados.h"
#include "desenhos.h"
#include "navegacao.h"
#include "preferencia.h"

// --- Definicao dos objetos globais declarados como extern em globais.h ---
U8G2_FOR_ADAFRUIT_GFX fontes;
GxEPD2_290_T94_V2 modeloTela(10, 14, 15, 16);
GxEPD2_BW<GxEPD2_290_T94_V2, GxEPD2_290_T94_V2::HEIGHT> tela(modeloTela);
QRCodeGFX qrcode(tela);

// --- Buffers de dados (declarados em estado_dados.h) ---
Cliente clienteAtual   = {0, "", "", ""};
bool    clienteCarregado = false;
Partida partidas[20];
uint8_t totalPartidas  = 0;
bool    partidasProntas = false;
Aposta  apostas[20];
uint8_t totalApostas   = 0;
bool    apostasProntas  = false;
volatile bool precisaRedesenhar = false;

Produto    produtos[20];
uint8_t    totalProdutos = 0;
bool       produtosProntos = false;

PedidoItem meusPedidos[20];
uint8_t    totalMeusPedidos = 0;
bool       meusPedidosProntos = false;

Copo copoAtual;
bool copoProntos = false;

float totalContaAtual = 0.0f;
bool  totalContaProntas = false;

String rfidAtual = "";

void mqttMensagemRecebida(String topico, String conteudo) {
  Serial.println("[MQTT] " + topico + ": " + conteudo);
  if (!apostasProcessarMensagem(topico, conteudo)) {
    cardapioProcessarMensagem(topico, conteudo);
  }
}

// --- Botoes ---
GFButton botao_up(1);
GFButton botao_down(2);
GFButton botao_left(42);
GFButton botao_right(41);
GFButton botao_mid(40);

// --- Preferences ---
Preferences preferencias;

// Controle de inatividade (volta para a tela inicial apos 1 min parado).
unsigned long instanteAnterior = 0;

void setup() {
  Serial.begin(115200);
  delay(500);

  // inicializacao do preferences
  initPreferences();

  // inicializacao da tela
  tela.init();
  tela.setRotation(3);
  tela.fillScreen(GxEPD_WHITE);
  tela.display(true);
  fontes.begin(tela);
  fontes.setForegroundColor(GxEPD_BLACK);

  // estado inicial + primeira renderizacao
  estado.tipo = AGUARDANDO_CARTAO;
  estado.indice = 0;
  renderizarTelaAtual();
  Serial.println("Setup");

  // liga cada botao ao seu handler de navegacao
  botao_mid.setPressHandler(botaoMidPressionado);
  botao_up.setPressHandler(botaoUpPressionado);
  botao_down.setPressHandler(botaoDownPressionado);
  botao_left.setPressHandler(botaoLeftPressionado);
  botao_right.setPressHandler(botaoRightPressionado);

  wifiInit();
  mqttInit();
  rfidInit();
  apostasInit();
  cardapioInit();

  instanteAnterior = millis();
}

void loop() {
  wifiReconectar();
  mqttReconectar();
  mqttLoop();

  if (rfidNovoCartao()) {
    String uid = rfidLerUID();
    rfidAtual = uid;
    Serial.println("[Loop] Cartao UID: " + uid);
    clienteCarregado = false;
    Serial.println("[Diag] 1) publicando autenticacao...");
    mqttPublicar(TOPICO_AUTENTICA_CLIENTE, uid);
    Serial.println("[Diag] 2) publicado; vou desenhar Carregando...");
    strncpy(msgCarregando, "Autenticando", sizeof(msgCarregando) - 1);
    estado.tipo = CARREGANDO; estado.indice = ESPERANDO_CLIENTE;
    renderizarTelaAtual();
    Serial.println("[Diag] 3) Carregando desenhado; aguardando dadosCliente.");
  }

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
    clienteCarregado = false;
    estado.tipo = AGUARDANDO_CARTAO;
    estado.indice = 0;
    renderizarTelaAtual();
  }

  static unsigned long inicioCarregando = 0;
  if (estado.tipo == CARREGANDO) {
    if (inicioCarregando == 0) inicioCarregando = millis();
    if (millis() - inicioCarregando > 8000) {
      strncpy(msgCarregando, "Sem resposta", sizeof(msgCarregando) - 1);
      estado.tipo = AVISO; renderizarTelaAtual();
      inicioCarregando = 0;
    }
  } else {
    inicioCarregando = 0;
  }

  if (precisaRedesenhar) {
    precisaRedesenhar = false;
    renderizarTelaAtual();
  }
}