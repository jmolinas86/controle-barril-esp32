# Relatório de entrega — Fase 3

## Estado

A HOME da Fase 3 está implementada e compila para `esp32_2432s028r`. O visual aprovado na Fase 2.5 foi preservado. A validação final de fluidez, touch e estabilidade deve ser feita na placa física.

- resolução: 240 × 320 portrait;
- RAM estática: 93.452 de 327.680 bytes (28,5%);
- flash: 809.369 de 1.900.544 bytes (42,6%);
- firmware: `.pio/build/esp32_2432s028r/firmware.bin`;
- tamanho do firmware: 809.728 bytes;
- SHA-256: `B05E5969AEE4B0F8438F7CBE2F5C54C46905B46780380AC513BCF1C39984A314B`;
- compilação: concluída sem erros e sem avisos do código da aplicação.

## Implementado

- `FreezerCard` pode atualizar temperatura, setpoint e compressor sem ser recriado;
- estados do compressor usam verde durante resfriamento, âmbar em espera, cinza quando parado e vermelho em erro;
- o floco de neve do compressor aparece somente enquanto o compressor está ligado;
- `KegCard` atualiza nome, volume, percentual, barra, sincronização e estado `LENDO/LIDO`;
- textos só são alterados no LVGL quando o valor realmente muda;
- a HOME consulta o `AppViewModel` a cada 250 ms;
- uma mudança estrutural na quantidade de KEGs reconstrói somente o conteúdo necessário;
- estado `NENHUM KEG` preparado para lista vazia;
- cabeçalho, card fixo do keezer, scroll dos KEGs e menu inferior permanecem como aprovados;
- telemetria serial registra heap livre/mínimo, FPS aproximado e tempo médio/máximo do handler LVGL a cada 30 segundos.

## Arquivos alterados

- `include/BuildConfig.h`
- `src/app/AppController.cpp`
- `src/ui/UIManager.h/.cpp`
- `src/ui/ScreenManager.h/.cpp`
- `src/ui/screens/HomeScreen.h/.cpp`
- `src/ui/widgets/FreezerCard.h/.cpp`
- `src/ui/widgets/KegCard.h/.cpp`
- `README.md`

## Como testar

1. Grave com `platformio run -e esp32_2432s028r -t upload`.
2. Abra `platformio device monitor -e esp32_2432s028r`.
3. Confirme que o visual da HOME não mudou em relação à Fase 2.5.
4. Role os cards, abra os três detalhes e retorne à HOME.
5. Navegue entre HOME, KEEZER e CONFIG. repetidamente por dez minutos.
6. A cada 30 segundos, confira no serial `Heap free`, `minimum`, `FPS approx`, `handler avg` e `max`.
7. Verifique que o heap mínimo estabiliza e que não ocorre reinicialização ou perda de resposta ao touch.

## Limitações atuais

- valores, Wi-Fi e horário continuam simulados;
- o `AppViewModel` ainda fornece dados constantes de `DemoData`;
- o estado vazio está implementado, mas não é selecionável no modo demonstração atual;
- FPS é uma estimativa baseada nos ciclos de atualização concluídos do display;
- sensores, balança, NFC, compressor real, armazenamento e rede não foram iniciados.

A próxima etapa prevista é a Fase 4: tornar o `AppViewModel` a fonte de estado dinâmica e retirar das telas qualquer dependência de dados fixos.
