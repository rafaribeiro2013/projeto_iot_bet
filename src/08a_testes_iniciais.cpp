#include <Arduino.h>

#include "rfid_module.h"
#include "wifi_module.h"
#include "mqtt_module.h"
#include "apostas.h"
#include "sessao.h"
#include "botao.h"

// ---------------------------------------------------------------------------
// Callback MQTT — ponto central de despacho de mensagens
// ---------------------------------------------------------------------------
void mqttMensagemRecebida(String topico, String conteudo) {
  Serial.println("[MQTT] " + topico + ": " + conteudo);

  if (apostasProcessarMensagem(topico, conteudo)) return;
  // TODO: if (outroModuloProcessarMensagem(topico, conteudo)) return;
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(500);

  rfidInit();
  wifiInit();
  mqttInit();
  apostasInit();
  sessaoInit();
  botaoInit();

  Serial.println("=== Sistema pronto ===");
}

// ---------------------------------------------------------------------------
// Loop
// ---------------------------------------------------------------------------
void loop() {
  wifiReconectar();
  mqttReconectar();
  mqttLoop();
  botaoLoop();

  // Leitura RFID — autentica o usuário na sessão
  if (rfidNovoCartao()) {
    String uid = rfidLerUID();
    Serial.println("[RFID] UID lido: " + uid);

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();

    sessaoAutenticar(uid);
  }
}