# Integração BetBar: back-end (peixoto1) × telas (tela-rafa)

**Data:** 2026-07-03
**Branches envolvidas:** `tela-rafa` (front-end / telas ePaper) + `peixoto1` (back-end WiFi/MQTT/RFID + apostas)
**Branch de trabalho:** `integracao` (criada a partir de `tela-rafa`)

---

## 1. Objetivo

Unir os dois trabalhos num único firmware para o ESP32-S3:

- A navegação e os dados de apostas deixam de acontecer **pelo terminal serial** (fluxo do `peixoto1`) e passam a acontecer **nas telas ePaper** (fluxo do `tela-rafa`).
- Os dados **reais** (cliente autenticado por RFID, partidas do banco, registro e consulta de apostas) alimentam as telas de **Apostar** e **Minhas Apostas**.
- Cardápio, Pedidos, Pagar Conta e Consumo de Cerveja continuam com dados **mock** nesta etapa (o back-end não tem esses dados).

## 2. Estado atual das duas branches

As branches **não têm ancestral comum** (históricos independentes) e **não conflitam** — são complementares. Não haverá merge/rebase git entre elas; a integração é trabalho de escrever código de cola.

### `tela-rafa` (front-end)
```
src/
  main.cpp       setup()/loop(); objetos globais (tela, fontes, qrcode, botões)
  globais.h      includes + extern dos objetos
  dados.h        camada de repositório — HOJE tudo MOCK
  desenhos.h     funções de desenho puras (uma por tela)
  navegacao.h    máquina de estados de TELA (enum Tela) + handlers dos 5 botões
```
- 5 botões: `up(1) down(2) left(42) right(41) mid(40)`.
- Navegação já definida: no MENU as setas andam na grade 2×2 e o meio seleciona; nas demais telas cima/baixo movem o cursor, **esquerda volta** e **direita seleciona**.

### `peixoto1` (back-end)
```
src/
  wifi_module.*   conecta no WiFi "LabIoT"
  mqtt_module.*   conecta no broker TLS mqtt.janks.dev.br:8883; mqttPublicar(); callback
  rfid_module.*   MFRC522 nos pinos SS=46, RST=17; rfidNovoCartao(); rfidLerUID()
  apostas.*       getPartidas(), apostasRealizar(), apostasConsultar(); parse JSON
  sessao.*        máquina de estados POR TERMINAL (Serial.print) — será desmontada
  botao.*         3 botões p/ terminal — DESCARTADO
certificados.h    certificado TLS do broker
main.cpp (raiz)   teste inicial RFID+MQTT — DESCARTADO
*.json            fluxos Node-RED do servidor (referência dos formatos, não código)
```

> Observação: `src/lixo/08c_integracao.cpp` está vazio ("COPIE E COLE AQUI"). **Não existe protótipo integrado** — os módulos nunca foram ligados num loop único. A cola é trabalho novo.

## 3. Arquitetura alvo

Suas telas viram o **front-end**; os módulos do `peixoto1` viram a **camada de dados**; a máquina de estados por terminal (`sessao.cpp`) é desmontada — o papel de *guardar dados* dela migra para um cache, o papel de *navegar* morre (a `navegacao.h` já faz).

```
┌─ FRONT-END (mantido) ──────────────┐   ┌─ BACK-END (importado) ─────────┐
│  desenhos.h   telas ePaper         │   │  wifi_module / mqtt_module     │
│  navegacao.h  estado + 5 botões    │   │  rfid_module                   │
│  main.cpp     setup/loop           │   │  apostas.cpp                   │
│                                     │   │                                │
│  dados.h  ◄──── A COLA ────────────┼───┤  modelo.h (structs)            │
│  (lê do cache; envia aposta real)  │   │  sessao.cpp  ✗ desmontado      │
│  estado_dados.h (buffers + flag)   │   │  botao.cpp   ✗ descartado      │
└─────────────────────────────────────┘   └────────────────────────────────┘
```

### A ponte síncrono ↔ assíncrono (ponto central)

A UI é **síncrona** (`obterJogos()` preenche o array e desenha na hora). O MQTT é **assíncrono** (publica um pedido, a resposta chega depois num callback). O ePaper é lento para redesenhar. A ponte:

1. **Buffers globais** (em `estado_dados.h`) guardam o que chega do MQTT: `clienteAtual`, `partidas[]`, `apostas[]`, com flags de "pronto".
2. Uma flag `volatile bool precisaRedesenhar` é ligada pelos callbacks do MQTT quando um dado novo chega.
3. O `loop()` verifica a flag e chama `renderizarTelaAtual()` — o redesenho acontece no laço principal, nunca dentro do callback.
4. Ao pedir dados (entrar em Apostar/Minhas Apostas), a tela vai para **CARREGANDO**; quando o dado chega, a flag dispara o redesenho já na tela final.

## 4. Modelo de dados

Structs extraídos de `sessao.h`/`apostas.h` para um `modelo.h` limpo:

```cpp
struct Cliente { int32_t id; String nomeCompleto; String telefone; String email; };
struct Partida { int32_t id; int32_t idTimeCasa; int32_t idTimeFora; String data; };
struct Aposta  { int32_t id; String createdAt; int32_t idCliente;
                 int32_t idPartida; int32_t idTimeApostado; };
```

### Tópicos MQTT (contrato com o servidor)

| Ação | Publica em | Payload | Resposta em (subscribe) |
|---|---|---|---|
| Autenticar cliente | `bet/autenticaCliente` | UID (string) | `dadosCliente` |
| Listar partidas | `bet/getPartidas` | `""` | `bet/partidas` |
| Registrar aposta | `bet/realizarAposta` | `{id_cliente,id_partida,id_time_apostado}` | `bet/apostaConfirmada` (opcional) |
| Consultar apostas | `bet/consultarAposta` | `{id_cliente}` | `bet/minhasApostas` |

### Formatos JSON (do banco, via Node-RED)

- **dadosCliente:** `[{ "id":1, "nome_completo":"...", "telefone":"...", "email":"..." }]` (array de 1).
- **bet/partidas:** `[{ "id":1, "mandante":10, "visitante":20, "data":"2025-06-15T18:00:00Z" }]` — **só IDs de time, sem nome**.
- **bet/minhasApostas:** `[{ "id":1, "id_cliente":1, "id_partida":2, "id_time_apostado":3, "created_at":"..." }]`.

### Decisões de mapeamento

- **Nomes de time:** o back-end não retorna nomes. Nesta etapa as telas exibem o **ID** (ex.: `Time 10 x Time 20`). Nomes ficam para uma etapa futura (JOIN no servidor).
- **Empate:** a tela `JOGO` mantém as 3 opções (casa / empate / fora). Envio de `id_time_apostado`:
  - Casa (palpite 0) → `idTimeCasa`
  - **Empate (palpite 1) → `null`** (no código, sentinela `EMPATE = -1`; ao serializar, `doc["id_time_apostado"] = nullptr`)
  - Fora (palpite 2) → `idTimeFora`

## 5. Arquivos: novos, alterados, removidos

### Importados do `peixoto1` (quase intactos)
- `src/wifi_module.h/.cpp` — copiar.
- `src/mqtt_module.h/.cpp` — copiar; o callback `mqttMensagemRecebida()` é definido no `main.cpp`.
- `src/rfid_module.h/.cpp` — copiar.
- `src/apostas.h/.cpp` — copiar e **ajustar** (ver §6): repontar 2 chamadas e **adicionar** o tratamento de `bet/minhasApostas`.
- `src/certificados.h` — copiar (certificado TLS).

### Novos
- `src/modelo.h` — structs `Cliente`, `Partida`, `Aposta` (extraídos de `sessao.h`/`apostas.h`).
- `src/estado_dados.h` — buffers de cache + flags + `precisaRedesenhar`.

### Alterados (do `tela-rafa`)
- `src/globais.h` — incluir os novos módulos.
- `src/dados.h` — trocar mocks de `obterJogos`/`obterApostas`/`registrarAposta` por leitura do cache / envio real; manter mock de produtos/pedidos/conta/cerveja.
- `src/desenhos.h` — **3 telas auxiliares novas** (ver §8) + `JOGO` continua com 3 opções.
- `src/navegacao.h` — novos estados `AGUARDANDO_CARTAO` e `CARREGANDO`; disparar busca assíncrona ao entrar em Apostar/Minhas Apostas.
- `src/main.cpp` — `setup()` inicializa WiFi/MQTT/RFID/apostas; `loop()` faz polling do RFID, `mqttLoop()`, reconexão e checagem de `precisaRedesenhar`; define `mqttMensagemRecebida()`.
- `platformio.ini` — adicionar libs (ver §10).

### Removidos / não usados
- `src/sessao.cpp` e a parte de máquina de estados de `sessao.h` (structs vão para `modelo.h`).
- `src/botao.cpp/.h` do peixoto1 (substituídos pelos handlers de 5 botões da `navegacao.h`).
- `main.cpp` (raiz), `pix.cpp`, `src/08a_testes_iniciais.cpp`, `src/lixo/*` — testes/legado.

## 6. A cola em detalhe

### `estado_dados.h` (novo)
```cpp
extern Cliente clienteAtual;   extern bool clienteCarregado;
extern Partida partidas[20];   extern uint8_t totalPartidas;  extern bool partidasProntas;
extern Aposta  apostas[20];    extern uint8_t totalApostas;   extern bool apostasProntas;
extern volatile bool precisaRedesenhar;
```
(As variáveis são **definidas** no `main.cpp`, como os demais globais.)

### `apostas.cpp` — 3 ajustes
1. `_processarDadosCliente()` — em vez de `sessaoReceberCliente(c)`, preencher `clienteAtual = c; clienteCarregado = true; precisaRedesenhar = true;`.
2. `_processarPartidas()` — em vez de `sessaoReceberPartidas(...)`, preencher `partidas[]`, `totalPartidas`, `partidasProntas = true; precisaRedesenhar = true;`.
3. **Adicionar** `_processarMinhasApostas()` e, em `apostasInit()`, `mqtt.subscribe("bet/minhasApostas")`; despachar em `apostasProcessarMensagem()`. Preenche `apostas[]`, `totalApostas`, `apostasProntas = true; precisaRedesenhar = true;`. (O parser já existe como `_processarResultadoAposta`; é reaproveitado apontando para o buffer.)

### `dados.h` — mock → real
- `obterJogos(destino)` — itera `partidas[0..totalPartidas)` e adiciona `{id, casa:"Time <idTimeCasa>", fora:"Time <idTimeFora>"}` (string com o ID enquanto não há nomes).
- `obterApostas(destino)` — itera `apostas[]` e monta `{jogo:"Partida <idPartida>", palpite:<casa/empate/fora do id>, status:"..."}` com o que houver.
- `registrarAposta(jogoId, palpite)` — monta `Aposta{ idCliente:clienteAtual.id, idPartida:jogoId, idTimeApostado }` onde `idTimeApostado` = casa/fora ou sentinela empate; chama `apostasRealizar(aposta)`. Na serialização, empate vira `null`.
- `obterProdutos/obterPedidos/obterTotalConta/obterConsumoCerveja` — **inalterados (mock)**.

### `navegacao.h` — disparos assíncronos
- Ao selecionar **Apostar** no MENU: `partidasProntas = false; getPartidas();` e ir para `CARREGANDO`. Quando `partidasProntas` (via redesenho), `renderizarTelaAtual()` na `CARREGANDO` detecta e troca para `LISTA_JOGOS`.
- Ao selecionar **Minhas Apostas**: `apostasProntas = false; apostasConsultar(clienteAtual.id);` e ir para `CARREGANDO` → `MINHAS_APOSTAS` quando chegar.
- `CONFIRMACAO` + selecionar: `registrarAposta(...)` (publish real) → `CONFIRMADO` (o retorno automático em 10s já existe).
- Mapeamento de palpite → `idTimeApostado` conforme §4.

### `main.cpp` — setup/loop
- `setup()`: `wifiInit(); mqttInit(); rfidInit(); apostasInit();` além do init de tela/fontes/botões atual.
- `loop()`: manter `process()` dos botões; **adicionar** `mqttLoop()` + reconexão WiFi/MQTT; polling `if (rfidNovoCartao()) { publish autenticaCliente(uid); ir para CARREGANDO }`; e `if (precisaRedesenhar) { precisaRedesenhar=false; renderizarTelaAtual(); }`.
- Definir `void mqttMensagemRecebida(String t, String c){ apostasProcessarMensagem(t,c); }`.

## 7. Fluxo de navegação combinado

```
AGUARDANDO_CARTAO  "Encoste seu cartão"
   │  (loop detecta RFID → publica autenticaCliente → CARREGANDO)
   │  (chega dadosCliente → precisaRedesenhar)
   ▼
MENU ─┬─ Cardápio ........ (mock)
      ├─ Apostar ──► CARREGANDO ──► LISTA_JOGOS(real) ──► JOGO(casa/empate/fora)
      │                                                     └─► CONFIRMAÇÃO ──►[realizarAposta]──► CONFIRMADO ──►(10s)──► MENU
      ├─ Pedidos ........ (mock: Ver Pedidos / Pagar Conta)
      └─ Minhas Apostas ──► CARREGANDO ──►[consultarAposta]──► MINHAS_APOSTAS(real)
```
Inatividade (1 min) e retorno automático do CONFIRMADO (10s) continuam. Ao voltar para a tela inicial por inatividade, a sessão do cliente é encerrada (volta a exigir cartão).

## 8. Telas auxiliares novas — **REVISAR LAYOUT**

O back-end tem estados que o `tela-rafa` não previa. Proponho 3 telas auxiliares (layout a validar por vocês no final):

1. **`desenharAguardandoCartao()`** — substitui/adapta a `INICIAL`. Título "BetBar", ícone de cartão/onda, texto central **"Encoste seu cartão para começar"**.
2. **`desenharCarregando(const char* msg)`** — tela genérica de espera de MQTT. Texto central **"Carregando..."** + `msg` opcional ("Buscando partidas", "Autenticando"). Usada em 3 momentos.
3. **`desenharAviso(const char* msg)`** — mensagens de erro/vazio: "Nenhuma partida disponível", "Cartão não reconhecido", "Sem conexão". Um ícone de alerta + a mensagem + dica "aperte esquerda para voltar".

> Estas 3 telas serão desenhadas seguindo o padrão visual existente (296×128, `setFontMode(1)`, centralização manual, sem `getUTF8Width`). No fim da implementação eu destaco os desenhos para vocês aprovarem o layout.

## 9. Pontos de atenção (hardware) — validar no dispositivo

- **SPI compartilhado:** o display ePaper e o leitor RFID usam o mesmo barramento SPI. Display CS=10; RFID SS=46. São CS distintos (ok), mas é preciso garantir o controle correto de chip-select entre os dois. Testar no hardware.
- **Auditoria de pinos:** display (10,14,15,16), botões (1,2,42,41,40), RFID (SS=46, RST=17 + SCK/MISO/MOSI). Conferir que nenhum pino de botão colide com os pinos SPI/RFID reais do ESP32-S3.
- **Bloqueio de rede:** `reconectarWiFi/MQTT` do peixoto1 usam `while(...) delay(1000)` — travam o loop (e os botões) enquanto reconectam. Na integração, tornar não-bloqueante ou aceitar o travamento momentâneo. **A decidir na implementação.**

## 10. `platformio.ini` — bibliotecas a adicionar

Somar às libs atuais (GxEPD2, U8g2_for_Adafruit_GFX, ArduinoJson, GFButton, QRCodeGFX):
- `miguelbalboa/MFRC522` — leitor RFID.
- `256dpi/MQTT` — cliente MQTT (`#include <MQTT.h>`).
- `WiFiClientSecure` / `WiFi` — já vêm no core do ESP32 (sem lib extra).

## 11. Notas para o servidor (parceiro do peixoto1)

- Garantir que o fluxo de partidas responda em **`bet/partidas`** (o `fluxoPrincipal.json` faz isso; o `getPartidas.json` antigo responde em `bet/retornaPartidas` — inconsistente).
- Confirmar que `consultarAposta` responde em **`bet/minhasApostas`** (o firmware passará a se inscrever nele — hoje não se inscrevia; era um bug).

## 12. Ordem de implementação (incremental, cada passo compila e testa)

1. **Trazer módulos + libs.** Copiar wifi/mqtt/rfid/apostas/certificados/modelo para `src/`, ajustar `platformio.ini`. Manter o mock ativo. **Meta: compilar.**
2. **Rede + RFID no setup/loop.** Inicializar e conectar; no serial, ver WiFi/MQTT conectados, UID do cartão lido e `dadosCliente` recebido. Sem mudança de tela ainda.
3. **Entrada por cartão.** Criar `AGUARDANDO_CARTAO` + `CARREGANDO`; splash exige cartão → MENU quando `clienteCarregado`.
4. **Apostar real.** Ao entrar, `getPartidas()` → CARREGANDO → `LISTA_JOGOS` do buffer (mostrando IDs).
5. **Registrar aposta.** `CONFIRMACAO` → `apostasRealizar()` com empate=null → `CONFIRMADO`.
6. **Minhas Apostas real.** Subscribe + handler de `bet/minhasApostas`; `consultarAposta()` → render.
7. **Polimento.** Tela `AVISO` (erros/vazios), timeout nas telas de CARREGANDO, reconexão não-bloqueante.

## 13. Estratégia git

- Criar branch **`integracao`** a partir de `tela-rafa`.
- Trazer os módulos do `peixoto1` para dentro de `src/` (cópia de arquivos, sem merge de histórico).
- Implementar na ordem da §12, commitando por passo.
- No fim, abrir **um PR** de `integracao` → (`tela-rafa` ou `main`, a combinar). `peixoto1` fica intacta.

## 14. Riscos e questões em aberto

- **SPI display+RFID** no mesmo barramento — maior risco de hardware; validar cedo (passo 2).
- **Reconexão bloqueante** trava os botões — definir tratamento no passo 7.
- **Nomes de time** ausentes — telas mostram ID por ora; depende do servidor para evoluir.
- **Confirmação de aposta** (`bet/apostaConfirmada`) não é aguardada; mostramos `CONFIRMADO` de forma otimista. Aceitável nesta etapa.
- **Layout das 3 telas auxiliares** — sujeito à revisão de vocês no fim.
