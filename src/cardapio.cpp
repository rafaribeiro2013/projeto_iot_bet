#include "cardapio.h"
#include "estado_dados.h"
#include <ArduinoJson.h>

void cardapioInit() {
  mqtt.subscribe(TOPICO_CARDAPIO);
  mqtt.subscribe(TOPICO_MEUS_PEDIDOS);
  mqtt.subscribe(TOPICO_COPO);
  mqtt.subscribe(TOPICO_TOTAL_CONTA);
  Serial.println("[Cardapio] Modulo inicializado.");
}

void getCardapio(const char* categoria) {
  produtosProntos = false;
  JsonDocument doc;
  doc["categoria"] = categoria;
  String payload; serializeJson(doc, payload);
  mqttPublicar(TOPICO_GET_CARDAPIO, payload);
  Serial.println("[Cardapio] Solicitando cardapio: " + payload);
}

static void _processarCardapio(const String& conteudo) {
  JsonDocument doc;
  if (deserializeJson(doc, conteudo)) { Serial.println("[Cardapio] Erro cardapio."); return; }
  JsonArray array = doc.as<JsonArray>();
  uint8_t total = array.size() > 20 ? 20 : (uint8_t)array.size();
  uint8_t i = 0;
  for (JsonObject p : array) {
    if (i >= total) break;
    produtos[i].id         = p["id"]          | 0;
    produtos[i].preco      = p["preco"]        | 0.0f;
    produtos[i].categoria  = String((const char*)(p["categoria"]   | ""));
    produtos[i].nomeComida = String((const char*)(p["nome_comida"] | ""));
    produtos[i].descricao  = String((const char*)(p["descricao"]   | ""));
    i++;
  }
  totalProdutos   = total;
  produtosProntos = true;
  precisaRedesenhar = true;
  Serial.printf("[Cardapio] %d produto(s).\n", totalProdutos);
}

void cardapioRegistrarPedido(int32_t idCliente, int32_t idProduto) {
  JsonDocument doc;
  doc["id_cliente"] = idCliente;
  doc["id_produto"] = idProduto;
  String payload; serializeJson(doc, payload);
  mqttPublicar(TOPICO_REGISTRAR_PEDIDO, payload);
  Serial.println("[Cardapio] Pedido enviado: " + payload);
}

void getMeusPedidos(int32_t idCliente) {
  meusPedidosProntos = false;
  JsonDocument doc;
  doc["id_cliente"] = idCliente;
  String payload; serializeJson(doc, payload);
  mqttPublicar(TOPICO_GET_MEUS_PEDIDOS, payload);
  Serial.printf("[Cardapio] Solicitando meus pedidos do cliente %d\n", idCliente);
}

static void _processarMeusPedidos(const String& conteudo) {
  JsonDocument doc;
  if (deserializeJson(doc, conteudo)) { Serial.println("[Cardapio] Erro meusPedidos."); return; }
  JsonArray array = doc.as<JsonArray>();
  uint8_t total = array.size() > 20 ? 20 : (uint8_t)array.size();
  uint8_t i = 0;
  for (JsonObject o : array) {
    if (i >= total) break;
    meusPedidos[i].id         = o["id"]            | 0;
    meusPedidos[i].idProduto  = o["id_produto"]    | 0;
    meusPedidos[i].nomeComida = String((const char*)(o["nome_comida"] | ""));
    meusPedidos[i].preco      = o["preco"]          | 0.0f;
    meusPedidos[i].status     = String((const char*)(o["status"]      | ""));
    i++;
  }
  totalMeusPedidos   = total;
  meusPedidosProntos = true;
  precisaRedesenhar = true;
  Serial.printf("[Cardapio] %d pedido(s) do cliente.\n", totalMeusPedidos);
}

void getCopo(const String& rfid) {
  copoProntos = false;
  JsonDocument doc;
  doc["rfid"] = rfid;
  String payload; serializeJson(doc, payload);
  mqttPublicar(TOPICO_GET_COPO, payload);
  Serial.println("[Cardapio] Solicitando copo do rfid: " + rfid);
}

static void _processarCopo(const String& conteudo) {
  JsonDocument doc;
  if (deserializeJson(doc, conteudo)) { Serial.println("[Cardapio] Erro copo."); return; }
  JsonObject o = doc[0];   // node-red-node-postgresql devolve array de linhas, mesmo p/ 1 linha
  copoAtual.id           = o["id"]             | 0;
  copoAtual.status       = String((const char*)(o["status"] | ""));
  copoAtual.idCopo       = o["id_copo"]        | 0;
  copoAtual.rfid         = String((const char*)(o["rfid"]   | ""));
  copoAtual.idCliente    = o["id_cliente"]     | 0;
  copoAtual.nivelBateria = o["nivel_bateria"]  | 0;
  copoAtual.quantidadeMl = o["quantidade_ml"]  | 0;
  copoAtual.temperaturaC = o["temperatura_c"]  | 0.0f;
  copoAtual.mesa         = o["mesa"]           | 0;
  copoProntos = true;
  precisaRedesenhar = true;
  Serial.printf("[Cardapio] Copo: %dml, %.1fC, %s\n",
                copoAtual.quantidadeMl, copoAtual.temperaturaC, copoAtual.status.c_str());
}

void getTotalConta(int32_t idCliente) {
  totalContaProntas = false;
  JsonDocument doc;
  doc["id_cliente"] = idCliente;
  String payload; serializeJson(doc, payload);
  mqttPublicar(TOPICO_GET_TOTAL_CONTA, payload);
  Serial.printf("[Cardapio] Solicitando total da conta do cliente %d\n", idCliente);
}

static void _processarTotalConta(const String& conteudo) {
  JsonDocument doc;
  if (deserializeJson(doc, conteudo)) { Serial.println("[Cardapio] Erro totalConta."); return; }
  JsonObject o = doc[0];   // node-red-node-postgresql devolve array de linhas, mesmo p/ 1 linha
  totalContaAtual = o["total"] | 0.0f;
  totalContaProntas = true;
  precisaRedesenhar = true;
  Serial.printf("[Cardapio] Total da conta: %.2f\n", totalContaAtual);
}

bool cardapioProcessarMensagem(const String& topico, const String& conteudo) {
  if (topico == TOPICO_CARDAPIO)     { _processarCardapio(conteudo);     return true; }
  if (topico == TOPICO_MEUS_PEDIDOS) { _processarMeusPedidos(conteudo);  return true; }
  if (topico == TOPICO_COPO)         { _processarCopo(conteudo);        return true; }
  if (topico == TOPICO_TOTAL_CONTA)  { _processarTotalConta(conteudo);  return true; }
  return false;
}
