# Cardápio, Pedidos e Copo (Cerveja) — Design

## Contexto

O firmware já integra apostas ao banco real via MQTT (branch `integracao`, mergeada em `main`), usando o padrão: **buffer global de cache + flag `precisaRedesenhar`** para ligar callbacks assíncronos do MQTT à renderização síncrona do ePaper. Este design estende o mesmo padrão para três telas que hoje usam dados mockados em `dados.h`:

- **Cardápio** (`obterProdutos`) — lista de produtos por categoria
- **Pedidos** (`obterPedidos`, `registrarAposta`-like) — registrar e listar pedidos do cliente
- **Controle de Cerveja** (`obterConsumoCerveja`) — hoje mock de gasto/copos/grátis; será redesenhada

Fora de escopo: **fechamento de conta/pagamento** (`obterTotalConta`) — responsabilidade de outro integrante do grupo, permanece mock e não é tocado aqui além de deixar o comentário explícito.

## Tabelas existentes no banco (Postgres `projeto_b`)

Fornecidas pelo time; **nenhuma tabela nova é necessária exceto `pedidos`**.

**`cardapio`** (já existe):
```
id, created_at, preco (float), categoria (varchar), nome_comida (varchar), descricao (varchar)
```
Valores de `categoria`: `"Drinks"`, `"Petiscos"`, `"Nao Alcoolico"` (maiúscula inicial, sem acento — exatamente assim no banco).

**`copos`** (já existe — modelo de **estado atual**, uma linha por copo físico, sem histórico):
```
id, created_at, status (varchar: "em_uso" | "disponivel"), id_copo (numeric),
rfid (varchar), id_cliente (int4), nivel_bateria, quantidade_ml (int4),
temperatura_c (float8), mesa (int4)
```
O `rfid` do copo é o mesmo RFID do cartão do cliente (o mesmo usado na autenticação via `bet/autenticaCliente`) — todo cliente autenticado sempre tem um copo pareado.

**`pedidos`** (a ser criada pelo time, a partir deste design):
```
id, created_at, id_cliente (int4), id_produto (int4, FK -> cardapio.id),
status (varchar, default 'pendente'), mesa (int4)
```
`mesa` é preenchido com o mesmo valor de `copos.mesa` do cliente (não precisa nova consulta — já teremos isso disponível ao autenticar, ver seção de fluxo).

## Por que `copos` muda o design da tela de cerveja

A tela atual mocka `gastoCentavos`, `copos` (contagem) e `cervejasGratis` — números que exigiriam um **histórico** de consumo. Como `copos` é estado atual (uma linha por copo físico, sobrescrita a cada leitura), esses números não são deriváveis sem uma tabela de histórico que não existe e não é escopo deste projeto (fica com quem cuida do fechamento).

Por isso a tela é redesenhada como um **monitor ao vivo do copo do cliente**: nível de líquido (ml), temperatura, status (`em_uso`/`disponivel`) e bateria — dados reais que a tabela já fornece, buscados uma vez ao entrar na tela (sem auto-refresh, mesmo padrão das demais telas).

## Tópicos MQTT (a implementar no Node-RED, seguindo o padrão de `fluxoPrincipal.json`)

| Tópico (publica) | Payload | Tópico (resposta) | Formato da resposta |
|---|---|---|---|
| `bet/getCardapio` | `{"categoria":"Drinks"}` | `bet/cardapio` | array de produtos: `{id, preco, categoria, nome_comida, descricao}` |
| `bet/registrarPedido` | `{"id_cliente":N, "id_produto":M}` | *(nenhuma — fire-and-forget, igual `bet/realizarAposta`)* | — |
| `bet/getMeusPedidos` | `{"id_cliente":N}` | `bet/meusPedidos` | array de pedidos **já com JOIN em `cardapio`**: `{id, id_produto, nome_comida, preco, status}` |
| `bet/getCopo` | `{"rfid":"<uid>"}` | `bet/copo` | objeto único: `{status, id_copo, id_cliente, nivel_bateria, quantidade_ml, temperatura_c, mesa}` |

O JOIN em `bet/meusPedidos` evita que o firmware precise cruzar `pedidos` com `produtos` localmente — o servidor já devolve nome e preço prontos, no mesmo espírito da query de autenticação que já faz `JOIN copos ON clientes.id = copos.id_cliente`.

## Modelo de dados (novo em `modelo.h`)

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
  String  nomeComida;   // do JOIN em bet/meusPedidos
  float   preco;        // do JOIN em bet/meusPedidos
  String  status;
};

struct Copo {
  int32_t id;
  String  status;
  int32_t idCopo;
  String  rfid;
  int32_t idCliente;
  int     nivelBateria;
  int     quantidadeMl;
  float   temperaturaC;
  int     mesa;
};
```

**Nota sobre `preco` (float no banco):** o código atual formata preço como inteiro em reais (`snprintf(buf, "R$ %d,00", preco)`, sem centavos). Mantemos essa convenção: `Produto.preco`/`PedidoItem.preco` guardam o `float` recebido do banco, mas a exibição continua arredondando para inteiro ao montar o JSON que vai para `desenhos.h` (ex.: `o["preco"] = (int)roundf(produtos[i].preco);`), sem introduzir exibição de centavos nesta etapa.

## Buffers de cache (novo em `estado_dados.h` / definidos em `main.cpp`)

```cpp
Produto    produtos[20];    uint8_t totalProdutos;    bool produtosProntos;
PedidoItem meusPedidos[20]; uint8_t totalMeusPedidos; bool meusPedidosProntos;
Copo       copoAtual;       bool    copoProntos;
String     rfidAtual;       // UID lido na autenticacao, reaproveitado para getCopo()
```

`rfidAtual` é preenchido no `loop()` de `main.cpp`, no mesmo ponto onde hoje já lemos `rfidLerUID()` para publicar em `bet/autenticaCliente` — só passa a guardar o valor também.

## Fluxo de telas

### Cardápio → Lista de Produtos
1. Usuário está no `MENU`, entra em `CARDAPIO` (lista com Cervejas/Drinks/Petiscos/Não Alcoólico).
2. Ao selecionar uma categoria de bebida/comida (índices 1–3), `selecionar()` chama `getCardapio(categoria)` (categoria já convertida para `"Drinks"`/`"Petiscos"`/`"Nao Alcoolico"`), empilha, vai para `CARREGANDO` com `ESPERANDO_PRODUTOS`.
3. Quando `bet/cardapio` chega, o parser enche `produtos[]`, seta `produtosProntos = true` e `precisaRedesenhar = true`.
4. `renderizarTelaAtual()` no case `CARREGANDO` detecta `produtosProntos` e muda para `LISTA_PRODUTOS`.
5. `obterProdutos()` em `dados.h` lê de `produtos[]` (mesmo formato `{"nome","preco","descricao"}` que `desenhos.h` já espera — nenhuma mudança em `desenhos.h` para esta tela).

### Detalhe do Produto → Pedir
1. Na tela `DETALHE_PRODUTO`, o botão "PEDIR" hoje só loga no serial.
2. Passa a chamar `registrarPedido(produtoId)` em `dados.h`, que delega para `cardapio.h`'s `cardapioRegistrarPedido(clienteAtual.id, produtoId)` — publica em `bet/registrarPedido` (fire-and-forget, sem tela de carregamento, igual `registrarAposta`).
3. Volta para a tela anterior (comportamento já existente, só troca o log pelo publish real).

### Cervejas → Monitor do Copo
1. Na tela `CARDAPIO`, ao selecionar "Cervejas" (índice 0), `selecionar()` chama `getCopo(rfidAtual)`, vai para `CARREGANDO` com `ESPERANDO_COPO`.
2. Quando `bet/copo` chega, o parser preenche `copoAtual`, seta `copoProntos = true` e `precisaRedesenhar = true`.
3. `CARREGANDO` detecta `copoProntos` e muda para `CONTROLE_CERVEJA`.
4. `desenharControleCerveja()` muda de assinatura — de `(int gastoCentavos, int copos, int cervejasGratis)` para `(int quantidadeMl, float temperaturaC, const char* status, int nivelBateria)` — mostrando o estado ao vivo do copo.

### Meus Pedidos
1. Em `MENU_PEDIDOS`, ao selecionar "Meus Pedidos", `selecionar()` chama `getMeusPedidos(clienteAtual.id)`, vai para `CARREGANDO` com `ESPERANDO_MEUS_PEDIDOS` (mesmo padrão de "Minhas Apostas").
2. Quando `bet/meusPedidos` chega, enche `meusPedidos[]`, seta `meusPedidosProntos = true` e `precisaRedesenhar = true`.
3. `obterPedidos()` em `dados.h` lê de `meusPedidos[]`.
4. **`obterTotalConta()` continua mockado** — não é alterado neste trabalho. O comentário no código será atualizado para deixar claro que é um stub aguardando a implementação do fechamento de conta (responsabilidade de outro integrante).

## Limpeza pontual: `CARREGANDO` com enum nomeado

Hoje `estado.indice` dentro de `CARREGANDO` usa números mágicos (`0`=cliente, `1`=partidas, `2`=apostas) para saber o que está sendo esperado. Com 3 novos casos, isso vira um enum:

```cpp
enum EsperandoDado {
  ESPERANDO_CLIENTE, ESPERANDO_PARTIDAS, ESPERANDO_APOSTAS,
  ESPERANDO_PRODUTOS, ESPERANDO_MEUS_PEDIDOS, ESPERANDO_COPO
};
```

`estado.indice` passa a receber esses valores (cast implícito para `int`), sem mudar o campo em si — é só uma limpeza de legibilidade, sem impacto em comportamento.

## Arquivos afetados

```
src/
  cardapio.h/.cpp   [NOVO] getCardapio(categoria), cardapioRegistrarPedido(idCliente, idProduto),
                    getMeusPedidos(idCliente), getCopo(rfid) + parsers das 3 respostas +
                    cardapioProcessarMensagem(topico, conteudo) (mesmo papel de
                    apostasProcessarMensagem)
  modelo.h          + structs Produto, PedidoItem, Copo
  estado_dados.h    + buffers produtos[20]/meusPedidos[20]/copoAtual + flags + rfidAtual
  dados.h           obterProdutos/obterPedidos/obterConsumoCerveja passam a ler os buffers;
                    nova função registrarPedido(produtoId) delega para cardapio.h;
                    obterTotalConta() inalterado, comentário de MOCK reforçado
  navegacao.h       CARDAPIO dispara getCardapio ou getCopo conforme o índice;
                    DETALHE_PRODUTO dispara registrarPedido; MENU_PEDIDOS dispara
                    getMeusPedidos; enum EsperandoDado substitui números mágicos;
                    strings de categoria corrigidas para "Drinks"/"Petiscos"/"Nao Alcoolico"
  desenhos.h        apenas desenharControleCerveja muda de assinatura
  main.cpp          define os buffers novos; inclui cardapio.h; despacha os 3 tópicos novos
                    em mqttMensagemRecebida(); guarda rfidAtual ao ler o cartão RFID
```

## Testes

Mesmo critério do trabalho anterior: **compilar** (`pio run`) a cada passo, e testar no dispositivo com Serial Monitor a 115200 quando envolver MQTT/RFID. Não há framework de testes unitários rodando no ESP32 — a verificação é observacional (compilação + comportamento no hardware/serial).

## Fora de escopo (explícito)

- Fechamento de conta / pagamento (`obterTotalConta`, tela `PAGAR_CONTA`) — outro integrante.
- Criação das tabelas/queries no Node-RED e Postgres — acontece na aula, com acesso da equipe; este documento serve de especificação para levar a essa etapa.
- Histórico de consumo de cerveja (gasto, cerveja grátis) — não modelado no banco atual; não é resolvido aqui.
