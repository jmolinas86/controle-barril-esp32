# Compilação, gravação e testes

## Pré-requisitos

- VS Code com PlatformIO ou PlatformIO Core disponível no terminal;
- cabo USB de dados conectado à CYD;
- ambiente `esp32_2432s028` do `platformio.ini`;
- balança e controlador na mesma rede para os testes de integração;
- microSD FAT32 para histórico.

## Compilar

Na raiz do projeto:

```powershell
platformio run -e esp32_2432s028
```

Última compilação validada antes da documentação:

- RAM estática: 123.968 bytes (37,8%);
- flash: 1.521.773 bytes (80,1%);
- `firmware.bin`: 1.528.352 bytes;
- SHA-256: `B8A2720260258C52CA283DA819B2074367D45DC190D4AE56342B177F324A4C29`.

## Gravar e monitorar

```powershell
platformio run -e esp32_2432s028 -t upload
platformio device monitor -e esp32_2432s028
```

Se a porta não for detectada, confira o cabo, driver USB e feche outros
monitores seriais. O monitor opera a 115.200 baud.

## Testar a OTA da Fase 19.1

1. faça a primeira gravação pelo USB;
2. confirme no serial `Web OTA ready` e `Phase 19.1 initialized`;
3. consulte o IP em `CONFIG. > REDE`;
4. abra `http://<IP>/update` e confirme que exige autenticação;
5. teste uma senha errada e confirme acesso negado;
6. compile novamente e envie `.pio/build/esp32_2432s028/firmware.bin`;
7. confira progresso até 100%, reinício e retorno da HOME;
8. confirme no serial `OTA_SUCCESS`, `OTA_RESTART` e novo boot;
9. verifique que KEGs, rede, setpoint e histórico foram preservados;
10. opcionalmente interrompa um upload e confirme que a versão anterior volta
    à operação sem apagar dados.

Credenciais iniciais de bancada: usuário `admin`, senha `keezer-update`. Troque
`KEEZER_OTA_PASSWORD` em `include/NetworkConfig.h` antes do uso definitivo.

## Smoke tests independentes de hardware

Os fontes em `test/` cobrem:

- `keg_calculator_smoke.cpp`: peso de cerveja, volume, percentual e validade;
- `keg_service_smoke.cpp`: CRUD, NFC, estados e persistência;
- `weight_filter_smoke.cpp`: mediana, deadband e salto significativo;
- `scale_service_smoke.cpp`: protocolo, duplicidade e reinício do emissor;
- `scale_http_json_smoke.cpp`: contrato JSON HTTP;
- `keg_presence_service_smoke.cpp`: troca, retirada e incerteza;
- `keg_tracking_service_smoke.cpp`: acompanhamento sem NFC;
- `history_service_smoke.cpp`: fila, deduplicação e recuperação;
- `temperature_state_machine_smoke.cpp`: histerese e anti-ciclo;
- `temperature_service_smoke.cpp`: falha, timeout e recuperação;
- `network_service_smoke.cpp`: conexão, timeout e backoff.

Eles são programas C++ nativos com `assert`. Este computador não possui um
compilador C++ nativo configurado, portanto a validação corrente é a compilação
PlatformIO mais os testes aprovados no hardware. Em CI ou máquina com `g++`,
compile cada teste com os respectivos `.cpp` de produção e `-std=c++17`.

## Aceitação completa no hardware

### Boot e interface

1. confirme geometria 240 × 320, cores normais e touch calibrado;
2. navegue HOME, detalhe, edição, KEEZER, CONFIG. e REDE;
3. role continuamente e confirme ausência de faixas pretas;
4. aceite apenas tearing residual mínimo, conforme referência aprovada;
5. acompanhe três blocos `PERF_*` sem queda contínua de heap.

### Catálogo

1. crie um KEG com NFC vazio e outro com UID;
2. reinicie e confirme que ambos voltam;
3. edite nome, tara, capacidade e densidade;
4. tente ID/NFC duplicados e confira a rejeição;
5. arquive e confirme que some da HOME sem perder o registro;
6. finalize um KEG de teste, aceite o aviso e confirme exclusão permanente.

### Balança híbrida

1. confirme `Scale resolved` e `ONLINE`;
2. compare peso real e peso exibido;
3. com NFC, teste UID conhecido, desconhecido e troca A → B;
4. sem NFC, use `PESAR` em dois KEGs diferentes;
5. retire ao menos 200 g e confirme gravação automática após 3 s estáveis;
6. provoque aumento de 300 g e salto de 2 kg para ver a confirmação;
7. desligue a balança e confirme que o KEG não troca nem é apagado;
8. religue e confirme recuperação sem reiniciar o controlador.

### Cálculo

Para um KEG com tara 4,350 kg, densidade 1,000 kg/L e capacidade 20 L:

- 18,550 kg bruto deve resultar em 14,200 L e 71,00%;
- peso igual/abaixo da tara deve resultar em 0 L com `BELOW_TARE`;
- volume acima da capacidade deve limitar a 20 L e avisar excesso;
- peso negativo ou acima de 100 kg deve ser rejeitado.

### Histórico e falhas

1. com SD presente, confirme `SD OK`, `HISTORY_FLUSHED` e gráfico atualizado;
2. retire o SD, faça medições e confira `FILA N`;
3. recoloque e aguarde a recuperação automática a cada 15 s;
4. copie os CSVs e valide que cada linha termina em CRC32;
5. desligue Wi-Fi por mais de 20 s e confirme UI/temperatura funcionando;
6. religue e confirme backoff/reconexão automática.

### Temperatura e relé real

1. mantenha a carga de potência desconectada no primeiro teste;
2. confirme leitura real do DS18B20 e estado inicial `WAITING` ou `IDLE`;
3. meça GPIO 27 em LOW no boot, erro, sensor ausente e durante OTA;
4. provoque demanda e confirme HIGH somente após os 180 s de proteção;
5. desconecte o sensor e confirme OFF imediatamente;
6. reconecte-o e confirme recuperação após três amostras válidas;
7. altere/salve o setpoint e reinicie sem ignorar a proteção anti-ciclo;
8. somente após esses testes conecte relé/contator e carga dimensionados.

## Critério de liberação

A versão pode ser liberada para monitoramento e gestão de KEGs quando todos os
itens anteriores passarem, não houver reset inesperado, a fila do SD voltar a
zero e as leituras da balança coincidirem com um peso de referência. Controle
elétrico real do compressor permanece bloqueado até implementação dedicada.
