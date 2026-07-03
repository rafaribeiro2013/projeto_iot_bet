#ifndef GLOBAIS_H
#define GLOBAIS_H

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <ArduinoJson.h>
#include <GFButton.h>
#include <QRCodeGFX.h>

extern U8G2_FOR_ADAFRUIT_GFX fontes;
extern GxEPD2_BW<GxEPD2_290_T94_V2, GxEPD2_290_T94_V2::HEIGHT> tela;
extern QRCodeGFX qrcode;

#endif
