# Relatório da Fase 7 — Balança

## Resultado

A Fase 7 implementa a entrada de dados da única balança física conceitual `SCALE_MAIN`, inicialmente por um transporte simulado. A camada não conhece cadastro, tara, capacidade, cerveja ou histórico e, portanto, não associa nem modifica KEGs antes da Fase 8.

## Componentes

- `models/Scale.h/.cpp`: `ScalePacket`, `ScaleReading`, `ScaleState` e `ScaleLinkStatus`;
- `scale/IScaleTransport.h`: contrato independente do meio de comunicação;
- `scale/ScaleProtocol.h/.cpp`: validação e normalização do pacote v1;
- `scale/SimulatedScaleTransport.h/.cpp`: fonte cíclica para teste de bancada;
- `services/ScaleService.h/.cpp`: consumo cooperativo, estado da conexão, sequência e última leitura estável;
- `AppController`: inicialização e atualização do serviço sem bloquear a UI;
- `AppViewModel`: snapshot somente leitura da balança;
- `SettingsScreen`: diagnóstico compacto no cartão `BALANCA`;
- `test/scale_service_smoke.cpp`: cenários determinísticos do contrato do serviço.

## Regras implementadas

- existe somente uma balança, identificada obrigatoriamente como `SCALE_MAIN`;
- protocolo aceito: versão 1;
- UID NFC é normalizado para hexadecimal maiúsculo sem separadores;
- UID malformado, peso fora de 0–100 kg, bateria acima de 100% e RSSI positivo são rejeitados;
- pacotes repetidos ou antigos não substituem o estado aceito;
- redução do uptime do emissor permite reinício da sequência após reboot da balança;
- leitura estável atualiza o peso filtrado provisório e `latestStableReading()`;
- leitura instável permanece disponível como peso bruto, mas não substitui a última leitura estável;
- `ONLINE`, `STALE` e `OFFLINE` são derivados da idade da última leitura;
- a ausência da balança não bloqueia touch, display, catálogo ou controle local.

## Temporização

Produção:

- `ONLINE`: última leitura com menos de 30 segundos;
- `STALE`: entre 30 segundos e 5 minutos;
- `OFFLINE`: sem leitura ou com 5 minutos ou mais.

Simulador de bancada:

- envia uma leitura por segundo durante 12 segundos;
- fica silencioso até completar 25 segundos;
- usa 3 segundos para `STALE` e 7 segundos para `OFFLINE`;
- reinicia o envio no início do ciclo seguinte.

O simulador envia UID `04A23F891C`, peso próximo de 18,55 kg, bateria 92% e RSSI −58 dBm. Esses valores são apenas entrada da balança e não são atribuídos a um KEG na Fase 7.

## Validação realizada

Comando:

```powershell
platformio run -e esp32_2432s028r
```

Resultado: compilação concluída com sucesso.

- RAM: 124.052 de 327.680 bytes (37,9%);
- flash: 894.193 de 1.900.544 bytes (47,0%);
- firmware: 894.560 bytes;
- SHA-256: `C7AD7BC71E27590CBB7C433F94FD3372925574AD366C9124CE4A4EE7A01CFF80`.

Também foi executada verificação sintática isolada do modelo, protocolo, serviço e teste de fumaça com o compilador Xtensa, sem erros.

## Teste recomendado na placa

1. Grave o firmware e abra o monitor em 115200 baud.
2. Confirme `Phase 7 initialized: ... scale=SIMULATOR`.
3. Confirme `Link state=ONLINE`, `NFC detected: 04A23F891C` e `Weight received`.
4. Abra `CONFIG.` e confira `ONLINE`, aproximadamente `18.55kg` e `NFC OK` no cartão `BALANCA`.
5. Após a pausa do simulador, confirme `STALE` e depois `OFFLINE`.
6. No começo do ciclo seguinte, confirme o retorno para `ONLINE`.
7. Repita dois ciclos e confirme que a UI continua responsiva.
8. Confira que nenhum KEG foi ativado, trocado ou alterado pela leitura simulada.

## Limites da fase

- não existe transporte físico HTTP/ESP-NOW nesta etapa;
- não há identificação ou ativação automática do KEG pelo UID;
- não há debounce temporal de presença NFC;
- o peso publicável ainda não usa a janela de mediana planejada para a Fase 11;
- leituras não são gravadas em histórico;
- os timeouts curtos são exclusivos do simulador de bancada.

A próxima etapa é a Fase 8: estabilizar o UID, localizar o cadastro por `findByNfcUid()` e controlar a associação temporária do único KEG ativo.
