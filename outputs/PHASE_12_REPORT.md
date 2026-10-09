# Relatório da Fase 12 — Controle de temperatura

## Resultado

O firmware agora possui um domínio térmico independente da UI, do sensor físico e do GPIO. O `TemperatureService` valida as amostras e controla falhas; o `CompressorController` aplica histerese e proteção anti-ciclo; o `NullCompressorOutput` permite observar todo o comando lógico sem energizar o relé.

## Componentes criados

- `models/Temperature.h/.cpp`: amostra, estado, qualidade, falha e estados de controle;
- `temperature/ITemperatureSensor.h`: contrato do sensor;
- `temperature/ICompressorOutput.h`: contrato da saída segura;
- `temperature/NullCompressorOutput.h`: saída lógica sem acesso a GPIO;
- `temperature/DemoTemperatureSensor.h/.cpp`: ciclo térmico e falha determinísticos;
- `temperature/CompressorController.h/.cpp`: histerese e relógios de proteção;
- `services/TemperatureService.h/.cpp`: validação, timeout, recuperação e eventos;
- `test/temperature_state_machine_smoke.cpp`: máquina do compressor;
- `test/temperature_service_smoke.cpp`: falha, timeout, faixa e recuperação.

## Integração

- `AppController` atualiza o serviço térmico cooperativamente antes dos demais serviços;
- `AppViewModel` converte centésimos de grau apenas para apresentação;
- HOME e a tela térmica básica recebem temperatura e estado reais do serviço simulado;
- nenhum `delay()` foi adicionado;
- o BSP continua forçando o GPIO candidato do relé para OFF no boot;
- a saída usada pela máquina de estados é `NullCompressorOutput`, portanto `COOLING` não aciona hardware.

## Parâmetros

Produção:

- setpoint: 2,00 °C;
- histerese: 1,00 °C;
- ligar em 2,50 °C e desligar em 1,50 °C;
- mínimo desligado: 180 s;
- mínimo ligado: 60 s;
- timeout: 10 s;
- faixa válida: −20,00 a +50,00 °C;
- recuperação: três amostras válidas consecutivas.

Simulador de bancada:

- mínimo desligado: 10 s;
- mínimo ligado: 5 s;
- amostra a cada 1 s;
- ciclo completo: 60 s;
- erro de leitura intencional entre 13 e 17 s.

## Segurança

- boot sempre começa com compressor OFF;
- demanda durante min-off resulta em `WAITING`;
- temperatura baixa durante min-on aguarda o prazo antes de desligar;
- sensor inválido, desconectado, fora da faixa ou em timeout força `ERROR` e OFF imediato;
- falha crítica vence o tempo mínimo ligado;
- mudar setpoint nunca zera os relógios de proteção;
- falha da própria saída fica latched e não é recuperada por amostras do sensor.

## Validação de código

Os módulos e testes térmicos foram compilados isoladamente com o toolchain Xtensa, C++17, `-Wall` e `-Wextra`, sem erros. O firmware completo também foi compilado com sucesso pelo PlatformIO para `esp32_2432s028r`.

- RAM: 124.236 bytes de 327.680 (37,9%);
- flash: 906.901 bytes de 1.900.544 (47,7%);
- SHA-256 de `firmware.bin`: `3E14D6E8FAB2CDB99E12CB4B1E8326D91D8DD185DF6D9A36FAA52CDE4C5801C4`.

## Limites atuais

- o DS18B20 físico ainda não está conectado;
- o GPIO 22 continua apenas candidato para o barramento 1-Wire;
- o GPIO 27 e a polaridade do relé precisam de validação elétrica antes do acionamento real;
- a configuração ainda não é editável/persistente pela tela; isso será desenvolvido na Fase 13;
- o controlador não substitui proteções elétricas adequadas para carga indutiva e tensão de rede.
