# Controle de temperatura

## Estado atual

O controle usa um DS18B20 real no GPIO 22 e um módulo de relé ativo em HIGH no
GPIO 27. A leitura 1-Wire é assíncrona, valida o CRC do scratchpad e considera
ausência, erro ou timeout como falha crítica, desligando o relé imediatamente.

## Parâmetros

| Parâmetro | Produção | Simulação atual |
|---|---:|---:|
| setpoint inicial | 2,00 °C | 2,00 °C |
| histerese total | 1,00 °C | 1,00 °C |
| mínimo desligado | 180 s | somente build demo: 10 s |
| mínimo ligado | 60 s | somente build demo: 5 s |
| timeout do sensor | 10 s | 10 s |
| faixa válida do sensor | −20 a 50 °C | −20 a 50 °C |
| amostras para recuperar | 3 | 3 |

Com setpoint 2,00 °C, a demanda começa em 2,50 °C e termina em 1,50 °C. A
histerese é dividida igualmente acima e abaixo do setpoint.

## Estados

- `IDLE`: sem demanda e compressor desligado;
- `WAITING`: existe demanda, mas o tempo mínimo desligado protege o compressor;
- `COOLING`: saída lógica ligada;
- `ERROR`: sensor/saída inválidos e comando forçado para OFF.

Ao atingir o limite inferior, o controlador respeita o tempo mínimo ligado
antes de parar. Uma falha crítica do sensor ignora essa espera e desliga
imediatamente. Após a falha, são exigidas três amostras válidas consecutivas.

## Configuração pela tela

1. abra `KEEZER` no menu inferior;
2. use `−` ou `+` em passos de 0,1 °C;
3. confirme `SALVAR SETPOINT`;
4. verifique a mensagem de confirmação;
5. reinicie e confirme que o valor foi restaurado da NVS.

O limite configurável é −5,0 a 15,0 °C. Alterar o setpoint não contorna as
proteções de tempo.

## Diagnóstico serial

- `TEMPERATURE_UPDATED`: amostra, estado e saída;
- `COOLING_STARTED` / `COOLING_STOPPED`: transição lógica;
- `Temperature control state=WAITING`: proteção em andamento;
- `TEMPERATURE_SENSOR_ERROR ... output=OFF`: falha segura;
- `Temperature sensor recovered`: três amostras válidas confirmadas.

## Validação obrigatória antes de conectar o compressor

1. energizar inicialmente apenas o ESP32 e o módulo de relé, sem a carga;
2. confirmar DS18B20 e resistor pull-up de 4,7 kΩ no GPIO 22;
3. medir o GPIO 27: LOW no boot/falha e HIGH somente em `COOLING`;
4. desconectar o sensor e confirmar relé OFF imediatamente;
5. testar boot, brownout, watchdog e OTA com a saída em estado seguro;
6. usar relé/contator dimensionado, isolamento e proteção elétrica;
7. somente depois conectar a carga do compressor.
