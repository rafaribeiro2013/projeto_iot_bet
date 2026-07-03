#ifndef NAVEGACAO_H
#define NAVEGACAO_H

#include "globais.h"
#include "estado_dados.h"
#include "apostas.h"
#include "dados.h"
#include "desenhos.h"

// ===========================================================================
// MOTOR DE NAVEGACAO
// ---------------------------------------------------------------------------
// Estado em um struct simples. Uma pilha guarda as telas anteriores para que
// "voltar" funcione de forma generica (sem mapear pais manualmente).
//
//   renderizarTelaAtual() -> desenha a tela do estado atual (busca dados aqui)
//   selecionar()          -> botao do meio: age sobre o item no cursor
//   voltar()              -> botao do meio SEGURADO: volta uma tela
//   moverCursor(dir)      -> setas: move o cursor (grade no menu, lista no resto)
// ===========================================================================

enum Tela {
  AGUARDANDO_CARTAO, CARREGANDO,
  INICIAL, MENU, CARDAPIO, LISTA_PRODUTOS, DETALHE_PRODUTO, CONTROLE_CERVEJA,
  LISTA_JOGOS, JOGO, CONFIRMACAO, CONFIRMADO,
  MENU_PEDIDOS, MEUS_PEDIDOS, PAGAR_CONTA, MINHAS_APOSTAS
};

// Direcoes do cursor
enum Direcao { CIMA, BAIXO, ESQUERDA, DIREITA };

struct EstadoTela {
  Tela tipo;
  int  indice;       // cursor da tela atual
  char categoria[16]; // contexto: categoria do cardapio (LISTA_PRODUTOS/DETALHE)
  int  jogoIndice;   // contexto: jogo escolhido (JOGO/CONFIRMACAO/CONFIRMADO)
  int  palpite;      // contexto: palpite escolhido (CONFIRMACAO/CONFIRMADO)
};

EstadoTela estado = { AGUARDANDO_CARTAO, 0, "", 0, 0 };

char msgCarregando[24] = "";

// Pilha de navegacao
EstadoTela pilha[8];
int pilhaTopo = 0;

// Instante em que a tela CONFIRMADO foi mostrada (para voltar ao menu em 10s).
unsigned long instanteConfirmado = 0;

void empilhar() {
  if (pilhaTopo < 8) pilha[pilhaTopo++] = estado;
}

// ---------------------------------------------------------------------------
// Renderizacao: olha o estado atual, busca os dados necessarios no repositorio
// (dados.h) e chama a funcao de desenho correspondente.
// ---------------------------------------------------------------------------
void renderizarTelaAtual() {
  switch (estado.tipo) {
    case AGUARDANDO_CARTAO:
      desenharAguardandoCartao();
      if (clienteCarregado) { estado.tipo = MENU; estado.indice = 0; desenharMenu(0); }
      break;

    case CARREGANDO:
      desenharCarregando(msgCarregando);
      if (clienteCarregado && estado.indice == 0) {
        estado.tipo = MENU; estado.indice = 0; desenharMenu(0);
      } else if (partidasProntas && estado.indice == 1) {
        estado.tipo = LISTA_JOGOS; estado.indice = 0; renderizarTelaAtual();
      } else if (apostasProntas && estado.indice == 2) {
        estado.tipo = MINHAS_APOSTAS; estado.indice = 0; renderizarTelaAtual();
      }
      break;

    case INICIAL:
      desenharTelaInicial();
      break;

    case MENU:
      desenharMenu(estado.indice);
      break;

    case CARDAPIO:
      desenharCardapio(estado.indice);
      break;

    case LISTA_PRODUTOS: {
      JsonDocument doc;
      JsonArray produtos = doc.to<JsonArray>();
      obterProdutos(estado.categoria, produtos);
      // titulo amigavel da categoria
      const char* titulo = "Cardapio";
      if (strcmp(estado.categoria, "drinks") == 0)             titulo = "Drinks";
      else if (strcmp(estado.categoria, "petiscos") == 0)      titulo = "Petiscos";
      else if (strcmp(estado.categoria, "nao alcoolico") == 0) titulo = "Nao Alcoolico";
      desenharListaProdutos(titulo, produtos, estado.indice);
      break;
    }

    case DETALHE_PRODUTO: {
      JsonDocument doc;
      JsonArray produtos = doc.to<JsonArray>();
      obterProdutos(estado.categoria, produtos);
      JsonObject p = produtos[estado.indice];
      const char* titulo = "Cardapio";
      if (strcmp(estado.categoria, "drinks") == 0)             titulo = "Drinks";
      else if (strcmp(estado.categoria, "petiscos") == 0)      titulo = "Petiscos";
      else if (strcmp(estado.categoria, "nao alcoolico") == 0) titulo = "Nao Alcoolico";
      desenharDetalheProduto(titulo, p["nome"].as<const char*>(),
                             p["preco"].as<int>(), p["descricao"].as<const char*>());
      break;
    }

    case CONTROLE_CERVEJA: {
      int gasto, copos, gratis;
      obterConsumoCerveja(gasto, copos, gratis);
      desenharControleCerveja(gasto, copos, gratis);
      break;
    }

    case LISTA_JOGOS: {
      JsonDocument doc;
      JsonArray jogos = doc.to<JsonArray>();
      obterJogos(jogos);
      desenharListaJogos(jogos, estado.indice);
      break;
    }

    case JOGO: {
      JsonDocument doc;
      JsonArray jogos = doc.to<JsonArray>();
      obterJogos(jogos);
      desenharJogo(jogos[estado.jogoIndice], estado.indice);
      break;
    }

    case CONFIRMACAO: {
      JsonDocument doc;
      JsonArray jogos = doc.to<JsonArray>();
      obterJogos(jogos);
      desenharConfirmacao(jogos[estado.jogoIndice], estado.palpite);
      break;
    }

    case CONFIRMADO: {
      JsonDocument doc;
      JsonArray jogos = doc.to<JsonArray>();
      obterJogos(jogos);
      desenharConfirmado(jogos[estado.jogoIndice], estado.palpite);
      break;
    }

    case MENU_PEDIDOS:
      desenharMenuPedidos(estado.indice);
      break;

    case MEUS_PEDIDOS: {
      JsonDocument doc;
      JsonArray pedidos = doc.to<JsonArray>();
      obterPedidos(pedidos);
      desenharMeusPedidos(pedidos, obterTotalConta());
      break;
    }

    case PAGAR_CONTA:
      desenharPagarConta(obterTotalConta());
      break;

    case MINHAS_APOSTAS: {
      JsonDocument doc;
      JsonArray apostas = doc.to<JsonArray>();
      obterApostas(apostas);
      desenharMinhasApostas(apostas);
      break;
    }
  }
}

// Vai para uma nova tela empilhando a atual (para o "voltar" funcionar).
void irPara(Tela t) {
  empilhar();
  estado.tipo = t;
  estado.indice = 0;
  renderizarTelaAtual();
}

// Volta ao menu limpando a pilha (usado apos confirmar a aposta).
void irParaMenu() {
  pilhaTopo = 0;
  estado.tipo = MENU;
  estado.indice = 0;
  renderizarTelaAtual();
}

// ---------------------------------------------------------------------------
// Selecionar (botao do meio): decide o proximo passo a partir do cursor.
// ---------------------------------------------------------------------------
void voltar();

void selecionar() {
  switch (estado.tipo) {
    case INICIAL:
      irPara(MENU);
      break;

    case MENU:
      if (estado.indice == 0) {
        irPara(CARDAPIO);
      } else if (estado.indice == 1) {
        getPartidas();
        strncpy(msgCarregando, "Buscando jogos", sizeof(msgCarregando) - 1);
        empilhar();
        estado.tipo = CARREGANDO; estado.indice = 1;
        renderizarTelaAtual();
      } else if (estado.indice == 2) {
        irPara(MENU_PEDIDOS);
      } else if (estado.indice == 3) {
        apostasConsultar(clienteAtual.id);
        strncpy(msgCarregando, "Buscando apostas", sizeof(msgCarregando) - 1);
        empilhar();
        estado.tipo = CARREGANDO; estado.indice = 2;
        renderizarTelaAtual();
      }
      break;

    case CARDAPIO:
      if (estado.indice == 0) {
        irPara(CONTROLE_CERVEJA);          // Cervejas: monitoramento, sem produtos
      } else {
        const char* cat = (estado.indice == 1) ? "drinks"
                        : (estado.indice == 2) ? "petiscos"
                        : "nao alcoolico";
        empilhar();
        estado.tipo = LISTA_PRODUTOS;
        strncpy(estado.categoria, cat, sizeof(estado.categoria) - 1);
        estado.categoria[sizeof(estado.categoria) - 1] = '\0';
        estado.indice = 0;
        renderizarTelaAtual();
      }
      break;

    case LISTA_PRODUTOS:
      // abre o detalhe do produto no cursor (mantem categoria e indice)
      empilhar();
      estado.tipo = DETALHE_PRODUTO;
      renderizarTelaAtual();
      break;

    case DETALHE_PRODUTO:
      // Botao PEDIR. MOCK: aqui entrara o registro do pedido no banco.
      Serial.println("PEDIR (a integrar com o banco)");
      voltar();
      break;

    case LISTA_JOGOS:
      // entra no jogo selecionado; o cursor passa a ser o palpite (0..2)
      empilhar();
      estado.jogoIndice = estado.indice;
      estado.tipo = JOGO;
      estado.indice = 0;
      renderizarTelaAtual();
      break;

    case JOGO:
      // confirma o palpite escolhido
      empilhar();
      estado.palpite = estado.indice;
      estado.tipo = CONFIRMACAO;
      estado.indice = 0;
      renderizarTelaAtual();
      break;

    case CONFIRMACAO: {
      // Registra a aposta e vai para a tela de sucesso.
      JsonDocument doc;
      JsonArray jogos = doc.to<JsonArray>();
      obterJogos(jogos);
      int jogoId = jogos[estado.jogoIndice]["id"].as<int>();
      registrarAposta(jogoId, estado.palpite);   // MOCK: grava no banco no id do usuario

      estado.tipo = CONFIRMADO;
      renderizarTelaAtual();
      instanteConfirmado = millis();             // dispara o retorno automatico em 10s
      break;
    }

    case MENU_PEDIDOS:
      if (estado.indice == 0) irPara(MEUS_PEDIDOS);
      else                    irPara(PAGAR_CONTA);
      break;

    // Telas apenas de visualizacao: o meio nao faz nada (volta-se segurando).
    case CONTROLE_CERVEJA:
    case CONFIRMADO:
    case MEUS_PEDIDOS:
    case PAGAR_CONTA:
    case MINHAS_APOSTAS:
    default:
      break;
  }
}

// ---------------------------------------------------------------------------
// Voltar (botao do meio segurado): desempilha a tela anterior.
// ---------------------------------------------------------------------------
void voltar() {
  if (pilhaTopo > 0) {
    estado = pilha[--pilhaTopo];
    renderizarTelaAtual();
  }
}

// ---------------------------------------------------------------------------
// Mover cursor (setas). A regra muda conforme a tela.
// ---------------------------------------------------------------------------
void moverCursor(Direcao dir) {
  switch (estado.tipo) {

    case MENU: {  // grade 2x2: 0=cardapio 1=apostar / 2=pedidos 3=minhas apostas
      if (dir == CIMA && estado.indice >= 2)        estado.indice -= 2;
      else if (dir == BAIXO && estado.indice <= 1)  estado.indice += 2;
      else if (dir == ESQUERDA && (estado.indice % 2) == 1) estado.indice -= 1;
      else if (dir == DIREITA && (estado.indice % 2) == 0)  estado.indice += 1;
      else return;
      renderizarTelaAtual();
      break;
    }

    case CARDAPIO: {  // lista de 4
      if (dir == CIMA && estado.indice > 0)        estado.indice--;
      else if (dir == BAIXO && estado.indice < 3)  estado.indice++;
      else return;
      renderizarTelaAtual();
      break;
    }

    case LISTA_PRODUTOS: {  // lista de tamanho dinamico
      JsonDocument doc;
      JsonArray produtos = doc.to<JsonArray>();
      obterProdutos(estado.categoria, produtos);
      int n = produtos.size();
      if (dir == CIMA && estado.indice > 0)          estado.indice--;
      else if (dir == BAIXO && estado.indice < n - 1) estado.indice++;
      else return;
      renderizarTelaAtual();
      break;
    }

    case LISTA_JOGOS: {  // lista de tamanho dinamico
      JsonDocument doc;
      JsonArray jogos = doc.to<JsonArray>();
      obterJogos(jogos);
      int n = jogos.size();
      if (dir == CIMA && estado.indice > 0)          estado.indice--;
      else if (dir == BAIXO && estado.indice < n - 1) estado.indice++;
      else return;
      renderizarTelaAtual();
      break;
    }

    case JOGO: {  // 3 palpites
      if (dir == CIMA && estado.indice > 0)        estado.indice--;
      else if (dir == BAIXO && estado.indice < 2)  estado.indice++;
      else return;
      renderizarTelaAtual();
      break;
    }

    case MENU_PEDIDOS: {  // lista de 2
      if (dir == CIMA && estado.indice > 0)        estado.indice--;
      else if (dir == BAIXO && estado.indice < 1)  estado.indice++;
      else return;
      renderizarTelaAtual();
      break;
    }

    default:
      break;  // telas sem cursor
  }
}

// ---------------------------------------------------------------------------
// Handlers ligados aos botoes (assinatura exigida pela lib GFButton).
// Cada interacao reinicia o contador de inatividade (definido em tela.cpp).
// ---------------------------------------------------------------------------
extern unsigned long instanteAnterior;
void registrarAtividade() { instanteAnterior = millis(); }

// No MENU as setas navegam a grade 2x2 e o meio seleciona.
// Nas demais telas: cima/baixo movem o cursor, esquerda VOLTA e direita AVANCA.
void botaoMidPressionado(GFButton& b)   { registrarAtividade(); selecionar(); }
void botaoUpPressionado(GFButton& b)    { registrarAtividade(); moverCursor(CIMA); }
void botaoDownPressionado(GFButton& b)  { registrarAtividade(); moverCursor(BAIXO); }

void botaoLeftPressionado(GFButton& b) {
  registrarAtividade();
  if (estado.tipo == MENU) moverCursor(ESQUERDA);
  else                     voltar();
}

void botaoRightPressionado(GFButton& b) {
  registrarAtividade();
  if (estado.tipo == MENU) moverCursor(DIREITA);
  else                     selecionar();
}

#endif
