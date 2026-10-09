# Relatório da Fase 11 — Filtro de peso

## Resultado

A leitura publicável da balança agora passa por uma janela de mediana de cinco amostras estáveis e por um deadband de 20 g. O peso bruto continua preservado para diagnóstico. Mudanças superiores a 2.000 g em saídas filtradas geram `SIGNIFICANT_WEIGHT_CHANGE`, sem executar nenhuma ação destrutiva ou troca de KEG.

## Implementação

- `scale/WeightFilter.h/.cpp`: filtro determinístico com memória fixa para até nove amostras;
- `ScaleService`: envia somente amostras estáveis ao filtro e publica o resultado filtrado;
- `ScaleState`: expõe quantidade de amostras, atualizações publicadas e revisão da última mudança significativa;
- `BuildConfig.h`: concentra janela, deadband e limiar configuráveis;
- `SimulatedScaleTransport`: injeta oscilação pequena e um pico isolado de 1,5 kg em cada cenário;
- `test/weight_filter_smoke.cpp`: cobre mediana, pico isolado, deadband e limiar significativo;
- `test/scale_service_smoke.cpp`: confirma que leitura instável atualiza o peso bruto, mas não o filtrado.

## Regras

1. Todo pacote válido atualiza `rawWeightGrams`.
2. Pacotes instáveis não entram na janela nem substituem o peso publicável.
3. A mediana começa com as amostras disponíveis e atinge a janela completa em cinco leituras.
4. Diferença menor que 20 g mantém a saída anterior.
5. Diferença exatamente igual a 20 g pode ser publicada.
6. Mudança exatamente igual a 2.000 g não é significativa; somente valor superior gera evento.
7. O evento não altera cadastro, presença, histórico nem vínculo NFC.

## Logs esperados

```text
Scale service started: source=READY state=OFFLINE filter=MEDIAN/5 deadband=20g significant=2000g
WEIGHT_FILTER raw=20050g filtered=18550g samples=5 published=NO
SIGNIFICANT_WEIGHT_CHANGE from=18550g to=10650g delta=-7900g
```

## Validação

Os fontes do novo filtro, seu teste e o teste do `ScaleService` foram compilados isoladamente com o toolchain Xtensa e `-Wall -Wextra`, sem erros. A compilação PlatformIO completa do ambiente `esp32_2432s028r` também foi concluída com sucesso:

- RAM: 124.460 de 327.680 bytes (38,0%);
- flash: 902.889 de 1.900.544 bytes (47,5%);
- firmware: 903.248 bytes;
- SHA-256: `93963BD6DE2F864A7BB1E4230FBC26ABD64D39C971DBE119BE241F8945522BC7`.

## Teste recomendado na placa

1. Grave o firmware e abra o monitor serial em 115200 baud.
2. Confirme `Phase 11 initialized` e os parâmetros `MEDIAN/5`, `20g` e `2000g`.
3. Espere pelo pico simulado: o log `WEIGHT_FILTER` deve mostrar diferença grande entre `raw` e `filtered`, com `published=NO`.
4. Na troca de cenário, confirme um `SIGNIFICANT_WEIGHT_CHANGE` e depois o cálculo do KEG correto.
5. Confira que HOME, detalhe e `PESAR` usam o peso filtrado e permanecem estáveis.
6. Confirme que a troca/retirada híbrida e os estados `ONLINE`, `STALE` e `OFFLINE` continuam funcionando.

## Limites atuais

- a entrada ainda vem do simulador; o transporte físico pertence à Fase 15;
- os valores de cinco amostras, 20 g e 2.000 g são iniciais e deverão ser confirmados com a célula de carga real;
- o filtro não substitui calibração correta da balança;
- histórico persistente pertence à Fase 14.
