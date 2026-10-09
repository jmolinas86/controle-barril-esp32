# Fase 19.2 — Configurações locais

## Implementado

- Tela de KEGs arquivados acessível por `CONFIG. > BARRIS`.
- Histórico gráfico individual para cada cadastro arquivado.
- Exclusão definitiva do cadastro após confirmação; o histórico bruto no SD
  permanece preservado.
- Cartão `OTA` no lugar do antigo cartão `TEMPERATURA`, com endereço de acesso
  calculado a partir do IP atual do controlador.
- Tela `DISPLAY` com brilho entre 10% e 100% e repouso configurável em 30 s,
  1 min, 2 min, 5 min ou sempre ligado.
- Persistência das preferências de display em NVS.
- Primeiro toque após o repouso dedicado exclusivamente a acordar a tela.
- Estado ONLINE/STALE/OFFLINE da balança separado verticalmente do título.

## Validação de bancada recomendada

1. Arquivar um KEG, confirmar que ele some da HOME e aparece em `BARRIS`.
2. Abrir o arquivado e conferir os últimos pontos do histórico.
3. Tocar em `APAGAR DEFINITIVO`, cancelar uma vez e depois confirmar.
4. Abrir `OTA` conectado à rede e conferir o endereço `/update`.
5. Salvar brilho e repouso, reiniciar e verificar se foram preservados.
6. Esperar o repouso; confirmar que o primeiro toque só acorda e o segundo
   executa a ação desejada.

## Build

- Ambiente: `esp32_2432s028r`
- Resultado: aprovado pelo PlatformIO.
- RAM estática: 124.560 bytes (38,0%).
- Flash: 1.528.709 bytes (80,4%).
