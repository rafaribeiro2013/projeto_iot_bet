#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

// Pinos do leitor RFID
#define RFID_PIN_SS  46
#define RFID_PIN_RST 17

extern MFRC522 rfid;
extern MFRC522::MIFARE_Key chaveA;

void rfidInit();
bool rfidNovoCartao();
String rfidLerUID();
String rfidLerBloco(byte bloco);