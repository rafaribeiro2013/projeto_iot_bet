#include "wifi_module.h"

WiFiClientSecure conexaoSegura;

void wifiInit() {
  WiFi.mode(WIFI_STA);
  wifiReconectar();
  conexaoSegura.setCACert(certificado1);
  Serial.println("[WiFi] Certificado TLS configurado.");
}

void wifiReconectar() {
  if (WiFi.status() == WL_CONNECTED) return;

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("[WiFi] Conectando");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.print(" conectado! IP: ");
  Serial.println(WiFi.localIP());
}

bool wifiConectado() {
  return WiFi.status() == WL_CONNECTED;
}