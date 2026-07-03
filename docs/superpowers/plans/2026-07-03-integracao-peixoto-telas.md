# Integração BetBar (peixoto1 × tela-rafa) — Plano de Implementação

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Unir o back-end (WiFi/MQTT/RFID + apostas do `peixoto1`) ao front-end de telas ePaper (`tela-rafa`), removendo a navegação por terminal e fazendo os dados reais de jogos e apostas aparecerem nas telas.

**Architecture:** As telas continuam sendo o front-end; os módulos do `peixoto1` viram a camada de dados; um cache de buffers globais + uma flag `precisaRedesenhar` fazem a ponte entre o MQTT (assíncrono) e a UI (síncrona). A máquina de estados por terminal (`sessao.cpp`) é desmontada.

**Tech Stack:** ESP32-S3, PlatformIO/Arduino, GxEPD2, U8g2_for_Adafruit_GFX, ArduinoJson, GFButton, QRCodeGFX, MFRC522 (RFID), 256dpi/MQTT (MQTT sobre TLS).

## Global Constraints

- Alvo: `board = esp32-s3-devkitc-1`, `framework = arduino`, em `[env:esp32s3]`.
- Display 296×128, `setRotation(3)`. Sempre chamar `setFontMode(1)` após cada `setFont()`. **Nunca** usar `getUTF8Width` nem `setForegroundColor(GxEPD_WHITE)`. Centralização por estimativa manual (`centralizar()`).
- Pinos existentes: display `(10,14,15,16)`; botões `up(1) down(2) left(42) right(41) mid(40)`; RFID `SS=46, RST=17`.
- Objetos globais são **definidos** só em `main.cpp` e declarados `extern` em headers (padrão já usado no projeto).
- Nomes de time: mostrar o **ID** (ex.: `Time 10`) — o back-end não retorna nomes.
- Empate: `id_time_apostado` enviado como `null` (sentinela `TIME_EMPATE` no código).
- "Teste" de cada tarefa = **compilar** com `pio run` (ou o ✓ do PlatformIO no VS Code) e, quando indicado, **observar o Serial Monitor a 115200** ou o comportamento no dispositivo. Não há framework de testes unitários rodando no device.
- Trabalhar na branch `integracao` (criada a partir de `tela-rafa`). Commits frequentes, um por tarefa.

**Spec de referência:** `docs/superpowers/specs/2026-07-03-integracao-peixoto-telas-design.md`

---

## Estrutura de arquivos (após a integração)

```
src/
  main.cpp          setup/loop; DEFINE todos os globais (tela, fontes, qrcode, botões,
                    buffers de dados); define mqttMensagemRecebida()
  globais.h         includes + extern dos objetos de tela
  modelo.h          [NOVO] structs Cliente, Partida, Aposta + sentinela TIME_EMPATE
  estado_dados.h    [NOVO] extern dos buffers de cache + precisaRedesenhar
  dados.h           camada de dados: jogos/apostas leem do cache; registrarAposta envia
                    de verdade; produtos/pedidos/conta/cerveja continuam mock
  desenhos.h        telas (+ 3 novas: AguardandoCartao, Carregando, Aviso)
  navegacao.h       máquina de estados de TELA + handlers dos 5 botões
  wifi_module.*     [IMPORTADO] conexão WiFi
  mqtt_module.*     [IMPORTADO] conexão MQTT TLS + publicar + callback
  rfid_module.*     [IMPORTADO] leitor RFID
  apostas.*         [IMPORTADO+AJUSTADO] getPartidas/realizar/consultar + parse → buffers
  certificados.h    [IMPORTADO] certificado TLS do broker
platformio.ini      + libs MFRC522 e 256dpi/MQTT
```

Arquivos **não** trazidos do `peixoto1`: `sessao.cpp/.h` (structs migram p/ `modelo.h`; resto morre), `botao.cpp/.h` (substituído pela `navegacao.h`), `main.cpp` (raiz), `pix.cpp`, `src/08a_testes_iniciais.cpp`, `src/lixo/*`.

---

## Task 0: Criar a branch de integração

**Files:** nenhum (operação git)

- [ ] **Step 1: Criar e ir para a branch**

```bash
cd "/c/Users/Rafael/OneDrive/PUC Rio/projeto_iot"
git checkout tela-rafa
git checkout -b integracao
```

- [ ] **Step 2: Confirmar a branch**

Run: `git branch --show-current`
Expected: `integracao`

---

## Task 1: Bibliotecas + módulos-folha (WiFi, certificados)

Módulos sem dependência de outros módulos do projeto. Compilam sozinhos.

**Files:**
- Modify: `platformio.ini`
- Create: `src/wifi_module.h`, `src/wifi_module.cpp`, `src/certificados.h` (cópia do peixoto1)

**Interfaces:**
- Produces: `void wifiInit();`, `void wifiReconectar();`, `bool wifiConectado();` (conforme `wifi_module.h` do peixoto1); `const char* certificado1` (em `certificados.h`).

- [ ] **Step 1: Adicionar libs ao platformio.ini**

No bloco `lib_deps`, somar duas linhas:

```ini
	miguelbalboa/MFRC522
	256dpi/MQTT
```

- [ ] **Step 2: Trazer os arquivos do peixoto1**

```bash
git show origin/peixoto1:src/wifi_module.h  > src/wifi_module.h
git show origin/peixoto1:src/wifi_module.cpp > src/wifi_module.cpp
git show origin/peixoto1:src/certificados.h > src/certificados.h
```

- [ ] **Step 3: Compilar**

Run: `pio run`
Expected: `SUCCESS` (as libs MFRC522/MQTT são baixadas; wifi_module compila).

- [ ] **Step 4: Commit**

```bash
git add platformio.ini src/wifi_module.* src/certificados.h
git commit -m "integracao: trazer wifi_module, certificados e libs RFID/MQTT"
```

---

## Task 2: Módulo RFID

**Files:**
- Create: `src/rfid_module.h`, `src/rfid_module.cpp` (cópia do peixoto1)

**Interfaces:**
- Produces: `void rfidInit();`, `bool rfidNovoCartao();`, `String rfidLerUID();` (de `rfid_module.h`).

- [ ] **Step 1: Trazer os arquivos**

```bash
git show origin/peixoto1:src/rfid_module.h  > src/rfid_module.h
git show origin/peixoto1:src/rfid_module.cpp > src/rfid_module.cpp
```

- [ ] **Step 2: Compilar**

Run: `pio run`
Expected: `SUCCESS`.

- [ ] **Step 3: Commit**

```bash
git add src/rfid_module.*
git commit -m "integracao: trazer rfid_module"
```

---

## Task 3: Modelo de dados + buffers de cache

O coração da ponte assíncrona. `modelo.h` tem os structs; `estado_dados.h` declara os buffers; `main.cpp` os define.

**Files:**
- Create: `src/modelo.h`, `src/estado_dados.h`
- Modify: `src/main.cpp` (definir os buffers globais)

**Interfaces:**
- Produces:
  - `struct Cliente { int32_t id; String nomeCompleto; String telefone; String email; };`
  - `struct Partida { int32_t id; int32_t idTimeCasa; int32_t idTimeFora; String data; };`
  - `struct Aposta { int32_t id; String createdAt; int32_t idCliente; int32_t idPartida; int32_t idTimeApostado; };`
  - `#define TIME_EMPATE INT32_MIN`
  - Globais: `Cliente clienteAtual; bool clienteCarregado; Partida partidas[20]; uint8_t totalPartidas; bool partidasProntas; Aposta apostas[20]; uint8_t totalApostas; bool apostasProntas; volatile bool precisaRedesenhar;`

- [ ] **Step 1: Criar `src/modelo.h`**

```cpp
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
```

- [ ] **Step 2: Criar `src/estado_dados.h`**

```cpp
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

// Ligada pelos callbacks quando chega dado novo; o loop() a observa e redesenha.
extern volatile bool precisaRedesenhar;

#endif
```

- [ ] **Step 3: Definir os buffers em `main.cpp`**

Logo após a definição de `qrcode` (linha com `QRCodeGFX qrcode(tela);`), adicionar:

```cpp
// --- Buffers de dados (declarados em estado_dados.h) ---
Cliente clienteAtual   = {0, "", "", ""};
bool    clienteCarregado = false;
Partida partidas[20];
uint8_t totalPartidas  = 0;
bool    partidasProntas = false;
Aposta  apostas[20];
uint8_t totalApostas   = 0;
bool    apostasProntas  = false;
volatile bool precisaRedesenhar = false;
```

E incluir os headers no topo do `main.cpp` (após os includes atuais):

```cpp
#include "modelo.h"
#include "estado_dados.h"
```

- [ ] **Step 4: Compilar**

Run: `pio run`
Expected: `SUCCESS`.

- [ ] **Step 5: Commit**

```bash
git add src/modelo.h src/estado_dados.h src/main.cpp
git commit -m "integracao: modelo de dados + buffers de cache"
```

---

## Task 4: Módulo de apostas ligado aos buffers

Traz `apostas.cpp/.h`, mas repontado para os buffers (sem `sessao`). Inclui a **correção** do bug de `minhasApostas` (subscribe + handler).

**Files:**
- Create: `src/apostas.h`, `src/apostas.cpp` (cópia ajustada)
- Modify: `src/main.cpp` (definir `mqttMensagemRecebida` despachando p/ apostas)

**Interfaces:**
- Consumes: buffers de `estado_dados.h`; `mqttPublicar()` de `mqtt_module.h`.
- Produces: `void apostasInit();`, `void getPartidas();`, `void apostasRealizar(const Aposta&);`, `void apostasConsultar(int32_t idCliente);`, `bool apostasProcessarMensagem(const String& topico, const String& conteudo);`

- [ ] **Step 1: Criar `src/apostas.h`**

```cpp
#pragma once

#include <Arduino.h>
#include "modelo.h"
#include "mqtt_module.h"

#define TOPICO_REALIZAR_APOSTA  "bet/realizarAposta"
#define TOPICO_CONSULTAR_APOSTA "bet/consultarAposta"
#define TOPICO_MINHAS_APOSTAS   "bet/minhasApostas"
#define TOPICO_GET_PARTIDAS     "bet/getPartidas"
#define TOPICO_PARTIDAS         "bet/partidas"
#define TOPICO_DADOS_CLIENTE    "dadosCliente"

void apostasInit();
void getPartidas();
void apostasRealizar(const Aposta& aposta);
void apostasConsultar(int32_t idCliente);
bool apostasProcessarMensagem(const String& topico, const String& conteudo);
```

- [ ] **Step 2: Criar `src/apostas.cpp`** (repontado aos buffers; empate → null; minhasApostas tratado)

```cpp
#include "apostas.h"
#include "estado_dados.h"
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
  mqttPublicar(TOPICO_GET_PARTIDAS, "");
  Serial.println("[Apostas] Solicitando partidas...");
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
  if (aposta.idTimeApostado == TIME_EMPATE) doc["id_time_apostado"] = nullptr;  // empate
  else                                      doc["id_time_apostado"] = aposta.idTimeApostado;
  String payload; serializeJson(doc, payload);
  mqttPublicar(TOPICO_REALIZAR_APOSTA, payload);
  Serial.println("[Apostas] Aposta enviada: " + payload);
}

void apostasConsultar(int32_t idCliente) {
  apostasProntas = false;
  JsonDocument doc;
  doc["id_cliente"] = idCliente;
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
    apostas[i].idTimeApostado = o["id_time_apostado"] | TIME_EMPATE;  // null -> empate
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
```

- [ ] **Step 3: Definir `mqttMensagemRecebida` em `main.cpp`**

Adicionar o include `#include "apostas.h"` no topo e, antes de `setup()`, a função de callback:

```cpp
void mqttMensagemRecebida(String topico, String conteudo) {
  Serial.println("[MQTT] " + topico + ": " + conteudo);
  apostasProcessarMensagem(topico, conteudo);
}
```

- [ ] **Step 4: Compilar**

Run: `pio run`
Expected: `SUCCESS`.

- [ ] **Step 5: Commit**

```bash
git add src/apostas.* src/main.cpp
git commit -m "integracao: apostas ligado aos buffers + fix bet/minhasApostas"
```

---

## Task 5: Trazer o mqtt_module e ligar init/loop de rede

Agora liga WiFi/MQTT/RFID no `setup()`/`loop()`. Primeira tarefa **testada no dispositivo** (serial).

**Files:**
- Create: `src/mqtt_module.h`, `src/mqtt_module.cpp` (cópia do peixoto1)
- Modify: `src/main.cpp` (setup + loop de rede/RFID)

**Interfaces:**
- Consumes: `wifiInit/Reconectar`, `rfidInit/NovoCartao/LerUID`, `apostasInit`, `mqttInit/Loop/Reconectar/Publicar`.
- Produces: publica `bet/autenticaCliente` ao ler cartão.

- [ ] **Step 1: Trazer o mqtt_module**

```bash
git show origin/peixoto1:src/mqtt_module.h  > src/mqtt_module.h
git show origin/peixoto1:src/mqtt_module.cpp > src/mqtt_module.cpp
```

- [ ] **Step 2: Incluir os módulos em `main.cpp`**

No topo, junto aos includes:

```cpp
#include "wifi_module.h"
#include "mqtt_module.h"
#include "rfid_module.h"
```

- [ ] **Step 3: Inicializar no `setup()`**

No fim do `setup()` (antes de `instanteAnterior = millis();`), adicionar:

```cpp
  wifiInit();
  mqttInit();
  rfidInit();
  apostasInit();
```

- [ ] **Step 4: Rede + RFID no `loop()`**

No início do `loop()`, antes do `process()` dos botões, adicionar:

```cpp
  wifiReconectar();
  mqttReconectar();
  mqttLoop();

  if (rfidNovoCartao()) {
    String uid = rfidLerUID();
    Serial.println("[Loop] Cartao UID: " + uid);
    mqttPublicar(TOPICO_AUTENTICA_CLIENTE, uid);
  }
```

E, no fim do `loop()`, o redesenho reativo:

```cpp
  if (precisaRedesenhar) {
    precisaRedesenhar = false;
    renderizarTelaAtual();
  }
```

- [ ] **Step 5: Compilar e gravar**

Run: `pio run -t upload` (ou ✓ + → no VS Code)
Expected: `SUCCESS` e upload concluído.

- [ ] **Step 6: Verificar no Serial Monitor (115200)**

Encostar o cartão. Esperado no serial: WiFi conectado → MQTT conectado → `Cartao UID: ...` → `[MQTT] dadosCliente: [{...}]` → `[Apostas] Cliente: <nome>`. (Ainda sem mudança de tela.)

- [ ] **Step 7: Commit**

```bash
git add src/mqtt_module.* src/main.cpp
git commit -m "integracao: init e loop de WiFi/MQTT/RFID; autentica ao ler cartao"
```

---

## Task 6: Telas AGUARDANDO_CARTAO e CARREGANDO + entrada por cartão

Cria as 2 primeiras telas auxiliares e faz o fluxo começar pedindo o cartão. **Layout a revisar.**

**Files:**
- Modify: `src/navegacao.h` (novos estados + render + entrada por cartão), `src/desenhos.h` (2 telas), `src/main.cpp` (estado inicial + reação ao cartão)

**Interfaces:**
- Consumes: `clienteCarregado` de `estado_dados.h`.
- Produces: `void desenharAguardandoCartao();`, `void desenharCarregando(const char* msg);`; estados `AGUARDANDO_CARTAO`, `CARREGANDO`.

- [ ] **Step 1: Adicionar os estados no enum `Tela` (`navegacao.h`)**

Trocar a primeira linha do enum para incluir os dois estados:

```cpp
enum Tela {
  AGUARDANDO_CARTAO, CARREGANDO,
  INICIAL, MENU, CARDAPIO, LISTA_PRODUTOS, DETALHE_PRODUTO, CONTROLE_CERVEJA,
  LISTA_JOGOS, JOGO, CONFIRMACAO, CONFIRMADO,
  MENU_PEDIDOS, MEUS_PEDIDOS, PAGAR_CONTA, MINHAS_APOSTAS
};
```

- [ ] **Step 2: Desenhar `desenharAguardandoCartao()` e `desenharCarregando()` (`desenhos.h`)**

Adicionar antes de `#endif`:

```cpp
// Tela de espera do cartao RFID (substitui a INICIAL no fluxo real).
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

// Tela generica de espera de resposta do servidor.
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
```

- [ ] **Step 3: Guardar a mensagem de carregamento e renderizar os novos estados (`navegacao.h`)**

Após a definição de `EstadoTela estado = ...`, adicionar um buffer para a mensagem:

```cpp
char msgCarregando[24] = "";
```

No `switch` de `renderizarTelaAtual()`, adicionar os dois casos no início:

```cpp
    case AGUARDANDO_CARTAO:
      desenharAguardandoCartao();
      // se o cliente ja chegou, avanca para o menu
      if (clienteCarregado) { estado.tipo = MENU; estado.indice = 0; desenharMenu(0); }
      break;

    case CARREGANDO:
      desenharCarregando(msgCarregando);
      // transicoes automaticas quando o dado esperado chega
      if (clienteCarregado && estado.indice == 0) { estado.tipo = MENU; estado.indice = 0; desenharMenu(0); }
      else if (partidasProntas && estado.indice == 1) { irPara(LISTA_JOGOS); }
      else if (apostasProntas  && estado.indice == 2) { irPara(MINHAS_APOSTAS); }
      break;
```

> Convenção: em `CARREGANDO`, `estado.indice` marca o que se espera — `0`=cliente, `1`=partidas, `2`=apostas.

- [ ] **Step 4: Estado inicial = AGUARDANDO_CARTAO (`main.cpp`)**

No `setup()`, trocar:

```cpp
  estado.tipo = INICIAL;
```
por
```cpp
  estado.tipo = AGUARDANDO_CARTAO;
```

E na reação ao cartão dentro do `loop()` (Task 5, Step 4), após publicar, mostrar carregando:

```cpp
  if (rfidNovoCartao()) {
    String uid = rfidLerUID();
    Serial.println("[Loop] Cartao UID: " + uid);
    clienteCarregado = false;
    mqttPublicar(TOPICO_AUTENTICA_CLIENTE, uid);
    strncpy(msgCarregando, "Autenticando", sizeof(msgCarregando) - 1);
    estado.tipo = CARREGANDO; estado.indice = 0;
    renderizarTelaAtual();
  }
```

> `msgCarregando` é global em `navegacao.h`; como `main.cpp` inclui `navegacao.h`, está visível. Se o compilador reclamar de ordem, declarar `extern char msgCarregando[24];` no topo do `main.cpp`.

- [ ] **Step 5: Compilar, gravar e testar no dispositivo**

Run: `pio run -t upload`
Expected: liga na tela "Encoste seu cartao"; ao encostar o cartão → "Carregando... Autenticando" → **MENU** quando o cliente chega.

- [ ] **Step 6: Commit**

```bash
git add src/navegacao.h src/desenhos.h src/main.cpp
git commit -m "integracao: entrada por cartao (telas AguardandoCartao e Carregando)"
```

---

## Task 7: Apostar → partidas reais na LISTA_JOGOS

Liga o `obterJogos()` ao buffer e faz "Apostar" buscar partidas de verdade.

**Files:**
- Modify: `src/dados.h` (`obterJogos` do buffer), `src/navegacao.h` (Apostar dispara `getPartidas`)

**Interfaces:**
- Consumes: `partidas[]`, `totalPartidas` de `estado_dados.h`; `getPartidas()` de `apostas.h`.

- [ ] **Step 1: Incluir headers de dados em `dados.h`**

No topo de `dados.h`, após `#include "globais.h"`:

```cpp
#include "estado_dados.h"
#include "apostas.h"
```

- [ ] **Step 2: Reescrever `obterJogos()` para ler do buffer**

Substituir todo o corpo mock de `obterJogos(JsonArray destino)` por:

```cpp
void obterJogos(JsonArray destino) {
  for (uint8_t i = 0; i < totalPartidas; i++) {
    JsonObject o = destino.add<JsonObject>();
    o["id"]   = partidas[i].id;
    char casa[12], fora[12];
    snprintf(casa, sizeof(casa), "Time %d", partidas[i].idTimeCasa);
    snprintf(fora, sizeof(fora), "Time %d", partidas[i].idTimeFora);
    o["casa"] = casa;
    o["fora"] = fora;
  }
}
```

- [ ] **Step 3: "Apostar" dispara busca assíncrona (`navegacao.h`)**

Em `selecionar()`, no `case MENU`, trocar a linha do índice 1:

```cpp
      else if (estado.indice == 1) {
        getPartidas();
        strncpy(msgCarregando, "Buscando jogos", sizeof(msgCarregando) - 1);
        empilhar();
        estado.tipo = CARREGANDO; estado.indice = 1;   // 1 = esperando partidas
        renderizarTelaAtual();
      }
```

- [ ] **Step 4: Compilar, gravar e testar**

Run: `pio run -t upload`
Expected: MENU → Apostar → "Carregando... Buscando jogos" → lista de jogos mostrando `Time X x Time Y` com os IDs reais do banco.

- [ ] **Step 5: Commit**

```bash
git add src/dados.h src/navegacao.h
git commit -m "integracao: Apostar busca partidas reais e lista jogos do buffer"
```

---

## Task 8: Registrar aposta real (com empate = null)

**Files:**
- Modify: `src/dados.h` (`registrarAposta` real), `src/navegacao.h` (mapeamento de palpite)

**Interfaces:**
- Consumes: `clienteAtual`, `partidas[]`; `apostasRealizar()` de `apostas.h`.

- [ ] **Step 1: Reescrever `registrarAposta()` em `dados.h`**

Substituir o corpo mock por (recebe o id da partida e o palpite 0/1/2):

```cpp
void registrarAposta(int jogoId, int palpite) {
  Aposta a;
  a.id = 0; a.createdAt = "";
  a.idCliente = clienteAtual.id;
  a.idPartida = jogoId;
  // localizar a partida para pegar os ids dos times
  int32_t casa = 0, fora = 0;
  for (uint8_t i = 0; i < totalPartidas; i++) {
    if (partidas[i].id == jogoId) { casa = partidas[i].idTimeCasa; fora = partidas[i].idTimeFora; break; }
  }
  if      (palpite == 0) a.idTimeApostado = casa;
  else if (palpite == 2) a.idTimeApostado = fora;
  else                   a.idTimeApostado = TIME_EMPATE;  // palpite 1 = empate
  apostasRealizar(a);
}
```

- [ ] **Step 2: Confirmar que a CONFIRMACAO usa o id da partida (`navegacao.h`)**

No `case CONFIRMACAO` de `selecionar()`, garantir que `jogoId` vem do buffer real (já é o caso: `jogos[estado.jogoIndice]["id"]`). Nenhuma mudança de lógica além de já usar `registrarAposta(jogoId, estado.palpite)`. Verificar que a linha existente é:

```cpp
      int jogoId = jogos[estado.jogoIndice]["id"].as<int>();
      registrarAposta(jogoId, estado.palpite);
```

- [ ] **Step 3: Compilar, gravar e testar**

Run: `pio run -t upload`
Expected: apostar num time → CONFIRMADO; no Serial: `[Apostas] Aposta enviada: {"id_cliente":..,"id_partida":..,"id_time_apostado":..}`. Testar empate → payload com `"id_time_apostado":null`.

- [ ] **Step 4: Commit**

```bash
git add src/dados.h src/navegacao.h
git commit -m "integracao: registrar aposta real (empate=null)"
```

---

## Task 9: Minhas Apostas reais

**Files:**
- Modify: `src/dados.h` (`obterApostas` do buffer), `src/navegacao.h` (Minhas Apostas dispara consulta)

**Interfaces:**
- Consumes: `apostas[]`, `totalApostas`; `apostasConsultar()`.

- [ ] **Step 1: Reescrever `obterApostas()` em `dados.h`**

```cpp
void obterApostas(JsonArray destino) {
  for (uint8_t i = 0; i < totalApostas; i++) {
    JsonObject o = destino.add<JsonObject>();
    char jogo[16], palpite[16];
    snprintf(jogo, sizeof(jogo), "Partida %d", apostas[i].idPartida);
    if (apostas[i].idTimeApostado == TIME_EMPATE) snprintf(palpite, sizeof(palpite), "Empate");
    else snprintf(palpite, sizeof(palpite), "Time %d", apostas[i].idTimeApostado);
    o["jogo"]    = jogo;
    o["palpite"] = palpite;
    o["status"]  = "Pendente";   // back-end nao envia resultado ainda
  }
}
```

- [ ] **Step 2: "Minhas Apostas" dispara consulta (`navegacao.h`)**

Em `selecionar()`, `case MENU`, trocar a linha do índice 3:

```cpp
      else if (estado.indice == 3) {
        apostasConsultar(clienteAtual.id);
        strncpy(msgCarregando, "Buscando apostas", sizeof(msgCarregando) - 1);
        empilhar();
        estado.tipo = CARREGANDO; estado.indice = 2;   // 2 = esperando apostas
        renderizarTelaAtual();
      }
```

- [ ] **Step 3: Compilar, gravar e testar**

Run: `pio run -t upload`
Expected: MENU → Minhas Apostas → "Carregando... Buscando apostas" → lista com as apostas reais do cliente (Partida N / Time X ou Empate).

- [ ] **Step 4: Commit**

```bash
git add src/dados.h src/navegacao.h
git commit -m "integracao: Minhas Apostas reais (consultarAposta + buffer)"
```

---

## Task 10: Polimento — tela AVISO, timeout e reconexão não-bloqueante

**Files:**
- Modify: `src/desenhos.h` (tela AVISO), `src/navegacao.h` (estado AVISO + timeout de CARREGANDO), `src/main.cpp` (timeout no loop; reconexão)

**Interfaces:**
- Produces: `void desenharAviso(const char* msg);`; estado `AVISO`.

- [ ] **Step 1: Adicionar `AVISO` ao enum `Tela`**

Incluir `AVISO` junto de `AGUARDANDO_CARTAO, CARREGANDO,` no enum.

- [ ] **Step 2: Desenhar `desenharAviso()` (`desenhos.h`)**

```cpp
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
```

- [ ] **Step 3: Render do AVISO + guardar a mensagem (`navegacao.h`)**

Reaproveitar `msgCarregando` como buffer de mensagem, ou criar `char msgAviso[24]`. Adicionar no `switch` de `renderizarTelaAtual()`:

```cpp
    case AVISO:
      desenharAviso(msgCarregando);
      break;
```

- [ ] **Step 4: Timeout da tela CARREGANDO (`main.cpp`)**

No `loop()`, junto às outras verificações de tempo, cair em AVISO se a resposta não vier em 8s. Guardar o instante ao entrar em CARREGANDO (setar `instanteAnterior = millis()` já acontece nos handlers; usar uma variável dedicada é mais seguro):

```cpp
  static unsigned long inicioCarregando = 0;
  if (estado.tipo == CARREGANDO) {
    if (inicioCarregando == 0) inicioCarregando = millis();
    if (millis() - inicioCarregando > 8000) {
      strncpy(msgCarregando, "Sem resposta", sizeof(msgCarregando) - 1);
      estado.tipo = AVISO; renderizarTelaAtual();
      inicioCarregando = 0;
    }
  } else {
    inicioCarregando = 0;
  }
```

- [ ] **Step 5: Reconexão não-bloqueante (opcional — decisão da spec §9)**

Se as travadas ao reconectar incomodarem, editar `wifi_module.cpp`/`mqtt_module.cpp` trocando os `while(...) delay(1000)` por uma tentativa única por iteração do loop (return se ainda não conectou). Caso contrário, manter como está.

- [ ] **Step 6: Compilar, gravar e testar**

Run: `pio run -t upload`
Expected: se o servidor não responder em 8s numa busca, aparece a tela de aviso "Sem resposta"; esquerda volta.

- [ ] **Step 7: Commit**

```bash
git add src/desenhos.h src/navegacao.h src/main.cpp
git commit -m "integracao: tela de aviso, timeout de carregamento e reconexao"
```

---

## Task 11: Revisão de layout + PR

**Files:** nenhum (revisão + git)

- [ ] **Step 1: Revisar as 3 telas auxiliares no dispositivo**

Fotografar/olhar `AGUARDANDO_CARTAO`, `CARREGANDO`, `AVISO` e ajustar posições/fontes conforme o padrão visual do projeto. (Destacar para o Rafael e o parceiro aprovarem.)

- [ ] **Step 2: Push da branch**

```bash
git push -u origin integracao
```

- [ ] **Step 3: Abrir o PR**

`integracao` → `tela-rafa` (ou `main`, a combinar). Link direto:
`https://github.com/rafaribeiro2013/projeto_iot_bet/compare/tela-rafa...integracao`

---

## Self-Review (cobertura da spec)

- §3 ponte assíncrona → Tasks 3, 4, 5, 6 (buffers + flag + redesenho no loop). ✔
- §4 modelo/tópicos/empate → Tasks 3 (structs/sentinela), 4 (tópicos, empate=null). ✔
- §5 arquivos novos/alterados/removidos → Tasks 1–4 (importados/novos), 6–9 (alterados); removidos nunca são trazidos. ✔
- §6 cola (apostas/dados/navegacao/main) → Tasks 4, 6, 7, 8, 9. ✔
- §7 fluxo combinado → Tasks 6 (cartão→menu), 7 (apostar), 8 (confirmar), 9 (minhas apostas). ✔
- §8 telas auxiliares → Tasks 6 (2 telas), 10 (AVISO), 11 (revisão). ✔
- §9 hardware (SPI, reconexão) → Task 5 (teste no device), Task 10 Step 5 (reconexão). SPI é validação de hardware, não de código — sinalizado. ✔
- §10 libs → Task 1. ✔
- §11 notas de servidor → fora do firmware (responsabilidade do parceiro); referenciado na spec. ✔
- §12 ordem → espelhada nas Tasks 1–10. ✔
- §13 git → Tasks 0 e 11. ✔

Sem placeholders de implementação; nomes/tipos consistentes entre tasks (`partidas[]`, `apostas[]`, `TIME_EMPATE`, `precisaRedesenhar`, `msgCarregando`, assinaturas de `apostas.h`).
