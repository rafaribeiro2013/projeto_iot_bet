#include <ArduinoJson.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <QRCodeGFX.h>
/*inserir bibliotecas para tratamento do WiFi*/

U8G2_FOR_ADAFRUIT_GFX fontes;
GxEPD2_290_T94_V2 modeloTela(10, 14, 15, 16);
GxEPD2_BW<GxEPD2_290_T94_V2, GxEPD2_290_T94_V2::HEIGHT> tela(modeloTela);
QRCodeGFX qrcode(tela); 


void fecharConta(){
    //Vamos receber, provavelmente de um tópico MQTT, o valor total que será debitado da conta
    float valorTotal;
    //Enviar esse valor para a API de pagamentos Pix, fazendo a cobrança efetiva do cliente.
    String codigoPix; //retorno da API
    qrcode.setScale(3);
    qrcode.draw(codigoPix, 100, 30);
    tela.display(true);
}

void setup(){
    Serial.begin(115200); delay(500);

    //Tela e fontes
    tela.init();
    tela.setRotation(3);
    tela.fillScreen(GxEPD_WHITE);
    tela.display(true);

    fontes.begin(tela);
    fontes.setForegroundColor(GxEPD_BLACK);
    
}

void loop(){
    
}
