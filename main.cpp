#include <Arduino.h>
#include <SPI.h> 
#include <MFRC522.h>
#include <WiFi.h> 
#include <WiFiClientSecure.h> 
#include "certificados.h" 
#include <MQTT.h> 

WiFiClientSecure conexaoSegura; 

MQTTClient mqtt(1000);

MFRC522 rfid(46, 17);
MFRC522::MIFARE_Key chaveA = 
{{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

String lerRFID() { 
  String id = ""; 
  for (byte i = 0; i < rfid.uid.size; i++) { 
    if (i > 0) { 
      id += " "; 
    } 
    if (rfid.uid.uidByte[i] < 0x10) { 
      id += "0"; 
    } 
    id += String(rfid.uid.uidByte[i], HEX); 
  } 
  id.toUpperCase(); 
  return id; 
}

String lerTextoDoBloco(byte bloco) { 
  byte tamanhoDados = 18; 
  char dados[tamanhoDados]; 
  MFRC522::StatusCode status = rfid.PCD_Authenticate( 
    MFRC522::PICC_CMD_MF_AUTH_KEY_A, 
    bloco, &chaveA, &(rfid.uid) 
  ); 
  if (status != MFRC522::STATUS_OK) { return ""; } 
  status = rfid.MIFARE_Read(bloco, 
              (byte*)dados, &tamanhoDados); 
  if (status != MFRC522::STATUS_OK) { return ""; } 
  dados[tamanhoDados - 2] = '\0'; 
  return String(dados); 
}

void reconectarWiFi() { 
  if (WiFi.status() != WL_CONNECTED) { 
    WiFi.begin("LabIoT", "4n1m4l5@))!!"); 
    Serial.print("Conectando ao WiFi..."); 
    while (WiFi.status() != WL_CONNECTED) { 
      Serial.print("."); 
      delay(1000); 
    } 
    Serial.print("conectado!\nEndereço IP: "); 
    Serial.println(WiFi.localIP()); 
  } 
}

void reconectarMQTT() { 
  if (!mqtt.connected()) { 
    Serial.print("Conectando MQTT..."); 
    while(!mqtt.connected()) { 
      mqtt.connect("BET", "aula", "zowmad-tavQez"); 
      Serial.print("."); 
      delay(1000); 
    } 
    Serial.println(" conectado!"); 
    
    mqtt.subscribe("dadosCliente");
  } 
}

void recebeuMensagem(String topico, String conteudo) { 
  Serial.println(topico + ": " + conteudo); 
}

void setup() {
  Serial.begin(115200); delay(500);
  Serial.println("Projeto 05 – Testes Iniciais");
  Serial.println("MENGO");
  SPI.begin(); 
  rfid.PCD_Init();

  reconectarWiFi(); 
  conexaoSegura.setCACert(certificado1); 
  mqtt.begin("mqtt.janks.dev.br", 8883, conexaoSegura); 
  mqtt.onMessage(recebeuMensagem); 
  mqtt.setKeepAlive(10); 
  mqtt.setWill("tópico da desconexão", "conteúdo"); 
  reconectarMQTT();
}

void loop() { 
  if (rfid.PICC_IsNewCardPresent() && 
      rfid.PICC_ReadCardSerial()){ 
    String id = lerRFID(); 
    Serial.println("UID: " + id); 

    // String texto = lerTextoDoBloco(6); 
    // Serial.println("Bloco 6: " + texto); 
     
    rfid.PICC_HaltA(); 
    rfid.PCD_StopCrypto1(); 

    mqtt.publish("bet/autenticaCliente", id);
  } 

  reconectarWiFi(); 
  reconectarMQTT(); 
  mqtt.loop();
}
