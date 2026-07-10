#include "mqtt_module.h"
#include "apostas.h"
#include "preferencia.h"
#include "cardapio.h"

int numeroMesa;

MQTTClient mqtt(1000);

// Inscreve em todos os topicos de resposta. Chamado a cada (re)conexao bem
// sucedida, para que o broker volte a entregar mensagens apos uma queda.
static void mqttInscrever() {
  mqtt.subscribe(TOPICO_DADOS_CLIENTE);
  mqtt.subscribe(TOPICO_PARTIDAS);
  mqtt.subscribe(TOPICO_MINHAS_APOSTAS);
  mqtt.subscribe(TOPICO_CARDAPIO);
  mqtt.subscribe(TOPICO_MEUS_PEDIDOS);
  mqtt.subscribe(TOPICO_COPO);
  mqtt.subscribe(TOPICO_TOTAL_CONTA);
  mqtt.subscribe(TOPICO_CODIGO_PIX);
  Serial.println("[MQTT] Inscrito nos topicos.");
}

void mqttInit() {
  numeroMesa = getNumeroMesa();
  mqtt.begin(MQTT_HOST, MQTT_PORT, conexaoSegura);
  mqtt.onMessage(mqttMensagemRecebida);
  mqtt.setKeepAlive(MQTT_KEEPALIVE);
  mqtt.setWill(TOPICO_LWT, LWT_MENSAGEM);
  // A conexao acontece de forma nao-bloqueante em mqttReconectar() (loop).
}

// NAO-BLOQUEANTE: uma tentativa por vez, no maximo a cada 3s. O laco
// while(!connected) anterior travava o loop inteiro para sempre se o broker
// nao respondesse — causando tela em branco e navegacao morta.
void mqttReconectar() {
  if (mqtt.connected()) return;
  if (WiFi.status() != WL_CONNECTED) return;   // sem WiFi, nem tenta

  static unsigned long ultimaTentativa = 0;
  unsigned long agora = millis();
  if (agora - ultimaTentativa < 3000) return;
  ultimaTentativa = agora;

  Serial.print("[MQTT] Tentando conectar... ");
  if (mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
    Serial.println("conectado!");
    mqttInscrever();
  } else {
    Serial.println("falhou (nova tentativa em 3s).");
  }
}

void mqttLoop() {
  mqtt.loop();
}

bool mqttConectado() {
  return mqtt.connected();
}

void mqttPublicar(const char* topico, const String& payload) {
  mqtt.publish(topico, payload);
}
