#pragma once

#include <Arduino.h>
#include <MQTT.h>
#include "wifi_module.h"

#define MQTT_HOST       "mqtt.janks.dev.br"
#define MQTT_PORT       8883
#define MQTT_CLIENT_ID  "BET"
#define MQTT_USER       "aula"
#define MQTT_PASSWORD   "zowmad-tavQez"
#define MQTT_KEEPALIVE  10

// Tópicos de publicação
#define TOPICO_AUTENTICA_CLIENTE  "bet/autenticaCliente"

// Tópicos de subscrição
#define TOPICO_DADOS_CLIENTE      "dadosCliente"

// Tópico de LWT (Last Will and Testament)
#define TOPICO_LWT                "bet/status"
#define LWT_MENSAGEM              "offline"

extern MQTTClient mqtt;

void mqttInit();
void mqttReconectar();
void mqttLoop();
bool mqttConectado();
void mqttPublicar(const char* topico, const String& payload);

// Callback — definido no main para permitir acesso a outros módulos
void mqttMensagemRecebida(String topico, String conteudo);