#ifndef DADOS_H
#define DADOS_H

#include "globais.h"
#include "estado_dados.h"
#include "apostas.h"
#include "cardapio.h"

// ===========================================================================
// CAMADA DE DADOS (repositorio)
// ---------------------------------------------------------------------------
// Hoje estas funcoes devolvem dados MOCKADOS. No futuro, cada uma sera
// substituida por uma chamada ao banco (HTTP/MQTT), mantendo EXATAMENTE o
// mesmo formato de retorno. A navegacao e o desenho nunca sabem se o dado e
// mock ou real -> a troca acontece so aqui.
//
// Procure por "MOCK:" para achar cada ponto de integracao com o banco.
// ===========================================================================

// Produtos de uma categoria do cardapio — lidos do buffer preenchido pelo MQTT.
// Cada item: { "id": <int>, "nome": <string>, "preco": <int>, "descricao": <string> }
void obterProdutos(const char* categoria, JsonArray destino) {
  for (uint8_t i = 0; i < totalProdutos; i++) {
    JsonObject o = destino.add<JsonObject>();
    o["id"]        = produtos[i].id;
    o["nome"]      = produtos[i].nomeComida;
    o["preco"]     = (int)(produtos[i].preco + 0.5f);
    o["descricao"] = produtos[i].descricao;
  }
}

// Envia o pedido real via MQTT (id do cliente autenticado + id do produto).
void registrarPedido(int32_t produtoId) {
  cardapioRegistrarPedido(clienteAtual.id, produtoId);
}

// Jogos disponiveis para aposta.
// Cada item: { "id": <int>, "casa": <string>, "fora": <string> }
void obterJogos(JsonArray destino) {
  for (uint8_t i = 0; i < totalPartidas; i++) {
    JsonObject o = destino.add<JsonObject>();
    o["id"]   = partidas[i].id;
    o["casa"] = partidas[i].nomeTimeCasa;
    o["fora"] = partidas[i].nomeTimeFora;
  }
}

// Apostas ja feitas pelo usuario — lidas do buffer preenchido pelo MQTT.
// Cada item: { "jogo": <string>, "palpite": <string>, "status": <string> }
void obterApostas(JsonArray destino) {
  for (uint8_t i = 0; i < totalApostas; i++) {
    JsonObject o = destino.add<JsonObject>();
    char jogo[16], palpite[24];
    snprintf(jogo, sizeof(jogo), "Partida %d", apostas[i].idPartida);
    if (apostas[i].idTimeApostado == TIME_EMPATE) snprintf(palpite, sizeof(palpite), "Empate");
    else snprintf(palpite, sizeof(palpite), "%s", apostas[i].nomeTimeApostado.c_str());
    o["jogo"]    = jogo;
    o["palpite"] = palpite;
    o["status"]  = "Pendente";
  }
}

// Envia aposta real via MQTT.
void registrarAposta(int jogoId, int palpite) {
  Aposta a;
  a.id = 0; a.createdAt = "";
  a.idCliente = clienteAtual.id;
  a.idPartida = jogoId;
  int32_t casa = 0, fora = 0;
  for (uint8_t i = 0; i < totalPartidas; i++) {
    if (partidas[i].id == jogoId) { casa = partidas[i].idTimeCasa; fora = partidas[i].idTimeFora; break; }
  }
  if      (palpite == 0) a.idTimeApostado = casa;
  else if (palpite == 2) a.idTimeApostado = fora;
  else                   a.idTimeApostado = TIME_EMPATE;
  apostasRealizar(a);
}

// Pedidos do cliente autenticado — lidos do buffer preenchido pelo MQTT.
// Cada item: { "nome": <string>, "preco": <int> }
void obterPedidos(JsonArray destino) {
  for (uint8_t i = 0; i < totalMeusPedidos; i++) {
    JsonObject o = destino.add<JsonObject>();
    o["nome"]  = meusPedidos[i].nomeComida;
    o["preco"] = (int)(meusPedidos[i].preco + 0.5f);
  }
}

// Total da conta do cliente autenticado (consumo_comidas + consumo_bebidas - desconto),
// lido do buffer preenchido pelo MQTT (getTotalConta). Usado tanto em Meus Pedidos
// quanto em Pagar Conta.
int obterTotalConta() {
  return (int)(totalContaAtual + 0.5f);
}

// Estado ao vivo do copo do cliente (tela de monitoramento da categoria Cervejas).
void obterConsumoCerveja(int& quantidadeMl, float& temperaturaC, String& status, int& nivelBateria) {
  quantidadeMl  = copoAtual.quantidadeMl;
  temperaturaC  = copoAtual.temperaturaC;
  status        = copoAtual.status;
  nivelBateria  = copoAtual.nivelBateria;
}

#endif
