#include "wifi_module.h"

WiFiClientSecure conexaoSegura;

void wifiInit() {
  WiFi.mode(WIFI_STA);
  conexaoSegura.setCACert(certificado1);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("[WiFi] Conexao iniciada (nao-bloqueante).");
}

// NAO-BLOQUEANTE: nunca trava o loop. Tenta reconectar no maximo a cada 5s.
// O laco while(...) bloqueante anterior congelava toda a navegacao quando a
// rede caia (tela em branco + botoes mortos).
void wifiReconectar() {
  static unsigned long ultimaTentativa = 0;
  static bool estavaConectado = false;

  if (WiFi.status() == WL_CONNECTED) {
    if (!estavaConectado) {
      estavaConectado = true;
      Serial.print("[WiFi] Conectado! IP: ");
      Serial.println(WiFi.localIP());
    }
    return;
  }

  estavaConectado = false;
  unsigned long agora = millis();
  if (agora - ultimaTentativa >= 5000) {
    ultimaTentativa = agora;
    Serial.println("[WiFi] Sem conexao, tentando reconectar...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  }
}

bool wifiConectado() {
  return WiFi.status() == WL_CONNECTED;
}
