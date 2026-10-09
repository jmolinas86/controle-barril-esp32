# Arquitetura do Keezer Controller

## Visão geral

O sistema é formado por dois equipamentos independentes na mesma rede local:

1. a **balança**, que adquire HX711, determina estabilidade e informa o UID NFC
   quando o PN532 está disponível;
2. o **controlador ESP32**, que consulta a balança, identifica o KEG, calcula
   volume, persiste dados, controla a interface e executa a lógica térmica.

O controlador não depende de internet nem de servidor externo. A integração é
feita por HTTP local e o firmware continua operando de forma degradada quando
Wi-Fi, balança ou microSD ficam indisponíveis.

## Camadas

| Camada | Responsabilidade | Diretórios principais |
|---|---|---|
| BSP | display, touch, microSD e saídas seguras | `src/bsp` |
| Transporte | HTTP da balança, OTA web e simulador | `src/scale`, `src/network`, `src/ota` |
| Serviços | regras de KEG, peso, presença, histórico, rede e temperatura | `src/services` |
| Domínio | fórmulas e validações independentes de hardware | `src/domain`, `src/models` |
| Persistência | catálogo LittleFS, configurações NVS e histórico SD | `src/storage` |
| Aplicação | composição, ordem de atualização e eventos | `src/app` |
| Interface | rotas, telas, widgets e tema LVGL | `src/ui` |

As camadas de domínio não acessam Wi-Fi, display, arquivos ou GPIO. Os serviços
recebem interfaces (`IScaleTransport`, `IKegRepository`,
`IHistoryRepository`, `ITemperatureSensor`, `ICompressorOutput`) para permitir
simulação e testes independentes.

## Ciclo cooperativo

O `AppController::update()` executa, sem tarefas bloqueantes longas:

1. rede, estado de conexão e servidor OTA;
2. sensor e controle térmico;
3. transporte e filtro da balança;
4. identificação NFC;
5. presença/troca do KEG;
6. acompanhamento automático;
7. fila de histórico e recuperação do SD;
8. ViewModel, LVGL e telemetria de desempenho.

Durante um upload OTA, as etapas de negócio ficam temporariamente pausadas para
evitar escrita concorrente na flash; a interface e o servidor continuam ativos.
Fora dessa manutenção, uma falha de um subsistema não deve paralisar os demais. Por exemplo, retirar o
SD mantém a UI, a balança e a temperatura em execução; perder a balança não
remove o KEG ativo por inferência.

## Fluxo de uma medição

```text
GET /api/v1/reading
        |
        v
ScaleHttpJson -> ScaleProtocol -> ScaleService -> WeightFilter
        |                                  |
        |                                  +-> NFC estabilizado
        |                                  +-> peso filtrado
        v
KegPresenceService / KegTrackingService
        |
        v
KegCalculator -> KegService -> LittleFS
        |
        +-> HistoryService -> RAM (fallback) -> microSD
        +-> AppViewModel -> LVGL
```

## Princípios de segurança

- no máximo um KEG fica ativo sobre a balança;
- sem NFC, o sistema nunca escolhe outro KEG apenas pelo peso;
- salto ambíguo pausa o acompanhamento e solicita confirmação;
- catálogo usa dois snapshots alternados com CRC32;
- falha térmica desliga imediatamente o comando lógico;
- saída física do compressor ainda não integra o serviço desta versão;
- estruturas contínuas usam tamanho fixo e evitam alocação dinâmica.

## Configuração de compilação atual

O ambiente `esp32_2432s028r` seleciona CYD/ST7789, LVGL 9.5,
LovyanGFX 1.2.25, transporte HTTP real e modo de demonstração térmica. Os
limites e tempos ficam centralizados em `include/BuildConfig.h`; pinagem em
`src/bsp/BoardConfig.h`; opções do ambiente em `platformio.ini`.

O histórico de decisões por fase permanece em `outputs/`, inclusive a
arquitetura detalhada evolutiva em `outputs/ARCHITECTURE.md`.
