#pragma once

#include <Arduino.h>
#include "modelo.h"
#include "mqtt_module.h"

#define TOPICO_REALIZAR_APOSTA  "bet/realizarAposta"
#define TOPICO_CONSULTAR_APOSTA "bet/consultarAposta"
#define TOPICO_MINHAS_APOSTAS   "bet/minhasApostas"
#define TOPICO_GET_PARTIDAS     "bet/getPartidas"
#define TOPICO_PARTIDAS         "bet/partidas"
// TOPICO_DADOS_CLIENTE vem de mqtt_module.h

void apostasInit();
void getPartidas();
void apostasRealizar(const Aposta& aposta);
void apostasConsultar(int32_t idCliente);
bool apostasProcessarMensagem(const String& topico, const String& conteudo);
