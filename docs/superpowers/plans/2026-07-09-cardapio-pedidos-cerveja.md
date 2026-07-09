# Cardápio, Pedidos e Copo (Cerveja) — Plano de Implementação

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Substituir os dados mockados de cardápio, pedidos e consumo de cerveja por dados reais vindos do banco via MQTT, usando o mesmo padrão buffer + `precisaRedesenhar` já validado na integração de apostas.

**Architecture:** Um módulo novo (`cardapio.cpp/h`) publica pedidos MQTT e processa as respostas, enchendo buffers globais; `dados.h` passa a ler desses buffers em vez de retornar mocks; `navegacao.h` dispara os fetches assíncronos e usa a tela `CARREGANDO` já existente enquanto espera.

**Tech Stack:** ESP32-S3, PlatformIO/Arduino, ArduinoJson, MQTT (256dpi/MQTT), GxEPD2/U8g2 (ePaper).

## Global Constraints

- Display 296×128, `setRotation(3)`. Sempre `setFontMode(1)` após `setFont()`. **Nunca** usar `getUTF8Width` nem `setForegroundColor(GxEPD_WHITE)`. Centralização via `centralizar()`.
- Objetos globais **definidos** só em `main.cpp`, `extern` em headers (padrão já usado no projeto).
- Categoria no banco: `"Drinks"`, `"Petiscos"`, `"Nao Alcoolico"` (maiúscula inicial, sem acento) — exatamente assim.
- Preço: `float` no banco, mas exibido como inteiro em reais (`"R$ %d,00"`, sem centavos) — conversão em `dados.h`: `(int)(preco + 0.5f)`.
- `copos` é **estado atual** (uma linha por copo físico, sem histórico) — a tela de cerveja mostra nível/temperatura/status ao vivo, não gasto/histórico.
- `pedidos` é criada pelo time durante a aula; o firmware assume as colunas: `id, created_at, id_cliente, id_produto, status, mesa`.
- Fechamento de conta (`obterTotalConta`, tela `PAGAR_CONTA`) é responsabilidade de outro integrante — **não tocar** além de reforçar o comentário de MOCK.
- "Teste" de cada tarefa = **compilar** com `pio run`. Não há framework de testes unitários no device — a verificação funcional completa (Serial Monitor, comportamento na tela) só é possível depois que os tópicos existirem no Node-RED (feito em aula); até lá, o critério de sucesso de cada task é compilar limpo. Este projeto não segue TDD clássico (sem testes automatizados no ESP32); cada task junta as edições de arquivo e termina com um único `pio run`, seguindo o padrão já usado no plano de integração anterior.
- Trabalhar na branch `cardapio-pedidos-cerveja` (criada a partir de `main`). Um commit por tarefa.

**Spec de referência:** `docs/superpowers/specs/2026-07-09-cardapio-pedidos-cerveja-design.md`

---

## Estrutura de arquivos (após esta implementação)

```
src/
  modelo.h          + structs Produto, PedidoItem, Copo
  estado_dados.h    + buffers produtos[20]/meusPedidos[20]/copoAtual + flags + rfidAtual
  cardapio.h/.cpp   [NOVO] getCardapio, cardapioRegistrarPedido, getMeusPedidos, getCopo
                    + parsers das respostas + cardapioProcessarMensagem
  dados.h           obterProdutos/obterPedidos/obterConsumoCerveja leem os buffers;
                    registrarPedido novo; obterTotalConta comentário reforçado
  navegacao.h       enum EsperandoDado; CARDAPIO dispara getCopo/getCardapio;
                    DETALHE_PRODUTO dispara registrarPedido; MENU_PEDIDOS dispara
                    getMeusPedidos; strings de categoria corrigidas
  desenhos.h        desenharControleCerveja muda de assinatura
  main.cpp          define os buffers novos; inclui cardapio.h; despacha os tópicos
                    novos; guarda rfidAtual ao ler o cartão
```

---

## Task 0: Criar a branch de trabalho

**Files:** nenhum (operação git)

- [ ] **Step 1: Criar e ir para a branch**

```bash
cd "/c/Users/Rafael/OneDrive/PUC Rio/projeto_iot"
git checkout main
git pull
git checkout -b cardapio-pedidos-cerveja
```

- [ ] **Step 2: Confirmar a branch**

Run: `git branch --show-current`
Expected: `cardapio-pedidos-cerveja`

---

## Task 1: Modelo de dados + buffers de cache

**Files:**
- Modify: `src/modelo.h` (adicionar structs `Produto`, `PedidoItem`, `Copo`)
- Modify: `src/estado_dados.h` (adicionar buffers + `rfidAtual`)
- Modify: `src/main.cpp` (definir os buffers novos)

**Interfaces:**
- Produces:
  - `struct Produto { int32_t id; float preco; String categoria; String nomeComida; String descricao; };`
  - `struct PedidoItem { int32_t id; int32_t idProduto; String nomeComida; float preco; String status; };`
  - `struct Copo { int32_t id; String status; int32_t idCopo; String rfid; int32_t idCliente; int nivelBateria; int quantidadeMl; float temperaturaC; int mesa; };`
  - Globais: `Produto produtos[20]; uint8_t totalProdutos; bool produtosProntos;`
  - Globais: `PedidoItem meusPedidos[20]; uint8_t totalMeusPedidos; bool meusPedidosProntos;`
  - Globais: `Copo copoAtual; bool copoProntos;`
  - Global: `String rfidAtual;`

- [ ] **Step 1: Adicionar os structs em `src/modelo.h`**

Adicionar, antes do `#endif`:

```cpp
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
```

- [ ] **Step 2: Adicionar os buffers em `src/estado_dados.h`**

Adicionar, antes do `#endif`:

```cpp
extern Produto produtos[20];
extern uint8_t totalProdutos;
extern bool    produtosProntos;

extern PedidoItem meusPedidos[20];
extern uint8_t    totalMeusPedidos;
extern bool       meusPedidosProntos;

extern Copo copoAtual;
extern bool copoProntos;

// UID do cartao lido na autenticacao, reaproveitado para consultar o copo do cliente.
extern String rfidAtual;
```

- [ ] **Step 3: Definir os buffers em `src/main.cpp`**

Logo após o bloco de buffers já existente (depois de `volatile bool precisaRedesenhar = false;`), adicionar:

```cpp
Produto    produtos[20];
uint8_t    totalProdutos = 0;
bool       produtosProntos = false;

PedidoItem meusPedidos[20];
uint8_t    totalMeusPedidos = 0;
bool       meusPedidosProntos = false;

Copo copoAtual;
bool copoProntos = false;

String rfidAtual = "";
```

- [ ] **Step 4: Compilar**

Run: `pio run`
Expected: `SUCCESS`.

- [ ] **Step 5: Commit**

```bash
git add src/modelo.h src/estado_dados.h src/main.cpp
git commit -m "cardapio: modelo de dados + buffers de cache (produtos, pedidos, copo)"
```

---

## Task 2: Módulo `cardapio` (MQTT) + captura do RFID atual

**Files:**
- Create: `src/cardapio.h`, `src/cardapio.cpp`
- Modify: `src/main.cpp` (include, despacho no `mqttMensagemRecebida`, capturar `rfidAtual`)

**Interfaces:**
- Consumes: buffers de `estado_dados.h`; `mqttPublicar()` de `mqtt_module.h`.
- Produces: `void getCardapio(const char* categoria);`, `void cardapioRegistrarPedido(int32_t idCliente, int32_t idProduto);`, `void getMeusPedidos(int32_t idCliente);`, `void getCopo(const String& rfid);`, `bool cardapioProcessarMensagem(const String& topico, const String& conteudo);`

- [ ] **Step 1: Criar `src/cardapio.h`**

```cpp
#pragma once

#include <Arduino.h>
#include "modelo.h"
#include "mqtt_module.h"

#define TOPICO_GET_CARDAPIO     "bet/getCardapio"
#define TOPICO_CARDAPIO         "bet/cardapio"
#define TOPICO_REGISTRAR_PEDIDO "bet/registrarPedido"
#define TOPICO_GET_MEUS_PEDIDOS "bet/getMeusPedidos"
#define TOPICO_MEUS_PEDIDOS     "bet/meusPedidos"
#define TOPICO_GET_COPO         "bet/getCopo"
#define TOPICO_COPO             "bet/copo"

void getCardapio(const char* categoria);
void cardapioRegistrarPedido(int32_t idCliente, int32_t idProduto);
void getMeusPedidos(int32_t idCliente);
void getCopo(const String& rfid);
bool cardapioProcessarMensagem(const String& topico, const String& conteudo);
```

- [ ] **Step 2: Criar `src/cardapio.cpp`**

```cpp
#include "cardapio.h"
#include "estado_dados.h"
#include <ArduinoJson.h>

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

bool cardapioProcessarMensagem(const String& topico, const String& conteudo) {
  if (topico == TOPICO_CARDAPIO)     { _processarCardapio(conteudo);     return true; }
  if (topico == TOPICO_MEUS_PEDIDOS) { _processarMeusPedidos(conteudo);  return true; }
  if (topico == TOPICO_COPO)         { _processarCopo(conteudo);        return true; }
  return false;
}
```

- [ ] **Step 3: Incluir `cardapio.h` e despachar as mensagens em `main.cpp`**

No topo, junto aos includes:

```cpp
#include "cardapio.h"
```

Substituir o corpo de `mqttMensagemRecebida`:

```cpp
void mqttMensagemRecebida(String topico, String conteudo) {
  Serial.println("[MQTT] " + topico + ": " + conteudo);
  if (!apostasProcessarMensagem(topico, conteudo)) {
    cardapioProcessarMensagem(topico, conteudo);
  }
}
```

- [ ] **Step 4: Guardar `rfidAtual` ao ler o cartão**

No `loop()`, dentro do bloco `if (rfidNovoCartao())`, logo após `String uid = rfidLerUID();`, adicionar:

```cpp
    rfidAtual = uid;
```

- [ ] **Step 5: Compilar**

Run: `pio run`
Expected: `SUCCESS`.

- [ ] **Step 6: Commit**

```bash
git add src/cardapio.h src/cardapio.cpp src/main.cpp
git commit -m "cardapio: modulo MQTT (getCardapio, registrarPedido, getMeusPedidos, getCopo)"
```

---

## Task 3: Cerveja — monitor ao vivo do copo

Redesenha a tela "Cervejas" (hoje mock de gasto/copos/grátis) para mostrar o estado real do copo do cliente. Introduz o `enum EsperandoDado`, substituindo os números mágicos usados em `CARREGANDO`.

**Files:**
- Modify: `src/navegacao.h` (enum `EsperandoDado`; inclui `cardapio.h`; `CARDAPIO` índice 0 dispara `getCopo`; `CARREGANDO` trata `ESPERANDO_COPO`)
- Modify: `src/main.cpp` (usa `ESPERANDO_CLIENTE` em vez do número mágico `0`)
- Modify: `src/dados.h` (`obterConsumoCerveja` nova assinatura)
- Modify: `src/desenhos.h` (`desenharControleCerveja` nova assinatura)

**Interfaces:**
- Consumes: `copoAtual`, `copoProntos`, `rfidAtual` de `estado_dados.h`; `getCopo()` de `cardapio.h`.
- Produces: `enum EsperandoDado { ESPERANDO_CLIENTE, ESPERANDO_PARTIDAS, ESPERANDO_APOSTAS, ESPERANDO_PRODUTOS, ESPERANDO_MEUS_PEDIDOS, ESPERANDO_COPO };`

- [ ] **Step 1: Incluir `cardapio.h` e adicionar o enum em `src/navegacao.h`**

No topo, junto aos includes:

```cpp
#include "cardapio.h"
```

Logo após `enum Direcao { CIMA, BAIXO, ESQUERDA, DIREITA };`, adicionar:

```cpp
// Marca o que a tela CARREGANDO esta esperando (usado em estado.indice).
enum EsperandoDado {
  ESPERANDO_CLIENTE, ESPERANDO_PARTIDAS, ESPERANDO_APOSTAS,
  ESPERANDO_PRODUTOS, ESPERANDO_MEUS_PEDIDOS, ESPERANDO_COPO
};
```

- [ ] **Step 2: Trocar os números mágicos existentes pelos nomes do enum**

Em `renderizarTelaAtual()`, o case `CARREGANDO` inteiro passa a ser:

```cpp
    case CARREGANDO:
      desenharCarregando(msgCarregando);
      if (clienteCarregado && estado.indice == ESPERANDO_CLIENTE) {
        estado.tipo = MENU; estado.indice = 0; desenharMenu(0);
      } else if (partidasProntas && estado.indice == ESPERANDO_PARTIDAS) {
        estado.tipo = LISTA_JOGOS; estado.indice = 0; renderizarTelaAtual();
      } else if (apostasProntas && estado.indice == ESPERANDO_APOSTAS) {
        estado.tipo = MINHAS_APOSTAS; estado.indice = 0; renderizarTelaAtual();
      } else if (copoProntos && estado.indice == ESPERANDO_COPO) {
        estado.tipo = CONTROLE_CERVEJA; estado.indice = 0; renderizarTelaAtual();
      }
      break;
```

Em `selecionar()`, `case MENU`, trocar `estado.indice = 1;` (Apostar) por `estado.indice = ESPERANDO_PARTIDAS;` e `estado.indice = 2;` (Minhas Apostas) por `estado.indice = ESPERANDO_APOSTAS;`.

Em `src/main.cpp`, no bloco de leitura do RFID (`if (rfidNovoCartao())`), trocar `estado.indice = 0;` por `estado.indice = ESPERANDO_CLIENTE;`.

- [ ] **Step 3: `obterConsumoCerveja` nova assinatura em `src/dados.h`**

Substituir a função inteira:

```cpp
// Estado ao vivo do copo do cliente (tela de monitoramento da categoria Cervejas).
void obterConsumoCerveja(int& quantidadeMl, float& temperaturaC, String& status, int& nivelBateria) {
  quantidadeMl  = copoAtual.quantidadeMl;
  temperaturaC  = copoAtual.temperaturaC;
  status        = copoAtual.status;
  nivelBateria  = copoAtual.nivelBateria;
}
```

- [ ] **Step 4: `desenharControleCerveja` nova assinatura em `src/desenhos.h`**

Substituir a função inteira (mesma posição, mesmo nome):

```cpp
// Monitor ao vivo do copo do cliente (categoria Cervejas).
void desenharControleCerveja(int quantidadeMl, float temperaturaC, const char* status, int nivelBateria) {
  tela.fillScreen(GxEPD_WHITE);

  fontes.setFont(u8g2_font_helvB12_te);
  fontes.setFontMode(1);
  fontes.setCursor(105, 14);
  fontes.print("Seu Copo");
  tela.drawLine(0, 18, 296, 18, GxEPD_BLACK);

  char nivel[20];
  snprintf(nivel, sizeof(nivel), "%d ml", quantidadeMl);
  fontes.setFont(u8g2_font_helvB24_te);
  fontes.setFontMode(1);
  fontes.setCursor(centralizar(nivel, 16), 56);
  fontes.print(nivel);

  tela.drawLine(10, 66, 286, 66, GxEPD_BLACK);

  char temp[20];
  snprintf(temp, sizeof(temp), "Temp: %.1f C", temperaturaC);
  fontes.setFont(u8g2_font_helvB12_te);
  fontes.setFontMode(1);
  fontes.setCursor(10, 84);
  fontes.print(temp);

  char statusTxt[24];
  snprintf(statusTxt, sizeof(statusTxt), "Status: %s", status);
  fontes.setCursor(10, 102);
  fontes.print(statusTxt);

  char bateria[20];
  snprintf(bateria, sizeof(bateria), "Bateria: %d%%", nivelBateria);
  fontes.setCursor(10, 120);
  fontes.print(bateria);

  tela.display(true);
}
```

- [ ] **Step 5: Ligar a tela em `src/navegacao.h`**

No `case CONTROLE_CERVEJA` de `renderizarTelaAtual()`, substituir o corpo:

```cpp
    case CONTROLE_CERVEJA: {
      int quantidadeMl, nivelBateria;
      float temperaturaC;
      String status;
      obterConsumoCerveja(quantidadeMl, temperaturaC, status, nivelBateria);
      desenharControleCerveja(quantidadeMl, temperaturaC, status.c_str(), nivelBateria);
      break;
    }
```

No `case CARDAPIO` de `selecionar()`, substituir a parte do índice 0 (mantendo o `else` das categorias intocado por enquanto — será ajustado na Task 4):

```cpp
    case CARDAPIO:
      if (estado.indice == 0) {
        getCopo(rfidAtual);
        strncpy(msgCarregando, "Buscando copo", sizeof(msgCarregando) - 1);
        empilhar();
        estado.tipo = CARREGANDO; estado.indice = ESPERANDO_COPO;
        renderizarTelaAtual();
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
```

(As strings `"drinks"/"petiscos"/"nao alcoolico"` e o fluxo assíncrono das categorias serão corrigidos na Task 4 — aqui só isolamos a mudança da Cerveja.)

- [ ] **Step 6: Compilar**

Run: `pio run`
Expected: `SUCCESS`.

- [ ] **Step 7: Commit**

```bash
git add src/navegacao.h src/main.cpp src/dados.h src/desenhos.h
git commit -m "cerveja: monitor ao vivo do copo (getCopo) + enum EsperandoDado"
```

---

## Task 4: Cardápio — produtos reais + registrar pedido

**Files:**
- Modify: `src/dados.h` (`obterProdutos` lê o buffer; novo `registrarPedido`)
- Modify: `src/navegacao.h` (categorias corrigidas + fluxo assíncrono; `DETALHE_PRODUTO` dispara `registrarPedido`; `CARREGANDO` trata `ESPERANDO_PRODUTOS`)

**Interfaces:**
- Consumes: `produtos[]`, `totalProdutos`, `produtosProntos` de `estado_dados.h`; `getCardapio()`, `cardapioRegistrarPedido()` de `cardapio.h`.
- Produces: `void registrarPedido(int32_t produtoId);` em `dados.h`.

- [ ] **Step 1: `obterProdutos` lê o buffer em `src/dados.h`**

Substituir a função inteira (mesmo formato de JSON de antes, `+ "id"` — mesmo padrão de `obterJogos`):

```cpp
// Produtos de uma categoria do cardapio — lidos do buffer preenchido pelo MQTT.
// Cada item: { "id": <int>, "nome": <string>, "preco": <int>, "descricao": <string> }
void obterProdutos(const char* categoria, JsonArray destino) {
  for (uint8_t i = 0; i < totalProdutos; i++) {
    JsonObject o = destino.add<JsonObject>();
    o["id"]        = produtos[i].id;
    o["nome"]      = produtos[i].nomeComida;
    o["preco"]     = (int)(produtos[i].preco + 0.5f);
    o["descricao"] = produtos[i].descricao;
  }
}
```

- [ ] **Step 2: Novo `registrarPedido` em `src/dados.h`**

Adicionar logo após `obterProdutos`:

```cpp
// Envia o pedido real via MQTT (id do cliente autenticado + id do produto).
void registrarPedido(int32_t produtoId) {
  cardapioRegistrarPedido(clienteAtual.id, produtoId);
}
```

- [ ] **Step 3: Corrigir as strings de categoria e o fluxo assíncrono em `src/navegacao.h`**

No `case CARDAPIO` de `selecionar()` (ajustado na Task 3), substituir o `else` inteiro:

```cpp
    case CARDAPIO:
      if (estado.indice == 0) {
        getCopo(rfidAtual);
        strncpy(msgCarregando, "Buscando copo", sizeof(msgCarregando) - 1);
        empilhar();
        estado.tipo = CARREGANDO; estado.indice = ESPERANDO_COPO;
        renderizarTelaAtual();
      } else {
        const char* cat = (estado.indice == 1) ? "Drinks"
                        : (estado.indice == 2) ? "Petiscos"
                        : "Nao Alcoolico";
        strncpy(estado.categoria, cat, sizeof(estado.categoria) - 1);
        estado.categoria[sizeof(estado.categoria) - 1] = '\0';
        getCardapio(cat);
        strncpy(msgCarregando, "Buscando cardapio", sizeof(msgCarregando) - 1);
        empilhar();
        estado.tipo = CARREGANDO; estado.indice = ESPERANDO_PRODUTOS;
        renderizarTelaAtual();
      }
      break;
```

Corrigir as comparações de categoria em `renderizarTelaAtual()` — nos casos `LISTA_PRODUTOS` e `DETALHE_PRODUTO`, trocar:

```cpp
      if (strcmp(estado.categoria, "drinks") == 0)             titulo = "Drinks";
      else if (strcmp(estado.categoria, "petiscos") == 0)      titulo = "Petiscos";
      else if (strcmp(estado.categoria, "nao alcoolico") == 0) titulo = "Nao Alcoolico";
```

por (nas duas ocorrências):

```cpp
      if (strcmp(estado.categoria, "Drinks") == 0)             titulo = "Drinks";
      else if (strcmp(estado.categoria, "Petiscos") == 0)      titulo = "Petiscos";
      else if (strcmp(estado.categoria, "Nao Alcoolico") == 0) titulo = "Nao Alcoolico";
```

- [ ] **Step 4: `CARREGANDO` trata `ESPERANDO_PRODUTOS` em `src/navegacao.h`**

O case `CARREGANDO` de `renderizarTelaAtual()` (editado na Task 3) ganha mais um ramo — substituir o case inteiro:

```cpp
    case CARREGANDO:
      desenharCarregando(msgCarregando);
      if (clienteCarregado && estado.indice == ESPERANDO_CLIENTE) {
        estado.tipo = MENU; estado.indice = 0; desenharMenu(0);
      } else if (partidasProntas && estado.indice == ESPERANDO_PARTIDAS) {
        estado.tipo = LISTA_JOGOS; estado.indice = 0; renderizarTelaAtual();
      } else if (apostasProntas && estado.indice == ESPERANDO_APOSTAS) {
        estado.tipo = MINHAS_APOSTAS; estado.indice = 0; renderizarTelaAtual();
      } else if (copoProntos && estado.indice == ESPERANDO_COPO) {
        estado.tipo = CONTROLE_CERVEJA; estado.indice = 0; renderizarTelaAtual();
      } else if (produtosProntos && estado.indice == ESPERANDO_PRODUTOS) {
        estado.tipo = LISTA_PRODUTOS; estado.indice = 0; renderizarTelaAtual();
      }
      break;
```

- [ ] **Step 5: "PEDIR" chama `registrarPedido` em `src/navegacao.h`**

No `case DETALHE_PRODUTO` de `selecionar()`, substituir o corpo:

```cpp
    case DETALHE_PRODUTO: {
      JsonDocument doc;
      JsonArray produtosJson = doc.to<JsonArray>();
      obterProdutos(estado.categoria, produtosJson);
      int32_t produtoId = produtosJson[estado.indice]["id"].as<int32_t>();
      registrarPedido(produtoId);
      voltar();
      break;
    }
```

- [ ] **Step 6: Compilar**

Run: `pio run`
Expected: `SUCCESS`.

- [ ] **Step 7: Commit**

```bash
git add src/dados.h src/navegacao.h
git commit -m "cardapio: produtos reais (getCardapio) + registrar pedido real"
```

---

## Task 5: Meus Pedidos reais

**Files:**
- Modify: `src/dados.h` (`obterPedidos` lê o buffer; comentário de `obterTotalConta` reforçado)
- Modify: `src/navegacao.h` ("Meus Pedidos" dispara `getMeusPedidos`; `CARREGANDO` trata `ESPERANDO_MEUS_PEDIDOS`)

**Interfaces:**
- Consumes: `meusPedidos[]`, `totalMeusPedidos`, `meusPedidosProntos` de `estado_dados.h`; `getMeusPedidos()` de `cardapio.h`.

- [ ] **Step 1: `obterPedidos` lê o buffer em `src/dados.h`**

Substituir a função inteira:

```cpp
// Pedidos do cliente autenticado — lidos do buffer preenchido pelo MQTT.
// Cada item: { "nome": <string>, "preco": <int> }
void obterPedidos(JsonArray destino) {
  for (uint8_t i = 0; i < totalMeusPedidos; i++) {
    JsonObject o = destino.add<JsonObject>();
    o["nome"]  = meusPedidos[i].nomeComida;
    o["preco"] = (int)(meusPedidos[i].preco + 0.5f);
  }
}
```

- [ ] **Step 2: Reforçar o comentário de `obterTotalConta` em `src/dados.h`**

Substituir o comentário acima da função (a função em si não muda):

```cpp
// Total da conta (soma dos pedidos + cerveja).
// MOCK: fechamento de conta e responsabilidade de outro integrante do grupo.
// Este stub (publica em fecharConta/{mesa} e sempre retorna 0) permanece ate
// que essa parte seja implementada por quem cuida do fechamento — nao mexer
// aqui alem de aguardar essa integracao futura.
int obterTotalConta() {
```

- [ ] **Step 3: "Meus Pedidos" dispara `getMeusPedidos` em `src/navegacao.h`**

No `case MENU_PEDIDOS` de `selecionar()`, substituir o corpo:

```cpp
    case MENU_PEDIDOS:
      if (estado.indice == 0) {
        getMeusPedidos(clienteAtual.id);
        strncpy(msgCarregando, "Buscando pedidos", sizeof(msgCarregando) - 1);
        empilhar();
        estado.tipo = CARREGANDO; estado.indice = ESPERANDO_MEUS_PEDIDOS;
        renderizarTelaAtual();
      } else {
        irPara(PAGAR_CONTA);
      }
      break;
```

- [ ] **Step 4: `CARREGANDO` trata `ESPERANDO_MEUS_PEDIDOS` em `src/navegacao.h`**

Substituir o case `CARREGANDO` inteiro (mais um ramo em relação à Task 4):

```cpp
    case CARREGANDO:
      desenharCarregando(msgCarregando);
      if (clienteCarregado && estado.indice == ESPERANDO_CLIENTE) {
        estado.tipo = MENU; estado.indice = 0; desenharMenu(0);
      } else if (partidasProntas && estado.indice == ESPERANDO_PARTIDAS) {
        estado.tipo = LISTA_JOGOS; estado.indice = 0; renderizarTelaAtual();
      } else if (apostasProntas && estado.indice == ESPERANDO_APOSTAS) {
        estado.tipo = MINHAS_APOSTAS; estado.indice = 0; renderizarTelaAtual();
      } else if (copoProntos && estado.indice == ESPERANDO_COPO) {
        estado.tipo = CONTROLE_CERVEJA; estado.indice = 0; renderizarTelaAtual();
      } else if (produtosProntos && estado.indice == ESPERANDO_PRODUTOS) {
        estado.tipo = LISTA_PRODUTOS; estado.indice = 0; renderizarTelaAtual();
      } else if (meusPedidosProntos && estado.indice == ESPERANDO_MEUS_PEDIDOS) {
        estado.tipo = MEUS_PEDIDOS; estado.indice = 0; renderizarTelaAtual();
      }
      break;
```

- [ ] **Step 5: Compilar**

Run: `pio run`
Expected: `SUCCESS`.

- [ ] **Step 6: Commit**

```bash
git add src/dados.h src/navegacao.h
git commit -m "pedidos: Meus Pedidos reais (getMeusPedidos + buffer)"
```

---

## Task 6: Preparar para a aula + push

Esta tarefa não implementa Node-RED/SQL (fora do alcance do firmware) — organiza o que levar para a aula e publica a branch.

**Files:** nenhum (documentação + git)

- [ ] **Step 1: Conferir a lista de tópicos a criar no Node-RED**

Usar a tabela da spec (`docs/superpowers/specs/2026-07-09-cardapio-pedidos-cerveja-design.md`, seção "Tópicos MQTT") como checklist na aula:
- `bet/getCardapio` → `bet/cardapio`
- `bet/registrarPedido` (sem resposta)
- `bet/getMeusPedidos` → `bet/meusPedidos` (com JOIN em `cardapio`)
- `bet/getCopo` → `bet/copo` (com JOIN, se necessário, para pegar `mesa`/`id_cliente` a partir do `rfid`)

- [ ] **Step 2: Conferir o `CREATE TABLE pedidos` a levar para a aula**

```sql
CREATE TABLE pedidos (
  id SERIAL PRIMARY KEY,
  created_at TIMESTAMP DEFAULT now(),
  id_cliente INT4 NOT NULL,
  id_produto INT4 NOT NULL REFERENCES cardapio(id),
  status VARCHAR DEFAULT 'pendente',
  mesa INT4
);
```

- [ ] **Step 3: Push da branch**

```bash
cd "/c/Users/Rafael/OneDrive/PUC Rio/projeto_iot"
git push -u origin cardapio-pedidos-cerveja
```

- [ ] **Step 4: Teste no dispositivo (assim que os tópicos existirem na aula)**

Run: `pio run -t upload`
Expected, com Serial Monitor a 115200:
- MENU → Cardápio → Cervejas → "Carregando... Buscando copo" → tela com ml/temperatura/status/bateria reais.
- MENU → Cardápio → Drinks/Petiscos/Não Alcoólico → "Carregando... Buscando cardapio" → lista de produtos reais.
- Detalhe do produto → PEDIR → serial mostra `[Cardapio] Pedido enviado: {"id_cliente":..,"id_produto":..}`.
- MENU → Pedidos → Meus Pedidos → "Carregando... Buscando pedidos" → lista real de pedidos do cliente.

- [ ] **Step 5: Abrir o PR**

`cardapio-pedidos-cerveja` → `main`. Link direto:
`https://github.com/rafaribeiro2013/projeto_iot_bet/compare/main...cardapio-pedidos-cerveja`

---

## Self-Review (cobertura da spec)

- Tabelas existentes (`cardapio`, `copos`) e nova (`pedidos`) → Tasks 1, 6. ✔
- `copos` como estado atual → redesenho da tela de cerveja → Task 3. ✔
- Tópicos MQTT propostos (getCardapio/cardapio, registrarPedido, getMeusPedidos/meusPedidos, getCopo/copo) → Task 2. ✔
- Modelo de dados (Produto, PedidoItem, Copo) e buffers → Task 1. ✔
- Nota sobre `preco` float → exibição inteira → Tasks 4 e 5 (`(int)(preco + 0.5f)`). ✔
- Fluxo Cardápio → Lista de Produtos → Task 4. ✔
- Fluxo Detalhe do Produto → Pedir → Task 4. ✔
- Fluxo Cervejas → Monitor do Copo → Task 3. ✔
- Fluxo Meus Pedidos → Task 5. ✔
- `obterTotalConta` inalterado, comentário reforçado → Task 5. ✔
- Limpeza do enum `EsperandoDado` → Task 3. ✔
- Fora de escopo (fechamento de conta, criação de tabelas/queries em aula, histórico de cerveja) → explicitado nas Global Constraints e Task 6. ✔

Sem placeholders de implementação; nomes/tipos consistentes entre tarefas (`produtos[]`, `meusPedidos[]`, `copoAtual`, `ESPERANDO_*`, assinaturas de `cardapio.h` usadas identicamente em `dados.h`/`navegacao.h`).
