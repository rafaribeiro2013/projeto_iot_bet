#pragma once

#include <Arduino.h>
#include "mqtt_module.h"

// ---------------------------------------------------------------------------
// Tópicos MQTT
// ---------------------------------------------------------------------------
#define TOPICO_REALIZAR_APOSTA  "bet/realizarAposta"
#define TOPICO_ATUALIZAR_APOSTA "bet/atualizarAposta"
#define TOPICO_CONSULTAR_APOSTA "bet/consultarAposta"
#define TOPICO_MINHAS_APOSTAS "bet/minhasApostas"
#define TOPICO_RESULTADO_APOSTA "bet/resultadoAposta"
#define TOPICO_GET_PARTIDAS     "bet/getPartidas"
#define TOPICO_PARTIDAS         "bet/partidas"
#define TOPICO_DADOS_CLIENTE "dadosCliente"

// ---------------------------------------------------------------------------
// Struct Aposta — espelha a tabela `apostas` do PostgreSQL
// ---------------------------------------------------------------------------
struct Aposta {
  int32_t id;          // PK — definido pelo servidor
  String  createdAt;   // ISO-8601 — definido pelo servidor
  int32_t idCliente;
  int32_t idPartida;
  int32_t idTimeApostado;
};

void apostasInit();

static void _processarDadosCliente(const String& conteudo);

// Solicita lista de partidas ao servidor (responde em bet/partidas)
void getPartidas();

// Envia uma aposta ao servidor
void apostasRealizar(const Aposta& aposta);

// Consulta apostas de um cliente pelo id
void apostasConsultar(int32_t idCliente);

// Despacho de mensagens MQTT — chame em mqttMensagemRecebida()
bool apostasProcessarMensagem(const String& topico, const String& conteudo);