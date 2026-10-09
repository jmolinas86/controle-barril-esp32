# Relatório de entrega — Fase 1

## Estado

A Fase 1 está implementada e compila com sucesso para o ambiente `esp32_2432s028r`.

- PlatformIO / Arduino para ESP32;
- LVGL 9.5.0 e LovyanGFX 1.2.25 com versões fixadas;
- firmware gerado sem warnings na compilação final;
- RAM: 99.436 de 327.680 bytes (30,3%);
- flash: 687.745 de 1.900.544 bytes (36,2%);
- nenhuma chamada a `delay()` no código da aplicação;
- nenhuma chamada direta a hardware na camada `src/ui`.

## Arquivos criados

### Configuração do projeto

- `.gitignore`
- `platformio.ini`
- `partitions.csv`
- `README.md`
- `include/BuildConfig.h`
- `include/lv_conf.h`

### Inicialização e coordenação

- `src/main.cpp`
- `src/app/AppController.h`
- `src/app/AppController.cpp`

### Suporte à placa

- `src/bsp/BoardConfig.h`
- `src/bsp/CydDisplay.h`
- `src/bsp/CydDisplay.cpp`
- `src/bsp/BoardSupport.h`
- `src/bsp/BoardSupport.cpp`

### Diagnóstico e interface inicial

- `src/diagnostics/Logger.h`
- `src/diagnostics/Logger.cpp`
- `src/ui/Phase1DiagnosticUi.h`
- `src/ui/Phase1DiagnosticUi.cpp`

## Arquitetura implementada

O `main.cpp` delega o ciclo de vida ao `AppController`. O controlador inicializa o logger, a abstração da placa e a interface de diagnóstico. A camada `BoardSupport` concentra display, touch, calibração em NVS, microSD e GPIOs seguros. A UI usa apenas essa abstração para leitura do touch e envio de pixels; não acessa periféricos diretamente.

O LVGL usa dois buffers parciais estáticos de 320 × 20 linhas, totalizando 25,6 KiB, para manter uso previsível de RAM. O loop é cooperativo e não bloqueante após a inicialização.

O GPIO candidato do compressor permanece desabilitado por configuração nesta fase.

## Como testar na placa

1. Insira um microSD formatado em FAT32.
2. Conecte a ESP32-2432S028R por USB.
3. Compile com `platformio run -e esp32_2432s028r`.
4. Grave com `platformio run -e esp32_2432s028r -t upload`.
5. Abra o monitor com `platformio device monitor -e esp32_2432s028r`.
6. No primeiro boot, conclua a calibração tocando nos alvos.
7. Este teste foi originalmente criado em landscape 320 × 240; a orientação foi substituída por portrait 240 × 320 durante a validação da Fase 2.
8. Verifique se coordenadas e contador mudam sem toques duplicados.
9. Confirme `SD: OK`; o teste cria, lê e remove `/phase1.tmp`.
10. Deixe o diagnóstico rodar por 30 minutos e confirme que o heap mínimo não cai continuamente.

## Limitações atuais

- A pinagem escolhida corresponde à revisão comum da ESP32-2432S028R e ainda precisa ser confirmada fisicamente na placa do projeto.
- Clones e revisões diferentes podem exigir ajuste de rotação, inversão de cores ou calibração.
- A compilação foi validada; display, touch e microSD não puderam ser testados eletricamente sem a placa conectada e uma execução de bancada.
- Barris, balança, comunicação, telas finais, ViewModel, Wi-Fi de aplicação, histórico e controle térmico pertencem às fases seguintes e não foram antecipados.

## Marco conceitual

`phase-01-base`
