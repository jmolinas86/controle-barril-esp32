# Relatório da Fase 9 — Troca e retirada de KEG

## Resultado

A Fase 9 implementa a presença física do único KEG sobre `SCALE_MAIN`, preservando o modo híbrido da Fase 8. O catálogo mantém no máximo um registro em `ACTIVE_ON_SCALE` e nenhuma perda de comunicação é interpretada como retirada.

## Máquina de presença

O serviço `KegPresenceService` é independente da interface e da persistência. Ele acompanha o `activeKegId`, um peso de referência e evidências temporais, publicando os estados:

- `NONE`: nenhum KEG ativo;
- `ACTIVE`: KEG identificado e sinais presentes;
- `REMOVAL_SUSPECTED`: ausência de NFC sendo confirmada;
- `UNCERTAIN`: transporte `STALE` ou `OFFLINE`, sem alterar o catálogo.

A decisão forte exige NFC ausente e peso abaixo de 1.000 g por 5 segundos contínuos. A decisão moderada exige NFC ausente por 15 segundos e uma queda mínima de 2.000 g. O retorno do UID, a recuperação da comunicação ou o cancelamento das condições reinicia as evidências.

## Troca e override manual

Um UID conhecido diferente, após o debounce existente e com leitura estável, passa pela ativação atômica do `KegService`: o KEG anterior vira `STORED`, o novo vira `ACTIVE_ON_SCALE` e o catálogo é persistido em um snapshot. O serial registra a retirada por `SWITCH` antes da nova ativação.

A tela de detalhes oferece `RETIRAR`. A ação requer confirmação, usa o mesmo serviço de domínio e registra o motivo `MANUAL`. Se a gravação falhar, o estado anterior é restaurado.

## Simulador de bancada

O transporte simulado alterna, em 75 segundos, KEG 1, KEG 2 e balança vazia. Entre os cenários há períodos sem mensagens para provar que `STALE` e `OFFLINE` não removem um KEG. A balança vazia mantém aproximadamente 450 g, suficiente para confirmar a retirada forte.

## Verificações executadas

- compilação PlatformIO do ambiente `esp32_2432s028r`: sucesso;
- RAM: 124.364 de 327.680 bytes (38,0%);
- flash: 900.969 de 1.900.544 bytes (47,4%);
- os fontes dos testes isolados foram compilados sem erros com o toolchain do ESP32;
- o teste de presença cobre offline seguro, retirada forte, retirada moderada e cancelamento de evidência;
- o teste de domínio cobre retirada explícita, troca A → B e reconciliação de um catálogo inconsistente para somente um `ACTIVE_ON_SCALE`;
- nenhuma alocação dinâmica foi adicionada ao domínio da presença.

## Validação física pendente

O computador não dispõe de um compilador C++ nativo para executar os binários de teste; por isso, além da compilação estática, o firmware precisa ser gravado na placa para executar o ciclo integrado e confirmar os logs seriais, o estado visual e o touch. Os limiares devem ser recalibrados quando o transporte real da balança e, opcionalmente, o PN532 estiverem instalados.
