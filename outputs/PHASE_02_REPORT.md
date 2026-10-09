# Relatório de entrega — Fase 2

## Estado

A Fase 2 está implementada e compila com sucesso para `esp32_2432s028r`.

A interface foi posteriormente redesenhada para portrait 240 × 320 por solicitação do usuário, após a validação do painel ST7789 na placa física.

- RAM: 93.140 de 327.680 bytes (28,4%);
- flash: 714.309 de 1.900.544 bytes (37,6%);
- firmware: `.pio/build/esp32_2432s028r/firmware.bin`;
- nenhuma chamada a `delay()` no código da aplicação;
- nenhuma chamada direta a SD, NVS, Wi-Fi ou GPIO dentro de `src/ui`;
- nenhuma alocação C++ explícita (`new`, `malloc` ou `String`) na camada de UI;
- compilação final sem erros ou warnings.

## Arquivos criados

### Framework de interface

- `src/ui/UIManager.h/.cpp`
- `src/ui/ScreenManager.h/.cpp`
- `src/ui/Theme.h/.cpp`
- `src/ui/Routes.h/.cpp`
- `src/ui/DemoData.h`

### Telas iniciais

- `src/ui/screens/HomeScreen.h/.cpp`
- `src/ui/screens/KegListScreen.h/.cpp`
- `src/ui/screens/TemperatureScreen.h/.cpp`
- `src/ui/screens/SettingsScreen.h/.cpp`

### Widgets básicos

- `src/ui/widgets/Card.h/.cpp`
- `src/ui/widgets/Header.h/.cpp`
- `src/ui/widgets/StatusBar.h/.cpp`
- `src/ui/widgets/NavigationBar.h/.cpp`

## Arquivos alterados

- `src/app/AppController.h/.cpp`: passa a iniciar e atualizar o `UIManager`.
- `include/BuildConfig.h`: adiciona a configuração de modo demonstração.
- `include/lv_conf.h`: documenta e habilita explicitamente os widgets utilizados.
- `platformio.ini`: habilita `KEEZER_DEMO_MODE` nesta etapa.
- `README.md`: descreve compilação, teste e limitações da Fase 2.
- `src/bsp/CydDisplay.h/.cpp`, `src/bsp/BoardConfig.h` e `include/BuildConfig.h`: corrigem o perfil após a identificação física da revisão de dois USB, usando ST7789, orientação própria do XPT2046 e nova versão da calibração.
- `src/bsp/CydDisplay.cpp`: habilita a inversão nativa exigida por esta revisão do ST7789, corrigindo a aparência de negativo observada no teste físico.
- `src/ui/Layout.h`, shell, widgets e quatro telas: adotam layout vertical 240 × 320 com navegação inferior de quatro áreas de 60 × 44 pixels.

## Arquivos removidos

- `src/ui/Phase1DiagnosticUi.h/.cpp`: substituídos pelo framework modular da Fase 2. O diagnóstico essencial de SD e heap permanece na barra de status.

## Arquitetura implementada

O `UIManager` é o único ponto de inicialização do LVGL e mantém os buffers parciais, a ponte de display/touch e o shell visual persistente. Esse shell contém cabeçalho, área de conteúdo, barra de status e navegação inferior.

O `ScreenManager` recebe pedidos da `NavigationBar`, atualiza a rota ativa e recria somente a área de conteúdo. As quatro telas não conhecem o BSP nem umas às outras. `Theme` centraliza a identidade visual e os widgets encapsulam os padrões reutilizáveis.

Os dados simulados estão concentrados em `DemoData.h`. Eles não representam ainda um modelo de domínio e serão substituídos pelo `AppViewModel` na Fase 4, conforme o planejamento.

## Navegação disponível

- HOME: resumo simulado do freezer e do barril ativo.
- BARRIS: German Pilsner, West Coast IPA e Vienna Lager simulados.
- TEMPERATURA: temperatura atual, setpoint e estado `COOLING` simulados.
- CONFIGURAÇÕES: categorias iniciais de barris, balança, temperatura, rede, display e sistema.

## Como testar na placa

1. Compile com `platformio run -e esp32_2432s028r`.
2. Grave com `platformio run -e esp32_2432s028r -t upload`.
3. Abra o monitor com `platformio device monitor -e esp32_2432s028r`.
4. Se solicitado, conclua a calibração do touch.
5. Toque em cada item da navegação inferior.
6. Confirme que título, destaque da rota e conteúdo mudam juntos.
7. Confirme `SD OK` e heap visível na barra de status.
8. Alterne as quatro rotas continuamente por 10 minutos e observe travamentos, toques duplicados ou queda contínua de heap.

## Limitações atuais

- O visual da HOME é apenas estrutural; a composição final pertence à Fase 3.
- Ainda não existe `AppViewModel`; a UI usa exclusivamente dados simulados isolados.
- As categorias de CONFIGURAÇÕES ainda não possuem subtelas.
- As telas não editam valores e não armazenam dados.
- Não existem ainda modelo real de barris, comunicação da balança, NFC, controle térmico ou histórico.
- Fluidez, cores, rotação e precisão do touch precisam ser confirmadas na placa física.

## Marco conceitual

`phase-02-ui-framework`
