#include "apostas.h"
#include "estado_dados.h"
#include "preferencia.h"
#include <ArduinoJson.h>

void apostasInit() {
  mqtt.subscribe(TOPICO_PARTIDAS);
  mqtt.subscribe(TOPICO_MINHAS_APOSTAS);
  mqtt.subscribe(TOPICO_DADOS_CLIENTE);
  Serial.println("[Apostas] Modulo inicializado.");
}

static void _processarDadosCliente(const String& conteudo) {
  JsonDocument doc;
  if (deserializeJson(doc, conteudo)) { Serial.println("[Apostas] Erro dadosCliente."); return; }
  JsonObject obj = doc[0];
  clienteAtual.id           = obj["id"]            | 0;
  clienteAtual.nomeCompleto = String((const char*)(obj["nome_completo"] | ""));
  clienteAtual.telefone     = String((const char*)(obj["telefone"]      | ""));
  clienteAtual.email        = String((const char*)(obj["email"]         | ""));
  clienteCarregado = true;
  precisaRedesenhar = true;
  Serial.println("[Apostas] Cliente: " + clienteAtual.nomeCompleto);
}

void getPartidas() {
  partidasProntas = false;
  JsonDocument doc;
  doc["mesa"] = getNumeroMesa();
  String payload; serializeJson(doc, payload);
  mqttPublicar(TOPICO_GET_PARTIDAS, payload);
  Serial.println("[Apostas] Solicitando partidas: " + payload);
}

static void _processarPartidas(const String& conteudo) {
  JsonDocument doc;
  if (deserializeJson(doc, conteudo)) { Serial.println("[Apostas] Erro partidas."); return; }
  JsonArray array = doc.as<JsonArray>();
  uint8_t total = array.size() > 20 ? 20 : (uint8_t)array.size();
  uint8_t i = 0;
  for (JsonObject p : array) {
    if (i >= total) break;
    partidas[i].id         = p["id"]        | 0;
    partidas[i].idTimeCasa = p["mandante"]  | 0;
    partidas[i].idTimeFora = p["visitante"] | 0;
    partidas[i].nomeTimeCasa = String((const char*)(p["mandante_nome"]  | ""));
    partidas[i].nomeTimeFora = String((const char*)(p["visitante_nome"] | ""));
    partidas[i].data       = String((const char*)(p["data"] | ""));
    i++;
  }
  totalPartidas  = total;
  partidasProntas = true;
  precisaRedesenhar = true;
  Serial.printf("[Apostas] %d partida(s).\n", totalPartidas);
}

void apostasRealizar(const Aposta& aposta) {
  JsonDocument doc;
  doc["id_cliente"] = aposta.idCliente;
  doc["id_partida"] = aposta.idPartida;
  if (aposta.idTimeApostado == TIME_EMPATE) doc["id_time_apostado"] = nullptr;
  else                                      doc["id_time_apostado"] = aposta.idTimeApostado;
  doc["mesa"] = getNumeroMesa();
  String payload; serializeJson(doc, payload);
  mqttPublicar(TOPICO_REALIZAR_APOSTA, payload);
  Serial.println("[Apostas] Aposta enviada: " + payload);
}

void apostasConsultar(int32_t idCliente) {
  apostasProntas = false;
  JsonDocument doc;
  doc["id_cliente"] = idCliente;
  doc["mesa"] = getNumeroMesa();
  String payload; serializeJson(doc, payload);
  mqttPublicar(TOPICO_CONSULTAR_APOSTA, payload);
  Serial.printf("[Apostas] Consultando apostas do cliente %d\n", idCliente);
}

static void _processarMinhasApostas(const String& conteudo) {
  JsonDocument doc;
  if (deserializeJson(doc, conteudo)) { Serial.println("[Apostas] Erro minhasApostas."); return; }
  JsonArray array = doc.as<JsonArray>();
  uint8_t total = array.size() > 20 ? 20 : (uint8_t)array.size();
  uint8_t i = 0;
  for (JsonObject o : array) {
    if (i >= total) break;
    apostas[i].id             = o["id"]               | 0;
    apostas[i].createdAt      = String((const char*)(o["created_at"] | ""));
    apostas[i].idCliente      = o["id_cliente"]       | 0;
    apostas[i].idPartida      = o["id_partida"]       | 0;
    apostas[i].idTimeApostado = o["id_time_apostado"] | TIME_EMPATE;
    i++;
  }
  totalApostas  = total;
  apostasProntas = true;
  precisaRedesenhar = true;
  Serial.printf("[Apostas] %d aposta(s) do cliente.\n", totalApostas);
}

bool apostasProcessarMensagem(const String& topico, const String& conteudo) {
  if (topico == TOPICO_DADOS_CLIENTE)  { _processarDadosCliente(conteudo);  return true; }
  if (topico == TOPICO_PARTIDAS)       { _processarPartidas(conteudo);      return true; }
  if (topico == TOPICO_MINHAS_APOSTAS) { _processarMinhasApostas(conteudo); return true; }
  return false;
}
