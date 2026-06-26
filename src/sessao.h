#pragma once

#include <Arduino.h>

// ---------------------------------------------------------------------------
// UID do único usuário autorizado — troque pelo UID real do cartão
// ---------------------------------------------------------------------------
#define RFID_AUTORIZADO "C4 D9 31 02"

struct Cliente {
  int32_t id;
  String  nomeCompleto;
  String  telefone;
  String  email;
};

// ---------------------------------------------------------------------------
// Máquina de estados
// ---------------------------------------------------------------------------
enum EstadoSessao {
  AGUARDANDO_RFID,
  ESCOLHENDO_ACAO,
  ESCOLHENDO_PARTIDA,
  ESCOLHENDO_TIME,
  CONFIRMANDO
};

// ---------------------------------------------------------------------------
// Struct de partida — preenchida quando o servidor responde em bet/partidas
// ---------------------------------------------------------------------------
struct Partida {
  int32_t id;
  int32_t idTimeCasa;
  int32_t idTimeFora;
  String  data;
};

#define VALOR_APOSTA_FIXO 10  // TODO: tornar configurável futuramente

void sessaoInit();
void sessaoAutenticar(const String& uid);
void sessaoReceberCliente(const Cliente& cliente);
void sessaoReceberPartidas(const Partida* lista, uint8_t quantidade);

// Chamados pelos botões
void sessaoAvancar();
void sessaoRecuar();
void sessaoConfirmar();

// Getter — usado por sessao.cpp e qualquer outro módulo que precisar
const Cliente& sessaoGetCliente();
bool           sessaoTemCliente();

// Getters
EstadoSessao sessaoGetEstado();
bool         sessaoAutenticada();
String       sessaoGetUID();