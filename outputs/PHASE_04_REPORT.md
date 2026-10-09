# Relatório de entrega — Fase 4

## Estado

A Fase 4 está implementada e compila para `esp32_2432s028r`. A interface não acessa mais os dados constantes de demonstração: ela consome exclusivamente o estado exposto pelo `AppViewModel`.

- RAM estática: 93.916 de 327.680 bytes (28,7%);
- flash: 811.937 de 1.900.544 bytes (42,7%);
- firmware: `.pio/build/esp32_2432s028r/firmware.bin`;
- tamanho: 812.304 bytes;
- SHA-256: `8F26BE564DF5856C6DE934B9113049A9ED60F4F72157E308ED0542CEECE5CCDD`;
- compilação: concluída sem erros e sem avisos do código da aplicação.

## Fluxo implementado

```text
DemoData
   ↓
DemoDataSource
   ↓
AppViewState + revision
   ↓
AppViewModel
   ↓
ScreenManager
   ↓
HOME / detalhe do KEG / tela KEEZER
```

`AppController` atualiza primeiro o ViewModel e depois a UI. `ScreenManager` verifica a revisão a cada 250 ms. Se a revisão não mudou, nenhum widget de dados é tocado. Quando muda, somente os textos, cores, barras e pontos do gráfico afetados são atualizados; o objeto da tela e sua posição de scroll são preservados.

## Demonstração automática

- temperatura: passo de 0,1 °C a cada 2 segundos;
- compressor: alternância simulada entre `COOLING` e `IDLE`;
- consumo: redução de 0,1 L a cada 5 segundos apenas no KEG presente;
- derivados atualizados: peso, percentual, SYNC e último ponto do gráfico;
- presença: ciclo de 80 segundos dividido entre KEG 1, nenhum, KEG 2 e nenhum;
- invariante: zero ou um KEG pode estar `LENDO`, nunca dois.

O autoplay pode ser congelado com `KEEZER_DEMO_AUTOPLAY=0`. `KEEZER_DEMO_MODE=0` deixa o ViewModel vazio e pronto para as fontes reais das fases posteriores.

## Arquivos criados

- `src/app/DemoDataSource.h`
- `src/app/DemoDataSource.cpp`

## Arquivos alterados

- `include/BuildConfig.h`
- `platformio.ini`
- `src/app/ViewData.h`
- `src/app/DemoData.h`
- `src/app/AppViewModel.h/.cpp`
- `src/app/AppController.cpp`
- `src/ui/ScreenManager.h/.cpp`
- `src/ui/UIManager.cpp`
- `src/ui/screens/HomeScreen.h/.cpp`
- `src/ui/screens/KegDetailScreen.h/.cpp`
- `src/ui/screens/TemperatureScreen.h/.cpp`
- `src/ui/widgets/FreezerCard.h/.cpp`
- `src/ui/widgets/KegCard.h/.cpp`
- `src/ui/widgets/InfoTile.h/.cpp`
- `src/ui/widgets/ConsumptionChart.h/.cpp`
- `README.md`

## Verificações

- nenhum acesso a `DemoData` ou `DemoDataSource` dentro de `src/ui`;
- nenhum `delay()`, `String`, `new` ou `malloc` adicionado à aplicação;
- atualização cooperativa, sem task FreeRTOS adicional;
- três telas vinculadas ao mesmo estado do ViewModel;
- compilação de produção concluída.

## Como testar na placa

1. Grave com `platformio run -e esp32_2432s028r -t upload`.
2. Na HOME, acompanhe temperatura e volume por pelo menos 80 segundos.
3. Confirme a sequência `KEG 1 LENDO → nenhum → KEG 2 LENDO → nenhum`.
4. Abra os detalhes durante o consumo e confira peso, volume, percentual, SYNC e gráfico.
5. Role os detalhes e verifique que a atualização não retorna a tela ao topo.
6. Abra a tela KEEZER e confirme temperatura e compressor dinâmicos.
7. Observe heap e métricas LVGL no monitor serial por dez minutos.

## Limitações

- não existe relógio de tempo real nesta fase; as datas avançam em tempo simulado;
- o ciclo térmico é apenas uma fonte de dados visual, não é controle de relé;
- o consumo é acelerado para facilitar o teste;
- não há persistência após reiniciar;
- nenhuma leitura vem da balança, NFC ou sensor físico.

A próxima etapa é a Fase 5: modelos de domínio `Keg`, `KegStatus`, `KegService` e `KegRepository`, com cadastro e armazenamento local.
