#ifndef ESTADO_DADOS_H
#define ESTADO_DADOS_H

#include "modelo.h"

// Buffers de cache preenchidos pelos callbacks do MQTT (apostas.cpp) e lidos
// pela camada de dados (dados.h). Definidos em main.cpp.
extern Cliente clienteAtual;
extern bool    clienteCarregado;

extern Partida partidas[20];
extern uint8_t totalPartidas;
extern bool    partidasProntas;

extern Aposta  apostas[20];
extern uint8_t totalApostas;
extern bool    apostasProntas;

extern Produto produtos[20];
extern uint8_t totalProdutos;
extern bool    produtosProntos;

extern PedidoItem meusPedidos[20];
extern uint8_t    totalMeusPedidos;
extern bool       meusPedidosProntos;

extern Copo copoAtual;
extern bool copoProntos;

extern float totalContaAtual;
extern bool  totalContaProntas;

// UID do cartao lido na autenticacao, reaproveitado para consultar o copo do cliente.
extern String rfidAtual;

// Ligada pelos callbacks quando chega dado novo; o loop() a observa e redesenha.
extern volatile bool precisaRedesenhar;

#endif
