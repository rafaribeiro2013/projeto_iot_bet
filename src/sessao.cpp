#include "sessao.h"
#include "apostas.h"

// ---------------------------------------------------------------------------
// Estado interno
// ---------------------------------------------------------------------------
static EstadoSessao _estado = AGUARDANDO_RFID;
static String _uidAtual = "";
static Partida _partidas[20];
static uint8_t _totalPartidas = 0;
static uint8_t _idxPartida = 0;
static uint8_t _idxTime = 0; // 0 = casa, 1 = fora
static Cliente _cliente        = {0, "", "", ""};
static bool    _clienteCarregado = false;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static void _imprimirEstado()
{
  switch (_estado)
  {
  case AGUARDANDO_RFID:
    Serial.println("[Sessao] Aguardando cartão RFID...");
    break;

  case ESCOLHENDO_ACAO:
    Serial.printf("[Sessao] Acao %s\n", _idxTime == 0 ? "> APOSTAR  consultar" : "  apostar  > CONSULTAR");
    Serial.println("[Sessao] UP/DOWN=alternar  CENTER=confirmar");
    break;

  case ESCOLHENDO_PARTIDA:
    if (_totalPartidas == 0)
    {
      Serial.println("[Sessao] Buscando partidas, aguarde...");
    }
    else
    {
      Partida &p = _partidas[_idxPartida];
      Serial.printf("[Sessao] Partida %d/%d | id:%d | casa:%d x fora:%d | %s\n",
                    _idxPartida + 1, _totalPartidas,
                    p.id, p.idTimeCasa, p.idTimeFora, p.data.c_str());
      Serial.println("[Sessao] UP=anterior  DOWN=proximo  CENTER=selecionar");
    }
    break;

  case ESCOLHENDO_TIME:
  {
    Partida &p = _partidas[_idxPartida];
    Serial.printf("[Sessao] Time: [%s casa(%d)] [%s fora(%d)]\n",
                  _idxTime == 0 ? ">" : " ", p.idTimeCasa,
                  _idxTime == 1 ? ">" : " ", p.idTimeFora);
    Serial.println("[Sessao] UP/DOWN=alternar  CENTER=confirmar");
    break;
  }

  case CONFIRMANDO:
  {
    Partida &p = _partidas[_idxPartida];
    int32_t time = _idxTime == 0 ? p.idTimeCasa : p.idTimeFora;
    Serial.println("[Sessao] --- CONFIRMAR APOSTA? ---");
    Serial.printf("[Sessao]   Partida : %d (%d x %d)\n", p.id, p.idTimeCasa, p.idTimeFora);
    Serial.printf("[Sessao]   Time    : %d\n", time);
    Serial.printf("[Sessao]   Valor   : R$%d\n", VALOR_APOSTA_FIXO);
    Serial.println("[Sessao] CENTER=apostar  UP/DOWN=cancelar");
    break;
  }
  }
}

static void _encerrarSessao()
{
  _estado = AGUARDANDO_RFID;
  _uidAtual = "";
  _totalPartidas = 0;
  _idxPartida = 0;
  _idxTime = 0;
  _cliente          = {0, "", "", ""};
_clienteCarregado = false;
  Serial.println("[Sessao] Sessão encerrada.");
  _imprimirEstado();
}

// ---------------------------------------------------------------------------
// API pública
// ---------------------------------------------------------------------------
void sessaoInit()
{
  Serial.println("[Sessao] Módulo inicializado.");
  _imprimirEstado();
}

void sessaoAutenticar(const String& uid) {
  if (_estado != AGUARDANDO_RFID) { return; }

  _uidAtual = uid;
  mqttPublicar(TOPICO_AUTENTICA_CLIENTE, uid);  // ← já existia no main, move pra cá
  Serial.println("[Sessao] Aguardando dados do cliente...");
  // estado ainda é AGUARDANDO_RFID — muda só quando dadosCliente chegar
}

void sessaoReceberCliente(const Cliente& cliente) {
  _cliente          = cliente;
  _clienteCarregado = true;
  _estado           = ESCOLHENDO_ACAO;
  _idxTime          = 0;
  Serial.println("[Sessao] Cliente carregado: " + cliente.nomeCompleto);
  _imprimirEstado();
}

void sessaoReceberPartidas(const Partida *lista, uint8_t quantidade)
{
  _totalPartidas = quantidade > 20 ? 20 : quantidade;
  for (uint8_t i = 0; i < _totalPartidas; i++)
  {
    _partidas[i] = lista[i];
  }
  _idxPartida = 0;
  Serial.printf("[Sessao] %d partida(s) carregada(s).\n", _totalPartidas);
  if (_estado == ESCOLHENDO_PARTIDA)
    _imprimirEstado();
}

void sessaoAvancar()
{
  switch (_estado)
  {
  case AGUARDANDO_RFID:
    break;
  case ESCOLHENDO_ACAO:
    _idxTime = (_idxTime + 1) % 2;
    _imprimirEstado();
    break;
  case ESCOLHENDO_PARTIDA:
    if (_totalPartidas > 0)
    {
      _idxPartida = (_idxPartida + 1) % _totalPartidas;
      _imprimirEstado();
    }
    break;
  case ESCOLHENDO_TIME:
  case CONFIRMANDO:
    // UP/DOWN no TIME e CONFIRMANDO = alterna time (ou cancela confirmação)
    if (_estado == CONFIRMANDO)
    {
      _estado = ESCOLHENDO_TIME;
      Serial.println("[Sessao] Confirmação cancelada.");
    }
    _idxTime = (_idxTime + 1) % 2;
    _imprimirEstado();
    break;
  }
}

void sessaoRecuar()
{
  switch (_estado)
  {
  case AGUARDANDO_RFID:
    break;
  case ESCOLHENDO_ACAO:
    _idxTime = (_idxTime + 1) % 2;
    _imprimirEstado();
    break;
  case ESCOLHENDO_PARTIDA:
    if (_totalPartidas > 0)
    {
      _idxPartida = (_idxPartida == 0) ? _totalPartidas - 1 : _idxPartida - 1;
      _imprimirEstado();
    }
    break;
  case ESCOLHENDO_TIME:
  case CONFIRMANDO:
    if (_estado == CONFIRMANDO)
    {
      _estado = ESCOLHENDO_TIME;
      Serial.println("[Sessao] Confirmação cancelada.");
    }
    _idxTime = (_idxTime + 1) % 2;
    _imprimirEstado();
    break;
  }
}

void sessaoConfirmar()
{
  switch (_estado)
  {
  case AGUARDANDO_RFID:
    break;

  case ESCOLHENDO_ACAO:
    if (_idxTime == 0)
    {
      _estado = ESCOLHENDO_PARTIDA;
      _idxTime = 0;
      getPartidas();
      delay(3000);
      _imprimirEstado();
    }
    else
    {
      apostasConsultar(_cliente.id);
      _encerrarSessao();
    }
    break;

  case ESCOLHENDO_PARTIDA:
    if (_totalPartidas == 0)
    {
      Serial.println("[Sessao] Nenhuma partida disponível ainda.");
      return;
    }
    _estado = ESCOLHENDO_TIME;
    _idxTime = 0;
    _imprimirEstado();
    break;

  case ESCOLHENDO_TIME:
    _estado = CONFIRMANDO;
    _imprimirEstado();
    break;

  case CONFIRMANDO:
  {
    Partida &p = _partidas[_idxPartida];
    Aposta aposta;
    aposta.id = 0;
    aposta.createdAt = "";
    aposta.idCliente = _cliente.id; // TODO: mapear UID → id_cliente real
    aposta.idPartida = p.id;
    aposta.idTimeApostado = (_idxTime == 0) ? p.idTimeCasa : p.idTimeFora;

    apostasRealizar(aposta);
    Serial.println("[Sessao] Aposta enviada! Encerrando sessão.");
    _encerrarSessao();
    break;
  }
  }
}

EstadoSessao sessaoGetEstado() { return _estado; }
bool sessaoAutenticada() { return _estado != AGUARDANDO_RFID; }
String sessaoGetUID() { return _uidAtual; }
const Cliente& sessaoGetCliente()  { return _cliente; }
bool           sessaoTemCliente()  { return _clienteCarregado; }