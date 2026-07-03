#ifndef DESENHOS_H
#define DESENHOS_H

#include "globais.h"

// ===========================================================================
// CAMADA DE DESENHO
// ---------------------------------------------------------------------------
// Funcoes PURAS: recebem os dados e o indice do cursor por parametro e apenas
// desenham. Nao conhecem o estado da navegacao nem de onde vieram os dados.
// Cada funcao desenha a tela inteira e ja envia para o display.
// ===========================================================================

// Estima um X para centralizar o texto, sem usar getUTF8Width (restrito).
// larguraChar e a largura media aproximada de um caractere na fonte usada.
int centralizar(const char* txt, int larguraChar) {
  int x = (296 - (int) strlen(txt) * larguraChar) / 2;
  return x < 0 ? 0 : x;
}

// Cursor de lista: triangulo cheio a esquerda da linha 'i' (0..3).
void cursorLista(int i) {
  int topo = 24 + i * 28;
  if (i == 3) topo = 107;            // ultima linha encosta na borda
  tela.fillTriangle(4, topo, 4, topo + 16, 14, topo + 8, GxEPD_BLACK);
}

// Setas de "entrar" no lado direito, uma por linha existente.
void setasEntrar(int n) {
  for (int i = 0; i < n; i++) {
    int topo = 24 + i * 28;
    if (i == 3) topo = 107;
    tela.fillTriangle(272, topo, 272, topo + 16, 282, topo + 8, GxEPD_BLACK);
  }
}

void desenharTelaInicial() {
  tela.fillScreen(GxEPD_WHITE);

  fontes.setFont(u8g2_font_helvB24_te);
  fontes.setFontMode(1);
  fontes.setCursor(100, 30);
  fontes.print("BetBar");

  tela.drawCircle(20, 50, 7, GxEPD_BLACK);
  tela.fillCircle(20, 50, 2, GxEPD_BLACK);
  tela.drawLine(20, 50, 20, 43, GxEPD_BLACK);
  tela.drawLine(20, 50, 27, 48, GxEPD_BLACK);
  tela.drawLine(20, 50, 24, 56, GxEPD_BLACK);
  tela.drawLine(20, 50, 16, 56, GxEPD_BLACK);
  tela.drawLine(20, 50, 13, 48, GxEPD_BLACK);

  fontes.setFont(u8g2_font_helvB14_te);
  fontes.setFontMode(1);
  fontes.setCursor(35, 56);
  fontes.print("Aposte e ganhe cerveja");

  tela.drawRect(263, 42, 12, 17, GxEPD_BLACK);
  tela.fillRect(264, 47, 10, 10, GxEPD_BLACK);
  tela.drawRect(275, 47, 5, 8, GxEPD_BLACK);

  tela.drawLine(10, 67, 286, 67, GxEPD_BLACK);

  fontes.setFont(u8g2_font_helvB12_te);
  fontes.setFontMode(1);
  fontes.setCursor(30, 105);
  fontes.print("Aperte o botao para comecar!");

  tela.display(true);
}

// indice: 0=Cardapio 1=Apostar 2=Pedidos 3=Minhas Apostas (grade 2x2)
void desenharMenu(int indice) {
  tela.fillScreen(GxEPD_WHITE);

  fontes.setFont(u8g2_font_helvB12_te);
  fontes.setFontMode(1);
  fontes.setCursor(128, 14);
  fontes.print("MENU");
  tela.drawLine(0, 18, 296, 18, GxEPD_BLACK);

  tela.drawLine(148, 19, 148, 127, GxEPD_BLACK);
  tela.drawLine(0, 73, 296, 73, GxEPD_BLACK);

  if (indice == 0)      tela.drawRect(3, 22, 142, 48, GxEPD_BLACK);
  else if (indice == 1) tela.drawRect(152, 22, 142, 48, GxEPD_BLACK);
  else if (indice == 2) tela.drawRect(3, 78, 142, 48, GxEPD_BLACK);
  else if (indice == 3) tela.drawRect(152, 78, 142, 48, GxEPD_BLACK);

  fontes.setFont(u8g2_font_open_iconic_all_4x_t);
  fontes.setFontMode(1);
  fontes.setCursor(58, 54);  fontes.print("a");
  fontes.setCursor(206, 54); fontes.print("X");
  fontes.setCursor(58, 108); fontes.print("q");
  fontes.setCursor(206, 108); fontes.print("z");

  fontes.setFont(u8g2_font_helvB12_te);
  fontes.setFontMode(1);
  fontes.setCursor(46, 66);  fontes.print("Cardapio");
  fontes.setCursor(198, 66); fontes.print("Apostar");
  fontes.setCursor(49, 120); fontes.print("Pedidos");
  fontes.setCursor(165, 120); fontes.print("Minhas Apostas");

  tela.display(true);
}

// indice: 0=Cervejas 1=Drinks 2=Petiscos 3=Nao Alcoolico (lista)
void desenharCardapio(int indice) {
  tela.fillScreen(GxEPD_WHITE);

  fontes.setFont(u8g2_font_helvB12_te);
  fontes.setFontMode(1);
  fontes.setCursor(105, 14);
  fontes.print("Cardápio");
  tela.drawLine(0, 18, 296, 18, GxEPD_BLACK);

  tela.drawLine(0, 46, 296, 46, GxEPD_BLACK);
  tela.drawLine(0, 74, 296, 74, GxEPD_BLACK);
  tela.drawLine(0, 102, 296, 102, GxEPD_BLACK);

  int yc[] = {32, 60, 88, 116};
  tela.fillCircle(12, yc[indice], 4, GxEPD_BLACK);

  setasEntrar(4);

  fontes.setFont(u8g2_font_helvB14_te);
  fontes.setFontMode(1);
  fontes.setCursor(22, 38);  fontes.print("Cervejas");
  fontes.setCursor(22, 66);  fontes.print("Drinks");
  fontes.setCursor(22, 94);  fontes.print("Petiscos");
  fontes.setCursor(22, 122); fontes.print("Não Alcoólico");

  tela.display(true);
}

// Lista de produtos de uma categoria, montada dinamicamente a partir do JSON.
void desenharListaProdutos(const char* titulo, JsonArray produtos, int indice) {
  tela.fillScreen(GxEPD_WHITE);

  fontes.setFont(u8g2_font_helvB12_te);
  fontes.setFontMode(1);
  fontes.setCursor(centralizar(titulo, 8), 14);
  fontes.print(titulo);
  tela.drawLine(0, 18, 296, 18, GxEPD_BLACK);

  tela.drawLine(0, 46, 296, 46, GxEPD_BLACK);
  tela.drawLine(0, 74, 296, 74, GxEPD_BLACK);
  tela.drawLine(0, 102, 296, 102, GxEPD_BLACK);

  int n = produtos.size();
  if (n > 4) n = 4;

  cursorLista(indice);
  setasEntrar(n);

  int yb[] = {38, 66, 94, 122};
  for (int i = 0; i < n; i++) {
    JsonObject p = produtos[i];
    fontes.setFont(u8g2_font_helvB14_te);
    fontes.setFontMode(1);
    fontes.setCursor(22, yb[i]);
    fontes.print(p["nome"].as<const char*>());

    char preco[12];
    snprintf(preco, sizeof(preco), "R$%d", p["preco"].as<int>());
    fontes.setFont(u8g2_font_helvB12_te);
    fontes.setFontMode(1);
    fontes.setCursor(218, yb[i]);
    fontes.print(preco);
  }

  tela.display(true);
}

// Tela de detalhe GENERICA: serve para qualquer produto.
void desenharDetalheProduto(const char* titulo, const char* nome, int preco, const char* descricao) {
  tela.fillScreen(GxEPD_WHITE);

  fontes.setFont(u8g2_font_helvB12_te);
  fontes.setFontMode(1);
  fontes.setCursor(centralizar(titulo, 8), 14);
  fontes.print(titulo);
  tela.drawLine(0, 18, 296, 18, GxEPD_BLACK);

  fontes.setFont(u8g2_font_helvB18_te);
  fontes.setFontMode(1);
  fontes.setCursor(10, 46);
  fontes.print(nome);

  fontes.setFont(u8g2_font_helvB12_te);
  fontes.setFontMode(1);
  fontes.setCursor(10, 63);
  fontes.print(descricao);

  tela.drawLine(10, 71, 286, 71, GxEPD_BLACK);

  char precoTxt[16];
  snprintf(precoTxt, sizeof(precoTxt), "R$ %d,00", preco);
  fontes.setFont(u8g2_font_helvB18_te);
  fontes.setFontMode(1);
  fontes.setCursor(10, 94);
  fontes.print(precoTxt);

  tela.drawRect(67, 104, 162, 22, GxEPD_BLACK);
  tela.drawRect(69, 106, 158, 18, GxEPD_BLACK);

  fontes.setFont(u8g2_font_helvB14_te);
  fontes.setFontMode(1);
  fontes.setCursor(126, 120);
  fontes.print("PEDIR");

  tela.display(true);
}

// Monitoramento de consumo de cerveja (categoria Cervejas).
void desenharControleCerveja(int gastoCentavos, int copos, int cervejasGratis) {
  tela.fillScreen(GxEPD_WHITE);

  fontes.setFont(u8g2_font_helvB12_te);
  fontes.setFontMode(1);
  fontes.setCursor(110, 14);
  fontes.print("Cervejas");
  tela.drawLine(0, 18, 296, 18, GxEPD_BLACK);

  fontes.setCursor(10, 33);
  fontes.print("Consumo da mesa");
  tela.drawLine(10, 38, 286, 38, GxEPD_BLACK);

  char gasto[16];
  snprintf(gasto, sizeof(gasto), "R$ %d,%02d", gastoCentavos / 100, gastoCentavos % 100);
  fontes.setFont(u8g2_font_helvB24_te);
  fontes.setFontMode(1);
  fontes.setCursor(88, 64);
  fontes.print(gasto);

  char coposTxt[24];
  snprintf(coposTxt, sizeof(coposTxt), "%d copos consumidos", copos);
  fontes.setFont(u8g2_font_helvB12_te);
  fontes.setFontMode(1);
  fontes.setCursor(10, 80);
  fontes.print(coposTxt);

  tela.drawLine(10, 88, 286, 88, GxEPD_BLACK);

  fontes.setFont(u8g2_font_open_iconic_all_4x_t);
  fontes.setFontMode(1);
  fontes.setCursor(8, 121);
  fontes.print("V");

  char gratis[24];
  snprintf(gratis, sizeof(gratis), "%d cervejas gratis!", cervejasGratis);
  fontes.setFont(u8g2_font_helvB14_te);
  fontes.setFontMode(1);
  fontes.setCursor(48, 112);
  fontes.print(gratis);

  tela.display(true);
}

// Lista de jogos, montada dinamicamente a partir do JSON.
void desenharListaJogos(JsonArray jogos, int indice) {
  tela.fillScreen(GxEPD_WHITE);

  fontes.setFont(u8g2_font_helvB12_te);
  fontes.setFontMode(1);
  fontes.setCursor(123, 14);
  fontes.print("Apostas");
  tela.drawLine(0, 18, 296, 18, GxEPD_BLACK);

  tela.drawLine(0, 46, 296, 46, GxEPD_BLACK);
  tela.drawLine(0, 74, 296, 74, GxEPD_BLACK);
  tela.drawLine(0, 102, 296, 102, GxEPD_BLACK);

  int n = jogos.size();
  if (n > 4) n = 4;

  cursorLista(indice);
  setasEntrar(n);

  int yb[] = {38, 66, 94, 122};
  for (int i = 0; i < n; i++) {
    JsonObject j = jogos[i];
    fontes.setCursor(22, yb[i]);  fontes.print(j["casa"].as<const char*>());
    fontes.setCursor(136, yb[i]); fontes.print("x");
    fontes.setCursor(150, yb[i]); fontes.print(j["fora"].as<const char*>());
  }

  tela.display(true);
}

// Tela do jogo: escolha do palpite (0=casa vence, 1=empate, 2=fora vence).
void desenharJogo(JsonObject jogo, int palpiteFoco) {
  tela.fillScreen(GxEPD_WHITE);

  fontes.setFont(u8g2_font_helvB12_te);
  fontes.setFontMode(1);
  fontes.setCursor(123, 14);
  fontes.print("Apostas");
  tela.drawLine(0, 18, 296, 18, GxEPD_BLACK);

  char contexto[40];
  snprintf(contexto, sizeof(contexto), "%s  X  %s",
           jogo["casa"].as<const char*>(), jogo["fora"].as<const char*>());
  fontes.setCursor(centralizar(contexto, 8), 34);
  fontes.print(contexto);
  tela.drawLine(0, 40, 296, 40, GxEPD_BLACK);

  tela.drawLine(0, 67, 296, 67, GxEPD_BLACK);
  tela.drawLine(0, 94, 296, 94, GxEPD_BLACK);

  // cursor de palpite
  int topo[] = {45, 72, 103};
  tela.fillTriangle(4, topo[palpiteFoco], 4, topo[palpiteFoco] + 16, 14, topo[palpiteFoco] + 8, GxEPD_BLACK);

  // setas de "entrar" nas 3 opcoes
  tela.fillTriangle(272, 45, 272, 61, 282, 53, GxEPD_BLACK);
  tela.fillTriangle(272, 72, 272, 88, 282, 80, GxEPD_BLACK);
  tela.fillTriangle(272, 103, 272, 119, 282, 111, GxEPD_BLACK);

  char opcCasa[28], opcFora[28];
  snprintf(opcCasa, sizeof(opcCasa), "%s vence", jogo["casa"].as<const char*>());
  snprintf(opcFora, sizeof(opcFora), "%s vence", jogo["fora"].as<const char*>());

  fontes.setCursor(22, 59);  fontes.print(opcCasa);
  fontes.setCursor(22, 86);  fontes.print("Empate");
  fontes.setCursor(22, 117); fontes.print(opcFora);

  tela.display(true);
}

// Monta o texto do palpite (0=casa, 1=empate, 2=fora) em 'buf'.
void textoPalpite(JsonObject jogo, int palpite, char* buf, int tam) {
  if (palpite == 0)      snprintf(buf, tam, "%s vence", jogo["casa"].as<const char*>());
  else if (palpite == 1) snprintf(buf, tam, "Empate");
  else                   snprintf(buf, tam, "%s vence", jogo["fora"].as<const char*>());
}

// Resumo da aposta antes de confirmar.
void desenharConfirmacao(JsonObject jogo, int palpite) {
  tela.fillScreen(GxEPD_WHITE);

  fontes.setFont(u8g2_font_helvB12_te);
  fontes.setFontMode(1);
  fontes.setCursor(123, 14);
  fontes.print("Apostas");
  tela.drawLine(0, 18, 296, 18, GxEPD_BLACK);

  char contexto[40];
  snprintf(contexto, sizeof(contexto), "%s X %s",
           jogo["casa"].as<const char*>(), jogo["fora"].as<const char*>());
  fontes.setCursor(10, 33);
  fontes.print(contexto);

  char palp[40];
  textoPalpite(jogo, palpite, palp, sizeof(palp));
  char linha[56];
  snprintf(linha, sizeof(linha), "Aposta: %s", palp);
  fontes.setCursor(10, 49);
  fontes.print(linha);

  tela.drawLine(10, 57, 286, 57, GxEPD_BLACK);

  fontes.setFont(u8g2_font_open_iconic_all_4x_t);
  fontes.setFontMode(1);
  fontes.setCursor(8, 94);
  fontes.print("V");

  fontes.setFont(u8g2_font_helvB14_te);
  fontes.setFontMode(1);
  fontes.setCursor(48, 84);
  fontes.print("1 cerveja gratis!");

  tela.drawRect(47, 101, 202, 22, GxEPD_BLACK);
  tela.drawRect(49, 103, 198, 18, GxEPD_BLACK);
  fontes.setCursor(107, 119);
  fontes.print("CONFIRMAR");

  tela.display(true);
}

// Tela de sucesso apos confirmar a aposta.
void desenharConfirmado(JsonObject jogo, int palpite) {
  tela.fillScreen(GxEPD_WHITE);

  fontes.setFont(u8g2_font_helvB12_te);
  fontes.setFontMode(1);
  fontes.setCursor(123, 14);
  fontes.print("Apostas");
  tela.drawLine(0, 18, 296, 18, GxEPD_BLACK);

  fontes.setFont(u8g2_font_open_iconic_all_4x_t);
  fontes.setFontMode(1);
  fontes.setCursor(132, 58);
  fontes.print("V");

  fontes.setFont(u8g2_font_helvB14_te);
  fontes.setFontMode(1);
  fontes.setCursor(centralizar("Aposta registrada!", 9), 84);
  fontes.print("Aposta registrada!");

  char contexto[40];
  snprintf(contexto, sizeof(contexto), "%s x %s",
           jogo["casa"].as<const char*>(), jogo["fora"].as<const char*>());
  fontes.setFont(u8g2_font_helvB12_te);
  fontes.setFontMode(1);
  fontes.setCursor(centralizar(contexto, 8), 103);
  fontes.print(contexto);

  fontes.setCursor(centralizar("Boa sorte!", 8), 120);
  fontes.print("Boa sorte!");

  tela.display(true);
}

// indice: 0=Ver Pedidos 1=Pagar Conta
void desenharMenuPedidos(int indice) {
  tela.fillScreen(GxEPD_WHITE);

  fontes.setFont(u8g2_font_helvB12_te);
  fontes.setFontMode(1);
  fontes.setCursor(115, 14);
  fontes.print("Pedidos");
  tela.drawLine(0, 18, 296, 18, GxEPD_BLACK);

  tela.drawLine(0, 73, 296, 73, GxEPD_BLACK);

  if (indice == 0)      tela.fillTriangle(4, 37, 4, 53, 14, 45, GxEPD_BLACK);
  else if (indice == 1) tela.fillTriangle(4, 92, 4, 108, 14, 100, GxEPD_BLACK);

  tela.fillTriangle(272, 37, 272, 53, 282, 45, GxEPD_BLACK);
  tela.fillTriangle(272, 92, 272, 108, 282, 100, GxEPD_BLACK);

  fontes.setFont(u8g2_font_helvB14_te);
  fontes.setFontMode(1);
  fontes.setCursor(22, 51);
  fontes.print("Ver Pedidos");
  fontes.setCursor(22, 106);
  fontes.print("Pagar Conta");

  tela.display(true);
}

// Lista de pedidos da mesa + total, montada a partir do JSON.
void desenharMeusPedidos(JsonArray pedidos, int total) {
  tela.fillScreen(GxEPD_WHITE);

  fontes.setFont(u8g2_font_helvB12_te);
  fontes.setFontMode(1);
  fontes.setCursor(106, 14);
  fontes.print("Meus Pedidos");
  tela.drawLine(0, 18, 296, 18, GxEPD_BLACK);

  tela.drawLine(0, 46, 296, 46, GxEPD_BLACK);
  tela.drawLine(0, 74, 296, 74, GxEPD_BLACK);
  tela.drawLine(0, 102, 296, 102, GxEPD_BLACK);

  int n = pedidos.size();
  if (n > 3) n = 3;  // a 4a linha e reservada para o Total

  int yb[] = {38, 66, 94};
  for (int i = 0; i < n; i++) {
    JsonObject p = pedidos[i];
    fontes.setFont(u8g2_font_helvB14_te);
    fontes.setFontMode(1);
    fontes.setCursor(10, yb[i]);
    fontes.print(p["nome"].as<const char*>());

    char preco[12];
    snprintf(preco, sizeof(preco), "R$%d", p["preco"].as<int>());
    fontes.setFont(u8g2_font_helvB12_te);
    fontes.setFontMode(1);
    fontes.setCursor(238, yb[i]);
    fontes.print(preco);
  }

  fontes.setFont(u8g2_font_helvB12_te);
  fontes.setFontMode(1);
  fontes.setCursor(10, 122);
  fontes.print("Total:");

  char totalTxt[12];
  snprintf(totalTxt, sizeof(totalTxt), "R$%d", total);
  fontes.setFont(u8g2_font_helvB14_te);
  fontes.setFontMode(1);
  fontes.setCursor(228, 122);
  fontes.print(totalTxt);

  tela.display(true);
}

// Tela de pagamento com QR Code Pix.
void desenharPagarConta(int total) {
  tela.fillScreen(GxEPD_WHITE);

  fontes.setFont(u8g2_font_helvB12_te);
  fontes.setFontMode(1);
  fontes.setCursor(115, 14);
  fontes.print("Pedidos");
  tela.drawLine(0, 18, 296, 18, GxEPD_BLACK);

  char totalTxt[24];
  snprintf(totalTxt, sizeof(totalTxt), "Total: R$ %d,00", total);
  fontes.setCursor(centralizar(totalTxt, 8), 31);
  fontes.print(totalTxt);

  tela.drawLine(10, 38, 286, 38, GxEPD_BLACK);

  // MOCK: "https://google.com" sera o payload Pix (copia-e-cola) vindo do banco.
  qrcode.setScale(2);
  qrcode.draw("https://google.com", 119, 42);

  fontes.setCursor(106, 120);
  fontes.print("Pague via Pix");

  tela.display(true);
}

// Apostas do usuario, montadas a partir do JSON (ate 2 visiveis).
void desenharMinhasApostas(JsonArray apostas) {
  tela.fillScreen(GxEPD_WHITE);

  fontes.setFont(u8g2_font_helvB12_te);
  fontes.setFontMode(1);
  fontes.setCursor(99, 14);
  fontes.print("Minhas Apostas");
  tela.drawLine(0, 18, 296, 18, GxEPD_BLACK);

  int n = apostas.size();
  if (n > 2) n = 2;

  if (n > 1) tela.drawLine(0, 72, 296, 72, GxEPD_BLACK);

  int yJogo[]    = {43, 97};
  int yPalpite[] = {59, 113};
  for (int i = 0; i < n; i++) {
    JsonObject ap = apostas[i];

    fontes.setCursor(10, yJogo[i]);
    fontes.print(ap["jogo"].as<const char*>());

    char palp[40];
    snprintf(palp, sizeof(palp), "Apostei: %s", ap["palpite"].as<const char*>());
    fontes.setCursor(10, yPalpite[i]);
    fontes.print(palp);

    const char* status = ap["status"].as<const char*>();
    int xStatus = 296 - (int) strlen(status) * 8 - 6;
    fontes.setCursor(xStatus, yPalpite[i]);
    fontes.print(status);
  }

  tela.display(true);
}

void desenharAguardandoCartao() {
  tela.fillScreen(GxEPD_WHITE);
  fontes.setFont(u8g2_font_helvB24_te); fontes.setFontMode(1);
  fontes.setCursor(centralizar("BetBar", 20), 34);
  fontes.print("BetBar");
  tela.drawLine(10, 44, 286, 44, GxEPD_BLACK);
  // simbolo de cartao
  tela.drawRoundRect(120, 58, 56, 34, 4, GxEPD_BLACK);
  tela.fillRect(120, 66, 56, 8, GxEPD_BLACK);
  fontes.setFont(u8g2_font_helvB12_te); fontes.setFontMode(1);
  fontes.setCursor(centralizar("Encoste seu cartao", 8), 112);
  fontes.print("Encoste seu cartao");
  tela.display(true);
}

void desenharAviso(const char* msg) {
  tela.fillScreen(GxEPD_WHITE);
  // triangulo de alerta
  tela.drawTriangle(148, 30, 128, 66, 168, 66, GxEPD_BLACK);
  fontes.setFont(u8g2_font_helvB18_te); fontes.setFontMode(1);
  fontes.setCursor(144, 60); fontes.print("!");
  fontes.setFont(u8g2_font_helvB12_te); fontes.setFontMode(1);
  fontes.setCursor(centralizar(msg, 8), 92); fontes.print(msg);
  fontes.setCursor(centralizar("Esquerda para voltar", 7), 116);
  fontes.print("Esquerda para voltar");
  tela.display(true);
}

void desenharCarregando(const char* msg) {
  tela.fillScreen(GxEPD_WHITE);
  fontes.setFont(u8g2_font_helvB18_te); fontes.setFontMode(1);
  fontes.setCursor(centralizar("Carregando...", 12), 56);
  fontes.print("Carregando...");
  fontes.setFont(u8g2_font_helvB12_te); fontes.setFontMode(1);
  fontes.setCursor(centralizar(msg, 8), 84);
  fontes.print(msg);
  tela.display(true);
}

#endif
