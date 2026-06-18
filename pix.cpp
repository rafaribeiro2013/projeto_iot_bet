#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <QRCodeGFX.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "certificados.h"
#include <MQTT.h>

U8G2_FOR_ADAFRUIT_GFX fontes;
GxEPD2_290_T94_V2 modeloTela(10, 14, 15, 16);
GxEPD2_BW<GxEPD2_290_T94_V2, GxEPD2_290_T94_V2::HEIGHT> tela(modeloTela);
QRCodeGFX qrcode(tela); 

WiFiClientSecure conexaoSegura;
MQTTClient mqtt(1000); 

void reconectarWiFi() {
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.begin("NOME DA REDE", "SENHA DA REDE");
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
            mqtt.connect("SEU ID", "LOGIN", "SENHA");
            Serial.print(".");
            delay(1000);
        }
        Serial.println(" conectado!");

        //mqtt.subscribe("topico1"); 
    }
}

void recebeuMensagem(String topico, String conteudo) {
    Serial.println(topico + ": " + conteudo);
}

void fecharConta(float valorTotal){
    //Enviar o valor para um tópico MQTT para que seja feita a cobrança pelo servidor
    mqtt.publish("conta", String(valorTotal));
    String codigoPix = "";
    qrcode.setScale(3);
    qrcode.draw(codigoPix, 100, 30);
    tela.display(true);
}

void setup(){
    Serial.begin(115200); delay(500);

    //WiFi
    reconectarWiFi();
    conexaoSegura.setCACert(certificado1);

    //Tela e fontes
    tela.init();
    tela.setRotation(3);
    tela.fillScreen(GxEPD_WHITE);
    tela.display(true);
    fontes.begin(tela);
    fontes.setForegroundColor(GxEPD_BLACK);

    //MQTT
    mqtt.begin("mqtt.janks.dev.br", 8883, conexaoSegura);
    mqtt.onMessage(recebeuMensagem);
    reconectarMQTT();
}

void loop(){
    reconectarWifi();
    reconectarMQTT();
    mqtt.loop();
}
