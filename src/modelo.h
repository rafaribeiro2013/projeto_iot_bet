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
  String nomeTimeCasa;
  String nomeTimeFora;
  String  data;
};

struct Aposta {
  int32_t id;
  String  createdAt;
  int32_t idCliente;
  int32_t idPartida;
  int32_t idTimeApostado;   // TIME_EMPATE = empate
  String nomeTimeApostado;
};

struct Produto {
  int32_t id;
  float   preco;
  String  categoria;
  String  nomeComida;
  String  descricao;
};

struct PedidoItem {
  int32_t id;
  int32_t idProduto;
  String  nomeComida;   // do JOIN feito no servidor (bet/meusPedidos)
  float   preco;        // idem
  String  status;
};

struct Copo {
  int32_t id;
  String  status;       // "em_uso" ou "disponivel"
  int32_t idCopo;
  String  rfid;
  int32_t idCliente;
  int     nivelBateria;
  int     quantidadeMl;
  float   temperaturaC;
  int     mesa;
};

#endif
