#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "certificados.h"

#define WIFI_SSID     "LabIoT"
#define WIFI_PASSWORD "4n1m4l5@))!!"

extern WiFiClientSecure conexaoSegura;

void wifiInit();
void wifiReconectar();
bool wifiConectado();