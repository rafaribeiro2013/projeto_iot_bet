# Projeto IoT BET — Terminal de Mesa Interativo

> Dispositivo embarcado (ESP32-S3) instalado em mesas de bar/restaurante que permite ao cliente **consultar o cardápio**, **fazer pedidos**, **apostar em partidas esportivas ao vivo** e **pagar a conta via Pix** — tudo sem precisar chamar um garçom.

---

## 👥 Integrantes do Grupo

| Nome | Matrícula |
|---|---|
| Felipe Peixoto  | 2211750 |
| Rafael Mesquita | 2210234 |
| Rafael Ribeiro  | 2210461 |

---

## Visão Geral

Cada mesa possui um terminal físico composto por uma **tela e-ink** e **cinco botões de navegação**. O cliente se identifica aproximando seu **cartão RFID** (vinculado ao seu copo inteligente) ao leitor. A partir daí, um menu é desbloqueado e toda a comunicação com o servidor acontece de forma transparente via **MQTT**.

```
┌─────────────────────────────────────────────────────────────┐
│                     TERMINAL DE MESA                        │
│   [Tela e-ink]  +  [5 Botões]  +  [Leitor RFID]            │
│                     ESP32-S3                                │
└──────────────────────┬──────────────────────────────────────┘
                       │ MQTT (TLS)
                       ▼
┌──────────────────────────────────────────────────────────────┐
│                    SERVIDOR (Node-RED)                        │
│  • Autenticação do cliente                                   │
│  • Cardápio / Pedidos / Apostas / Conta                      │
│  • Geração de cobrança Pix (API Asaas)                       │
│  • Polling de confirmação de pagamento                        │
└──────────────────────┬───────────────────────────────────────┘
                       │ SQL
                       ▼
              ┌─────────────────┐
              │   PostgreSQL    │
              │  (banco de dados│
              │   do projeto)   │
              └─────────────────┘
```

---

## Hardware Utilizado

| Componente | Modelo | Função |
|---|---|---|
| Microcontrolador | ESP32-S3 DevKitC-1 | Processamento central e Wi-Fi/MQTT |
| Tela | GxEPD2 2.9" e-ink (T94 V2) | Exibição de menus e QR Code |
| Leitor RFID | MFRC522 | Identificação do cliente pelo copo |
| Botões | 5× GFButton | Navegação (↑ ↓ ← → ✓) |

> **Por que e-ink?** A tela de papel eletrônico não consome energia quando a imagem está estática, ideal para um terminal que fica ligado por longas horas.

---

## Fluxo de Uso Típico

```
1. Cliente aproxima o copo (RFID) ao terminal
        │
        ▼
2. Terminal publica o UID do cartão no tópico MQTT "bet/autenticaCliente"
        │
        ▼
3. Node-RED consulta o banco (JOIN copos → vinculo_copo_cliente → clientes)
   e devolve os dados do cliente no tópico "dadosCliente"
        │
        ▼
4. Terminal exibe o MENU principal:
   ┌──────────┬────────────┐
   │ Cardápio │   Apostar  │
   ├──────────┼────────────┤
   │  Pedidos │  Minhas    │
   │          │  Apostas   │
   └──────────┴────────────┘
        │
   (navegação pelos botões físicos)
        │
   ┌────┴──────────────────────────────────────────────┐
   │ Cardápio  → lista categorias → lista produtos      │
   │            → detalhe → CONFIRMAR PEDIDO            │
   │                                                    │
   │ Cervejas  → status ao vivo do copo inteligente     │
   │             (temperatura, volume, bateria)         │
   │                                                    │
   │ Apostar   → lista partidas ao vivo → escolhe time  │
   │             → tela de confirmação → APOSTA FEITA   │
   │                                                    │
   │ Pedidos   → meus pedidos + total da conta          │
   │          → Pagar Conta → gera QR Code Pix          │
   └───────────────────────────────────────────────────┘
        │
5. Ao pagar (Pix confirmado), o copo é desvinculado da mesa
   e o terminal volta para a tela inicial de aguardo
```

---

## Arquitetura do Firmware (ESP32-S3)

O firmware é escrito em **C++ com framework Arduino**, organizado em módulos independentes:

```
src/
├── main.cpp          # Setup, loop principal, callback MQTT
├── navegacao.h       # Máquina de estados das telas + handlers dos botões
├── dados.h           # Camada de repositório (abstrai a fonte dos dados)
├── desenhos.h        # Funções de renderização na tela e-ink
├── modelo.h          # Structs de dados (Cliente, Partida, Aposta, Produto, Copo)
│
├── apostas.cpp/.h    # Módulo de apostas (publica/recebe via MQTT)
├── cardapio.cpp/.h   # Módulo de cardápio, pedidos, conta e Pix
├── mqtt_module.cpp   # Conexão e reconexão MQTT (não-bloqueante)
├── rfid_module.cpp   # Leitura do cartão RFID
├── wifi_module.cpp   # Conexão e reconexão Wi-Fi
│
├── estado_dados.h    # Variáveis globais de estado (buffers, flags)
├── globais.h         # Objetos globais (tela, QRCode, fontes)
├── preferencia.h     # Número da mesa salvo em NVS (flash persistente)
└── certificados.h    # Certificado TLS para a conexão MQTT segura
```

### Máquina de Estados das Telas

A navegação é gerenciada por um **enum `Tela`** e uma **pilha de navegação** (até 8 níveis). Isso permite que o botão "Voltar" (←) funcione de forma genérica, sem precisar mapear manualmente qual tela é pai de qual.

```
AGUARDANDO_CARTAO
    └─► CARREGANDO (autenticando...)
            └─► MENU
                 ├─► CARDAPIO → LISTA_PRODUTOS → DETALHE_PRODUTO
                 │                             └─► (registra pedido)
                 │   └─► CONTROLE_CERVEJA (dados do copo ao vivo)
                 │
                 ├─► LISTA_JOGOS → JOGO → CONFIRMACAO → CONFIRMADO
                 │
                 ├─► MENU_PEDIDOS → MEUS_PEDIDOS
                 │              └─► PAGAR_CONTA (QR Code Pix)
                 │                          └─► PIX_CONFIRMADO
                 │
                 └─► MINHAS_APOSTAS
```

---

## Backend — Fluxos Node-RED

Os fluxos Node-RED fazem a ponte entre o terminal MQTT e o banco de dados. Cada fluxo é exportado como um arquivo `.json` neste repositório e pode ser importado diretamente no Node-RED.

| Arquivo | Descrição |
|---|---|
| `fluxoPrincipal.json` | Autenticação RFID, consulta/registro de apostas, listagem de partidas |
| `fluxoCardapioPedidosConta.json` | Cardápio, pedidos, copo inteligente, total da conta |
| `fluxoPix.json` | Geração de cobrança Pix via API Asaas e polling de confirmação de pagamento |

### Fluxo de Pagamento Pix (`fluxoPix.json`)

1. Terminal publica o valor total no tópico `conta`
2. Node-RED chama a **API Asaas** (`POST /v3/lean/payments`) para criar a cobrança
3. Obtém o QR Code (`GET /v3/payments/{id}/pixQrCode`) e publica no tópico `codigoPix`
4. A cada **10 segundos**, um timer verifica o status de todas as cobranças pendentes (`GET /v3/payments/{id}/status`)
5. Ao detectar `status = RECEIVED`, publica no tópico `liberaPix/{numMesa}`
6. O terminal exibe a tela de **"Pagamento confirmado!"** e desvincula o copo

---

## Tópicos MQTT

| Tópico | Direção | Descrição |
|---|---|---|
| `bet/autenticaCliente` | ESP32 → Servidor | Solicita autenticação pelo RFID |
| `dadosCliente` | Servidor → ESP32 | Dados do cliente autenticado |
| `bet/getPartidas` | ESP32 → Servidor | Solicita partidas em andamento |
| `bet/partidas` | Servidor → ESP32 | Lista de partidas disponíveis |
| `bet/realizarAposta` | ESP32 → Servidor | Registra uma nova aposta |
| `bet/consultarAposta` | ESP32 → Servidor | Solicita apostas do cliente |
| `bet/minhasApostas` | Servidor → ESP32 | Lista de apostas do cliente |
| `bet/getCardapio` | ESP32 → Servidor | Solicita produtos de uma categoria |
| `bet/cardapio` | Servidor → ESP32 | Lista de produtos |
| `bet/registrarPedido` | ESP32 → Servidor | Registra um pedido |
| `bet/getMeusPedidos` | ESP32 → Servidor | Solicita pedidos do cliente |
| `bet/meusPedidos` | Servidor → ESP32 | Lista de pedidos do cliente |
| `bet/getTotalConta` | ESP32 → Servidor | Solicita o total da conta |
| `bet/totalConta` | Servidor → ESP32 | Valor total da conta |
| `conta` | ESP32 → Servidor | Solicita geração de cobrança Pix |
| `codigoPix` | Servidor → ESP32 | Payload do QR Code Pix |
| `liberaPix/{mesa}` | Servidor → ESP32 | Confirmação de pagamento recebido |
| `bet/getCopo` | ESP32 → Servidor | Solicita dados do copo inteligente |
| `bet/copo` | Servidor → ESP32 | Status do copo (volume, temperatura, bateria) |
| `bet/liberarCopo` | ESP32 → Servidor | Avisa que o copo pode ser desvinculado |

> A conexão MQTT é feita com **TLS na porta 8883**, usando o broker `mqtt.janks.dev.br`.

---

## Comportamentos de Segurança / UX

- **Inatividade:** Se nenhum botão for pressionado por **1 minuto**, o terminal volta automaticamente para a tela de aguardo e desautentica o cliente.
- **Timeout de carregamento:** Se o servidor não responder em **8 segundos**, exibe a mensagem "Sem resposta" e aguarda nova ação.
- **Tela de confirmado:** Após confirmar uma aposta, a tela de sucesso volta sozinha para o menu em **10 segundos**.
- **MQTT não-bloqueante:** A reconexão MQTT é feita no `loop()` com no máximo uma tentativa a cada 3s, garantindo que a tela e os botões nunca travem.

---

## Dependências (PlatformIO)

```ini
lib_deps =
    zinggjm/GxEPD2                            ; Driver da tela e-ink
    olikraus/U8g2_for_Adafruit_GFX            ; Fontes vetoriais na tela
    bblanchon/ArduinoJson                     ; Serialização/desserialização JSON
    geekfactory/GeekFactory GFButton Library  ; Debounce dos botões
    wallysalami/QRCodeGFX                     ; Geração de QR Code na tela
    miguelbalboa/MFRC522                      ; Leitor RFID
    256dpi/MQTT                               ; Cliente MQTT com TLS
```

---

## Como Compilar e Gravar

1. Instale o [PlatformIO](https://platformio.org/) (extensão do VS Code ou CLI).
2. Clone este repositório.
3. Abra a pasta no VS Code com PlatformIO instalado.
4. Conecte o ESP32-S3 via USB.
5. Clique em **Upload** (ou `pio run -t upload`).

> **Atenção:** O número da mesa é salvo na memória flash (NVS) do ESP32. Na primeira gravação, ajuste-o pelo Serial Monitor ou por uma função de configuração inicial.

---

## Como Importar os Fluxos Node-RED

1. Acesse a interface do Node-RED.
2. Menu ≡ → **Import** → cole o conteúdo de um dos arquivos `.json`.
3. Configure as credenciais do broker MQTT e do banco PostgreSQL nos nós de configuração.
4. Configure o `access_token` da API Asaas no fluxo Pix.
5. Clique em **Deploy**.
