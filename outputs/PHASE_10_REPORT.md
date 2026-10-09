# Relatório da Fase 10 — Cálculo de volume

## Resultado

A Fase 10 centraliza o cálculo no `KegCalculator` e passa a gravar uma medição completa quando um NFC conhecido é estabilizado. O peso fornecido pela balança continua neutro; tara, densidade e capacidade pertencem ao KEG e somente são combinadas no domínio do controlador.

## Operações implementadas

- `validateWeight()`: aceita de 0 a 100.000 g;
- `validateConfiguration()`: tara de 0 a menos de 100 kg, capacidade de 1 a 100 L e densidade de 0,900 a 1,300 kg/L;
- `calculateBeerWeight()`: subtrai a tara e limita o resultado inferior a zero;
- `calculateVolume()`: divide o peso líquido pela densidade usando aritmética inteira de 64 bits e limita à capacidade;
- `calculatePercentage()`: calcula centésimos de percentual e limita a 100,00%;
- `calculate()`: devolve peso bruto, peso líquido, volume bruto, volume limitado, percentual e validade.

## Validades

- `VALID`: medição dentro da faixa esperada;
- `BELOW_TARE`: volume e percentual iguais a zero;
- `ABOVE_EXPECTED_MAXIMUM`: volume limitado à capacidade, percentual limitado a 100%, peso bruto preservado e aviso no serial;
- `INVALID_WEIGHT`: leitura negativa ou acima do limite físico; não grava;
- `INVALID_KEG_CONFIGURATION`: tara, densidade ou capacidade inválida; não grava.

O limite superior esperado é `tara + capacidade × densidade + 500 g`. A tolerância evita classificar pequenas diferenças de envase como erro.

## Integração

Na identificação NFC, o `AppController` aguarda UID estabilizado, conexão online e peso estável. Em seguida chama `recordMeasurement()` uma única vez para o KEG encontrado. Essa operação calcula, atualiza peso/volume, realiza a troca atômica do ativo e persiste o catálogo. UID desconhecido, leitura ausente, instável ou inválida não recebe atribuição.

A pesagem manual continua mostrando o resultado antes da gravação, com avisos para peso abaixo da tara, acima do esperado ou conflito NFC.

Os avisos `PESO ABAIXO DA TARA` e `PESO ACIMA DO ESPERADO` são destacados em laranja. Um conflito entre o KEG escolhido e o UID NFC detectado é destacado em vermelho.

## Testes e compilação

O teste `keg_calculator_smoke.cpp` cobre:

- peso líquido;
- densidade diferente de 1,000 kg/L;
- volume e percentual;
- percentual fracionário em centésimos (por exemplo, 31,50%);
- peso abaixo da tara;
- peso negativo e acima de 100 kg;
- volume acima da capacidade;
- densidade e capacidade inválidas.

Os fontes dos testes de cálculo e do `KegService` foram compilados sem erros com o toolchain do ESP32. A compilação PlatformIO do ambiente `esp32_2432s028r` foi concluída com sucesso:

- RAM: 124.364 de 327.680 bytes (38,0%);
- flash: 901.737 de 1.900.544 bytes (47,4%);
- SHA-256 do firmware: `C286B2A63F01E5864CE011A0FA534FADF2F38919F56E0150D46167C1F8AFF3BB`.

## Pendente de validação física

Gravar o firmware e confirmar os valores do simulador na HOME, na tela de detalhes e no serial. A estabilidade/deadband entre leituras sucessivas pertence à Fase 11.
