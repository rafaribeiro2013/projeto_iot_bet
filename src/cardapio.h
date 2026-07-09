#pragma once

#include <Arduino.h>
#include "modelo.h"
#include "mqtt_module.h"

#define TOPICO_GET_CARDAPIO     "bet/getCardapio"
#define TOPICO_CARDAPIO         "bet/cardapio"
#define TOPICO_REGISTRAR_PEDIDO "bet/registrarPedido"
#define TOPICO_GET_MEUS_PEDIDOS "bet/getMeusPedidos"
#define TOPICO_MEUS_PEDIDOS     "bet/meusPedidos"
#define TOPICO_GET_COPO         "bet/getCopo"
#define TOPICO_COPO             "bet/copo"
#define TOPICO_GET_TOTAL_CONTA  "bet/getTotalConta"
#define TOPICO_TOTAL_CONTA      "bet/totalConta"

extern String payloadPixAtual;
extern bool pixPronto;

void cardapioInit();
void getCardapio(const char* categoria);
void cardapioRegistrarPedido(int32_t idCliente, int32_t idProduto);
void getMeusPedidos(int32_t idCliente);
void getCopo(const String& rfid);
void getTotalConta(int32_t idCliente);
void solicitarPix(float valor);
bool cardapioProcessarMensagem(const String& topico, const String& conteudo);
