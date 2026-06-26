#include "apostas.h"
#include "sessao.h"
#include <ArduinoJson.h>

void apostasInit() {
  mqtt.subscribe(TOPICO_RESULTADO_APOSTA);
  mqtt.subscribe(TOPICO_PARTIDAS);
  mqtt.subscribe(TOPICO_DADOS_CLIENTE);
  Serial.println("[Apostas] Módulo inicializado.");
}

static void _processarDadosCliente(const String& conteudo) {
  JsonDocument doc;
  if (deserializeJson(doc, conteudo)) {
    Serial.println("[Apostas] Erro ao parsear dadosCliente.");
    return;
  }
  JsonObject obj = doc[0];  // array com um elemento
  Cliente c;
  c.id           = obj["id"]            | 0;
  c.nomeCompleto = obj["nome_completo"] | "";
  c.telefone     = obj["telefone"]      | "";
  c.email        = obj["email"]         | "";
  sessaoReceberCliente(c);
}

void getPartidas() {
  mqttPublicar(TOPICO_GET_PARTIDAS, "");
  Serial.println("[Apostas] Solicitando partidas...");
}

// Payload: { "id_cliente": 1, "id_partida": 2, "id_time_apostado": 3 }
void apostasRealizar(const Aposta& aposta) {
  JsonDocument doc;
  doc["id_cliente"]       = aposta.idCliente;
  doc["id_partida"]       = aposta.idPartida;
  doc["id_time_apostado"] = aposta.idTimeApostado;

  String payload;
  serializeJson(doc, payload);

  mqttPublicar(TOPICO_REALIZAR_APOSTA, payload);
  Serial.printf("[Apostas] Aposta enviada — cliente:%d partida:%d time:%d\n",
    aposta.idCliente, aposta.idPartida, aposta.idTimeApostado);
}

// Payload: { "id_cliente": 123 }
void apostasConsultar(int32_t idCliente) {
  JsonDocument doc;
  doc["id_cliente"] = idCliente;

  String payload;
  serializeJson(doc, payload);

  mqttPublicar(TOPICO_CONSULTAR_APOSTA, payload);
  Serial.printf("[Apostas] Consultando apostas do cliente: %d\n", idCliente);
}

// ---------------------------------------------------------------------------
// Handlers internos de mensagens recebidas
// ---------------------------------------------------------------------------

// Resposta esperada em bet/resultadoAposta:
// [{ "id":1, "created_at":"...", "id_cliente":1, "id_partida":2, "id_time_apostado":3 }]
static void _processarResultadoAposta(const String& conteudo) {
  JsonDocument doc;
  if (deserializeJson(doc, conteudo)) {
    Serial.println("[Apostas] Erro ao parsear resultado.");
    return;
  }
  JsonArray array = doc.as<JsonArray>();
  Serial.printf("[Apostas] %d aposta(s) recebida(s):\n", array.size());
  for (JsonObject obj : array) {
    Serial.printf("  id:%d cliente:%d partida:%d time:%d em %s\n",
      (int32_t)(obj["id"]               | 0),
      (int32_t)(obj["id_cliente"]       | 0),
      (int32_t)(obj["id_partida"]       | 0),
      (int32_t)(obj["id_time_apostado"] | 0),
      (const char*)(obj["created_at"]   | ""));
  }
}

// Resposta esperada em bet/partidas:
// [{ "id":1, "time_casa":10, "time_fora":20, "data":"2025-06-15T18:00:00Z" }]
static void _processarPartidas(const String& conteudo) {
  JsonDocument doc;
  if (deserializeJson(doc, conteudo)) {
    Serial.println("[Apostas] Erro ao parsear partidas.");
    return;
  }

  JsonArray array = doc.as<JsonArray>();
  uint8_t total = array.size() > 20 ? 20 : (uint8_t)array.size();

  Partida lista[20];
  uint8_t i = 0;
  for (JsonObject p : array) {
    if (i >= total) break;
    lista[i].id         = p["id"]        | 0;
    lista[i].idTimeCasa = p["time_casa"] | 0;
    lista[i].idTimeFora = p["time_fora"] | 0;
    lista[i].data       = p["data"]      | "";
    i++;
  }

  sessaoReceberPartidas(lista, total);
}

bool apostasProcessarMensagem(const String& topico, const String& conteudo) {
  if (topico == TOPICO_RESULTADO_APOSTA) {
    _processarResultadoAposta(conteudo);
    return true;
  }
  if (topico == TOPICO_PARTIDAS) {
    _processarPartidas(conteudo);
    return true;
  }
  // Em apostasProcessarMensagem(), adicionar:
  if (topico == TOPICO_DADOS_CLIENTE) {
    _processarDadosCliente(conteudo);
    return true;
  }
  return false;
}