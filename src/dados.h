#ifndef DADOS_H
#define DADOS_H

#include "globais.h"
#include "estado_dados.h"
#include "apostas.h"

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

// Produtos de uma categoria do cardapio.
// Cada item: { "nome": <string>, "preco": <int>, "descricao": <string> }
void obterProdutos(const char* categoria, JsonArray destino) {
  // MOCK: substituir por consulta ao banco filtrando por 'categoria'.
  if (strcmp(categoria, "drinks") == 0) {
    JsonObject a = destino.add<JsonObject>();
    a["nome"] = "Caipirinha";    a["preco"] = 18; a["descricao"] = "Limao, cachaca e acucar";
    JsonObject b = destino.add<JsonObject>();
    b["nome"] = "Mojito";        b["preco"] = 20; b["descricao"] = "Rum, hortela e limao";
    JsonObject c = destino.add<JsonObject>();
    c["nome"] = "Gin Tonica";    c["preco"] = 22; c["descricao"] = "Gin, agua tonica e limao";
    JsonObject d = destino.add<JsonObject>();
    d["nome"] = "Aperol Spritz"; d["preco"] = 24; d["descricao"] = "Aperol, espumante e soda";
  } else if (strcmp(categoria, "petiscos") == 0) {
    JsonObject a = destino.add<JsonObject>();
    a["nome"] = "Batata Frita";  a["preco"] = 16; a["descricao"] = "Porcao crocante 300g";
    JsonObject b = destino.add<JsonObject>();
    b["nome"] = "Calabresa";     b["preco"] = 22; b["descricao"] = "Acebolada na cerveja";
    JsonObject c = destino.add<JsonObject>();
    c["nome"] = "Gurjao";        c["preco"] = 28; c["descricao"] = "Frango empanado 400g";
    JsonObject d = destino.add<JsonObject>();
    d["nome"] = "Hamburguer";    d["preco"] = 35; d["descricao"] = "Artesanal com fritas";
  } else if (strcmp(categoria, "nao alcoolico") == 0) {
    JsonObject a = destino.add<JsonObject>();
    a["nome"] = "Suco de Laranja"; a["preco"] = 10; a["descricao"] = "Natural 400ml";
    JsonObject b = destino.add<JsonObject>();
    b["nome"] = "Refrigerante";    b["preco"] = 8;  b["descricao"] = "Lata 350ml";
    JsonObject c = destino.add<JsonObject>();
    c["nome"] = "Agua";            c["preco"] = 5;  c["descricao"] = "Com ou sem gas 500ml";
    JsonObject d = destino.add<JsonObject>();
    d["nome"] = "Limonada";        d["preco"] = 12; d["descricao"] = "Limonada suica cremosa";
  }
}

// Jogos disponiveis para aposta.
// Cada item: { "id": <int>, "casa": <string>, "fora": <string> }
void obterJogos(JsonArray destino) {
  for (uint8_t i = 0; i < totalPartidas; i++) {
    JsonObject o = destino.add<JsonObject>();
    o["id"] = partidas[i].id;
    char casa[12], fora[12];
    snprintf(casa, sizeof(casa), "Time %d", partidas[i].idTimeCasa);
    snprintf(fora, sizeof(fora), "Time %d", partidas[i].idTimeFora);
    o["casa"] = casa;
    o["fora"] = fora;
  }
}

// Apostas ja feitas pelo usuario — lidas do buffer preenchido pelo MQTT.
// Cada item: { "jogo": <string>, "palpite": <string>, "status": <string> }
void obterApostas(JsonArray destino) {
  for (uint8_t i = 0; i < totalApostas; i++) {
    JsonObject o = destino.add<JsonObject>();
    char jogo[16], palpite[16];
    snprintf(jogo, sizeof(jogo), "Partida %d", apostas[i].idPartida);
    if (apostas[i].idTimeApostado == TIME_EMPATE) snprintf(palpite, sizeof(palpite), "Empate");
    else snprintf(palpite, sizeof(palpite), "Time %d", apostas[i].idTimeApostado);
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

// Pedidos da mesa.
// Cada item: { "nome": <string>, "preco": <int> }
void obterPedidos(JsonArray destino) {
  // MOCK: substituir pelos pedidos da mesa vindos do banco.
  JsonObject a = destino.add<JsonObject>();
  a["nome"] = "Caipirinha";   a["preco"] = 18;
  JsonObject b = destino.add<JsonObject>();
  b["nome"] = "Batata Frita"; b["preco"] = 16;
  JsonObject c = destino.add<JsonObject>();
  c["nome"] = "Mojito";       c["preco"] = 20;
}

// Total da conta (soma dos pedidos).
int obterTotalConta() {
  // MOCK: idealmente vem somado do banco. Aqui somamos os pedidos mockados.
  JsonDocument doc;
  JsonArray pedidos = doc.to<JsonArray>();
  obterPedidos(pedidos);
  int total = 0;
  for (JsonObject p : pedidos) {
    total += p["preco"].as<int>();
  }
  return total;
}

// Consumo de cerveja da mesa (tela de monitoramento da categoria Cervejas).
void obterConsumoCerveja(int& gastoCentavos, int& copos, int& cervejasGratis) {
  // MOCK: substituir pelos dados do copo/choppeira inteligente vindos do banco.
  gastoCentavos  = 4750;  // R$ 47,50
  copos          = 12;
  cervejasGratis = 2;
}

#endif
