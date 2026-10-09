# Relatório de entrega — Fase 2.5

## Estado

A implementação visual está concluída em código e compila para o ambiente `esp32_2432s028r`. A validação visual, de fluidez e de touch ainda deve ser feita na placa física.

- biblioteca gráfica: LVGL 9.5.0;
- driver de display/touch: LovyanGFX 1.2.25;
- resolução: 240 × 320 portrait;
- RAM estática: 93.316 de 327.680 bytes (28,5%);
- folga estática de RAM: 234.364 bytes antes das alocações de execução;
- flash: 808.073 de 1.900.544 bytes (42,5%);
- firmware: `.pio/build/esp32_2432s028r/firmware.bin`;
- tamanho do firmware: 808.432 bytes;
- SHA-256: `9D7C50EBF6BB2C3F1C44E2D302A9FE758078A246DC045BBDB67047B1BA289AD7`;
- heap em execução: registrado no monitor serial no boot e a cada 30 segundos;
- renderização: RGB565 parcial com dois buffers de 20 linhas;
- compilação: sem erros e sem warnings do código da aplicação.

## Arquivos criados

### ViewModel e dados

- `src/app/AppViewModel.h/.cpp`
- `src/app/ViewData.h`
- `src/app/DemoData.h`

### Tela

- `src/ui/screens/KegDetailScreen.h/.cpp`

### Widgets

- `src/ui/widgets/ActionButton.h/.cpp`
- `src/ui/widgets/BottomNavigation.h/.cpp`
- `src/ui/widgets/ConsumptionChart.h/.cpp`
- `src/ui/widgets/FreezerCard.h/.cpp`
- `src/ui/widgets/InfoTile.h/.cpp`
- `src/ui/widgets/KegCard.h/.cpp`
- `src/ui/widgets/KegIcon.h/.cpp`
- `src/ui/widgets/Modal.h/.cpp`
- `src/ui/widgets/ProgressBar.h/.cpp`
- `src/ui/widgets/ScrollContainer.h/.cpp`
- `src/ui/widgets/SnowflakeIcon.h/.cpp`
- `src/ui/assets/MiniBitmaps.h/.cpp`
- `assets/ui/header-snowflake-18.png`
- `assets/ui/keezer-temperature-36x54.png`
- `assets/ui/keg-36x51.png`
- `assets/ui/keg-56x76.png`
- `assets/ui/details-list-18x14.png`
- `assets/ui/freezer-nav-18x19.png`
- `assets/ui/measurement-tare-28x30.png`
- `assets/ui/measurement-density-28x30.png`
- `assets/ui/measurement-volume-28x30.png`
- `assets/ui/measurement-weight-28x30.png`
- `tools/generate_mini_bitmaps.py`

## Refinamento conforme a referência física

- inversão de cor do ST7789 desativada após comparação com a foto da placa;
- paleta substituída por preto/azul-marinho, ciano, branco, verde e âmbar;
- cabeçalho reduzido para 31 px e navegação inferior para 44 px;
- área central aumentada para 245 px;
- cartão do freezer reduzido para 64 px e cartões de barril para 86 px;
- espaçamentos internos e tipografia refeitos para mostrar os dois primeiros barris;
- floco do cabeçalho convertido em mini bitmap RGB565A8 de 18 × 18 px;
- termômetro e floco do keezer convertidos em mini bitmap RGB565A8 de 36 × 54 px;
- barril metálico convertido em mini bitmaps RGB565A8 de 36 × 51 px e 56 × 76 px;
- botão de detalhes reduzido a um mini bitmap RGB565A8 de lista com 18 × 14 px, sem legenda;
- bitmaps embutidos na flash com transparência, sem dependência do microSD;
- divisória do cabeçalho elevada em 5 px sem alterar a área de touch;
- cartão de temperatura do keezer fixado no topo da HOME;
- somente cartões de barril e botão de inclusão rolam abaixo do cartão fixo;
- coluna de cartões deslocada 4 px para a esquerda, preservando a largura de 228 px;
- ícone e legenda do menu inferior separados para reproduzir a hierarquia visual da referência.
- ícones de tara, densidade, volume atual e peso extraídos da referência visual, convertidos para mini bitmaps RGB565A8 de 28 × 30 px e aplicados aos respectivos cartões;
- títulos desses quatro cartões em Montserrat 12 ciano e medições em Montserrat 14 branca, com espaçamento ajustado à resolução 240 × 320.

## Arquivos modificados

- `src/app/AppController.h/.cpp`
- `src/ui/UIManager.h/.cpp`
- `src/ui/ScreenManager.h/.cpp`
- `src/ui/Routes.h/.cpp`
- `src/ui/Layout.h`
- `src/ui/Theme.h/.cpp`
- `src/ui/screens/HomeScreen.h/.cpp`
- `src/ui/screens/TemperatureScreen.h/.cpp`
- `src/ui/screens/SettingsScreen.h/.cpp`
- `src/ui/widgets/Header.h/.cpp`
- `include/BuildConfig.h`
- `include/lv_conf.h`
- `README.md`
- `outputs/ARCHITECTURE.md`

## Arquivos removidos

- `src/ui/DemoData.h`: substituído pelo fluxo `DemoData -> AppViewModel -> UI`.
- `src/ui/screens/KegListScreen.h/.cpp`: a lista agora faz parte da HOME.
- `src/ui/widgets/NavigationBar.h/.cpp`: substituído por `BottomNavigation` com três opções.
- `src/ui/widgets/StatusBar.h/.cpp`: removido da composição visual aprovada; heap permanece no log serial.

## Navegação

O shell mantém cabeçalho e menu inferior fixos. O menu abre HOME, TEMPERATURA e CONFIG. A HOME destaca a rota também durante o detalhe do barril. Cada botão `DETALHES` seleciona seu barril no `ScreenManager`; a seta do cabeçalho retorna à HOME. `NOVO BARRIL` abre apenas um modal simulado.

## Scroll

Na HOME, o cartão do freezer fica fixo no topo da região de conteúdo. Somente os três barris e o botão de inclusão rolam na coluna inferior. A tela de detalhe continua usando toda a região central rolável para acomodar hero, oito informações e gráfico sem reduzir excessivamente as fontes. O gesto é processado pelo input pointer do LVGL conectado ao XPT2046.

## Desempenho e memória

Não há animações, sombras, framebuffer completo ou bitmap grande. Gradientes simples ficam restritos aos botões destacados. Os ícones fotográficos são mini bitmaps RGB565A8 embutidos na flash. O gráfico usa o widget nativo do LVGL, sem dependência adicional. Não há alocação de aplicação dentro do loop; telas são recriadas somente quando a rota muda.

O valor real de heap depende das alocações feitas no boot e deve ser lido na placa pelo monitor serial. O firmware registra `Heap free` e `minimum` periodicamente para permitir o teste de estabilidade de dez minutos.

## Limitações visuais

- A referência possui uma proporção de tela maior; em 240 × 320 o conteúdo detalhado precisa de scroll.
- A fonte Montserrat disponível não contém um conjunto dedicado de pictogramas industriais; foram usados símbolos LVGL e formas leves.
- O barril metálico foi reduzido para dois mini bitmaps otimizados; detalhes menores que um pixel são necessariamente simplificados.
- Wi-Fi, horário, histórico e todos os valores são simulados.

## Como testar

1. Grave com `platformio run -e esp32_2432s028r -t upload`.
2. Abra `platformio device monitor -e esp32_2432s028r`.
3. Execute o roteiro descrito no `README.md`.
4. Compare as cores e o espaçamento com a imagem aprovada.
5. Registre o primeiro `Heap free`, navegue por dez minutos e compare com `minimum`.

Não foram implementados sensores, balança, NFC, controle térmico, rede ou persistência. A Fase 3 não foi iniciada.
