# Relatório da Fase 15 — Comunicação real com a balança

## Resultado

A Fase 15 substitui a fonte simulada padrão pelo transporte HTTP real da
balança e preserva integralmente a abstração `IScaleTransport`. O controlador
consulta o snapshot atômico da balança em `GET /api/v1/reading`, converte-o
para `ScalePacket` e mantém protocolo, domínio, catálogo e interface
desacoplados.

## Componentes entregues

- `HttpScaleTransport`: Wi-Fi, mDNS, consulta HTTP cooperativa, timeout,
  limite de resposta e recuperação automática;
- `ScaleHttpJson`: conversão do JSON v1 para estruturas fixas;
- `ScaleProtocol`: validação já existente reutilizada sem dependência de HTTP;
- seleção por `KEEZER_SCALE_USE_HTTP`, preservando `SimulatedScaleTransport`;
- configuração centralizada em `include/NetworkConfig.h`;
- contrato formal em `outputs/SCALE_PROTOCOL.md`;
- smoke test do parser com leitura completa, ausência de NFC e entradas
  inválidas.

## Decisão de integração

O esboço original previa HTTP push. Depois da validação e rearquitetura do
firmware real da balança, o contrato aprovado passou a ser HTTP pull: a
balança fornece uma fotografia atômica e o controlador a consulta a cada
500 ms. Essa decisão foi registrada também em `outputs/ARCHITECTURE.md`.

## Comportamento operacional

- `balanca.local` é resolvido por mDNS com espera limitada e IP em cache;
- a instalação atual usa diretamente o IP fixo `10.0.0.188`;
- após três falhas de comunicação, o endereço é resolvido novamente;
- `HTTP 200` com JSON válido entrega um pacote ao serviço;
- `HTTP 503` informa balança acessível, porém sem leitura válida do HX711;
- resposta grande, inválida ou expirada é descartada;
- sequência repetida não cria nova leitura nem novo histórico;
- redução de `uptime_ms` permite recuperação após reinício da balança;
- `nfc_uid=null` mantém disponível a pesagem manual do modo híbrido;
- perda de Wi-Fi ou da balança não bloqueia LVGL, histórico ou controle
  térmico.

## Configuração necessária

Antes da gravação física, preencher a rede do controlador em
`include/NetworkConfig.h`, ou deixar os campos vazios somente se o ESP32 já
possuir credenciais Wi-Fi gravadas. O endereço padrão é `balanca.local`; o IP
da balança pode ser usado como alternativa.

## Validação

- o parser de produção foi compilado sem avisos pelo toolchain do ESP32;
- `test/scale_http_json_smoke.cpp` contém vetores de resposta completa,
  ausência de NFC, RSSI fora da faixa, UID excedido e booleano malformado; a
  execução nativa isolada não foi possível porque não há compilador C++ de
  desktop instalado nesta máquina;
- build PlatformIO `esp32_2432s028r`: sucesso;
- RAM estática: 116.584 de 327.680 bytes (35,6%);
- flash: 1.437.617 de 1.900.544 bytes (75,6%);
- binário: 1.444.192 bytes;
- SHA-256: `F3BF82197E1F2EDC5214839FB26CECDCF39F2C14BFA2D48584C1FA2451859B8F`;
- integração física: deve ser confirmada com as duas placas na mesma rede.

## Limites conhecidos

- provisionamento Wi-Fi completo e tela de configuração pertencem à Fase 16;
- a API atual não possui autenticação e deve ficar restrita à LAN confiável;
- a confirmação física de reconexão e leitura real depende da gravação deste
  firmware no controlador.
