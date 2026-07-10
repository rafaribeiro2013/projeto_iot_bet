#include "mqtt_module.h"
#include "apostas.h"
#include "preferencia.h"
#include "cardapio.h"

int numeroMesa;

MQTTClient mqtt(1000);

void mqttInit() {
  numeroMesa = getNumeroMesa();
  mqtt.begin(MQTT_HOST, MQTT_PORT, conexaoSegura);
  mqtt.onMessage(mqttMensagemRecebida);
  mqtt.setKeepAlive(MQTT_KEEPALIVE);
  mqtt.setWill(TOPICO_LWT, LWT_MENSAGEM);
  mqttReconectar();
}

void mqttReconectar() {
  if (mqtt.connected()) return;

  Serial.print("[MQTT] Conectando");
  while (!mqtt.connected()) {
    mqtt.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD);
    Serial.print(".");
    delay(1000);
  }
  Serial.println(" conectado!");

  mqtt.subscribe(TOPICO_DADOS_CLIENTE);
  mqtt.subscribe(TOPICO_PARTIDAS);
  mqtt.subscribe(TOPICO_MINHAS_APOSTAS);
  mqtt.subscribe(TOPICO_CARDAPIO);
  mqtt.subscribe(TOPICO_MEUS_PEDIDOS);
  mqtt.subscribe(TOPICO_COPO);
  mqtt.subscribe(TOPICO_TOTAL_CONTA);
  mqtt.subscribe(TOPICO_CODIGO_PIX);
  Serial.println("[MQTT] Inscrito em: " TOPICO_DADOS_CLIENTE);
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