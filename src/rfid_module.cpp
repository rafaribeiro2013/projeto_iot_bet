#include "rfid_module.h"

MFRC522 rfid(RFID_PIN_SS, RFID_PIN_RST);
MFRC522::MIFARE_Key chaveA = {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};

void rfidInit() {
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("[RFID] Leitor inicializado.");
}

// Verifica se há novo cartão E lê o serial em uma chamada só
bool rfidNovoCartao() {
  return rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial();
}

String rfidLerUID() {
  String id = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (i > 0) id += " ";
    if (rfid.uid.uidByte[i] < 0x10) id += "0";
    id += String(rfid.uid.uidByte[i], HEX);
  }
  id.toUpperCase();
  return id;
}

String rfidLerBloco(byte bloco) {
  byte tamanhoDados = 18;
  char dados[tamanhoDados];

  MFRC522::StatusCode status = rfid.PCD_Authenticate(
    MFRC522::PICC_CMD_MF_AUTH_KEY_A,
    bloco, &chaveA, &(rfid.uid)
  );
  if (status != MFRC522::STATUS_OK) return "";

  status = rfid.MIFARE_Read(bloco, (byte*)dados, &tamanhoDados);
  if (status != MFRC522::STATUS_OK) return "";

  dados[tamanhoDados - 2] = '\0';
  return String(dados);
}