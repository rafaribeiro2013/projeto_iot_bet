#ifndef MODELO_H
#define MODELO_H

#include <Arduino.h>

// Sentinela: aposta em empate -> id_time_apostado enviado como null.
#define TIME_EMPATE INT32_MIN

struct Cliente {
  int32_t id;
  String  nomeCompleto;
  String  telefone;
  String  email;
};

struct Partida {
  int32_t id;
  int32_t idTimeCasa;
  int32_t idTimeFora;
  String  data;
};

struct Aposta {
  int32_t id;
  String  createdAt;
  int32_t idCliente;
  int32_t idPartida;
  int32_t idTimeApostado;   // TIME_EMPATE = empate
};

#endif
