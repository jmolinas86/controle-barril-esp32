# Relatório da Fase 14 — Histórico por KEG

## Resultado

A Fase 14 implementa histórico persistente separado por `kegId`. Toda pesagem confirmada pelo fluxo NFC ou pelo botão `PESAR` passa pelo `HistoryService`, é armazenada preferencialmente no microSD e passa a alimentar o gráfico da tela de detalhes.

## Componentes entregues

- `models::HistoryRecord`: registro compacto e versionado;
- `storage::IHistoryRepository`: contrato independente do meio físico;
- `storage::SdHistoryRepository`: gravação e leitura incremental em CSV;
- `services::HistoryService`: deduplicação, buffer circular, flush e consulta dos pontos recentes;
- integração com a ativação NFC em `AppController`;
- integração com a confirmação manual em `AppViewModel`;
- gráfico LVGL ligado às sete pesagens reais mais recentes.

## Organização no cartão

Sem um relógio UTC válido nesta fase, cada inicialização usa um identificador de boot:

```text
/history/KEG_001/unsynced-A1B2C3D4.csv
/history/KEG_002/unsynced-A1B2C3D4.csv
```

Cada linha contém:

```text
versao,sequencia,boot,utc,monotonico_ms,peso_bruto_g,peso_filtrado_g,volume_ml,percentual_bp,origem,validade,flags
```

Os arquivos permanecem no cartão mesmo quando o cadastro do KEG é finalizado ou excluído. A exclusão de histórico deverá ser uma ação explícita em uma fase futura.

## Proteções

- IDs de KEG usados no caminho aceitam somente letras, números, `_` e `-`;
- não existe gravação contínua de cada pacote da balança;
- repetição NFC com menos de 60 s e variação menor que 50 mL é ignorada;
- confirmação manual sempre é preservada;
- registros inválidos de peso/configuração não entram no histórico;
- ausência ou falha do cartão mantém até 64 registros num buffer circular em RAM;
- overflow descarta o mais antigo e registra `HISTORY_BUFFER_OVERFLOW`;
- falha do histórico não bloqueia catálogo, balança, temperatura nem LVGL.

## Interface

O gráfico fictício foi substituído por `HISTORICO - ULTIMAS PESAGENS`. Ele mostra no máximo sete volumes persistidos, usa a capacidade do KEG como escala vertical e apresenta a média desses pontos. Como ainda não existe NTP/RTC, o eixo usa `ANTERIOR` e `RECENTE`, evitando exibir datas inventadas.

## Build validado

- ambiente: `esp32_2432s028r`;
- resultado: sucesso;
- RAM estática: 120.652 de 327.680 bytes (36,8%);
- flash: 954.089 de 1.900.544 bytes (50,2%);
- firmware: 954.448 bytes;
- SHA-256: `06DFAD3C9445EF25D7BA3A6A3A1EC4DFAB0F815312CEE7BC32E75882776EB4C0`.

O buffer de 64 registros é reservado uma única vez no heap durante o boot para não pressionar o segmento DRAM estático utilizado pelo display.

## Teste na placa

1. Inserir o microSD e reiniciar.
2. Confirmar `History repository=SD_READY` e `history=SD` no monitor serial.
3. Abrir o detalhe de um KEG, tocar `PESAR` e confirmar uma leitura estável.
4. Confirmar `HISTORY_QUEUED` e `HISTORY_FLUSHED storage=SD`.
5. Conferir o novo ponto no gráfico do mesmo KEG.
6. Repetir com volumes diferentes e confirmar a ordem do gráfico.
7. Reiniciar e verificar que o gráfico é reconstruído a partir do cartão.
8. Reiniciar sem cartão e confirmar `DEGRADED_RAM`; todas as demais funções devem continuar operando.

## Limites conhecidos

- o histórico ainda não possui horário civil porque NTP/RTC não foi implementado;
- registros pendentes somente em RAM não sobrevivem a reinício;
- o firmware ainda não remonta automaticamente um cartão inserido após o boot;
- exportação, filtros por período e limpeza explícita ficam para fases futuras.
