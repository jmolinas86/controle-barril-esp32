# Control de Barril — firmware da balança

Firmware HTTP para Wemos D1 mini (ESP8266). A balança mede via HX711, identifica o barril pelo PN532, exibe diagnóstico em OLED e funciona de forma autônoma pelo navegador. O controlador ESP32 consulta a leitura pela API HTTP; MQTT não é necessário.

## Primeiro acesso

Sem credenciais salvas, o módulo cria a rede `BC Balanca`. Conecte-se a ela e abra `http://192.168.4.1/`. O portal também tenta abrir automaticamente.

Escolha a rede Wi-Fi, informe a senha e salve. Depois da conexão:

- a OLED e o Serial mostram o endereço IP;
- a interface responde em `http://balanca.local/`;
- o portal temporário é desligado;
- se a rede configurada ficar inacessível, o portal volta após 20 segundos sem apagar a configuração.

As credenciais, tara e fator de calibração são gravados em um bloco versionado com checksum. Instalações do firmware anterior têm os dados antigos lidos e migrados na próxima gravação.

## Interface web

A página embarcada funciona sem internet ou arquivos adicionais e apresenta:

- peso com duas casas decimais e indicação de estabilidade;
- UID NFC;
- estado do HX711, PN532, OLED e Wi-Fi;
- tara e calibração guiada;
- busca e troca da rede Wi-Fi;
- reinicialização e restauração de fábrica;
- memória livre, uptime, IP e fator de calibração.

## API HTTP v1

O controlador consulta `GET /api/v1/reading` a cada 500 ms:

```json
{
  "protocol_version": 1,
  "sequence": 1842,
  "scale_id": "SCALE_MAIN",
  "weight_grams": 18540,
  "stable": true,
  "nfc_uid": "04A23F891C",
  "rssi_dbm": -58,
  "uptime_ms": 928441
}
```

Sem cartão, `nfc_uid` é `null`. Enquanto não houver uma leitura válida do HX711, o endpoint retorna HTTP 503.

| Método e rota | Uso |
|---|---|
| `GET /api/v1/reading` | Leitura compacta para o ESP32 |
| `GET /api/v1/status` | Diagnóstico completo para a interface |
| `GET /api/v1/wifi/scan` | Busca redes sob demanda |
| `POST /api/v1/config/wifi` | Salva `ssid` e `password` |
| `POST /api/v1/actions/tare` | Executa e salva a tara |
| `POST /api/v1/actions/calibrate` | Recebe `reference_grams` |
| `POST /api/v1/actions/restart` | Reinicia o módulo |
| `POST /api/v1/actions/factory-reset` | Apaga Wi-Fi e calibração |
| `GET /update` | Interface autenticada de atualização OTA |
| `POST /update` | Recebe exclusivamente o firmware da aplicação (`.bin`) |

Os endpoints antigos `/api/status`, `/wifi/save`, `/calibration/tare`, `/calibration/known-weight` e `/factory-reset` permanecem como compatibilidade local.

## Processamento e resposta

O HX711 é consultado continuamente por `is_ready()`. Uma conversão ainda não pronta não é tratada como defeito. O firmware:

- coleta uma amostra por conversão disponível;
- mantém janela de cinco amostras;
- calcula peso local e estabilidade;
- aplica zona morta de 10 g e zero até 50 g;
- incrementa `sequence` a cada amostra válida;
- marca o HX711 indisponível somente após 1,5 s sem dados.

Não existem `delay()` no ciclo normal. A busca Wi-Fi e a tara são operações explicitamente solicitadas e podem bloquear brevemente.

## Estrutura

```text
include/
  BoardConfig.h                 pinagem
  config.h                      limites e identidade
src/
  main.cpp                      entrada mínima
  app/ScaleApplication.*        composição e ciclo cooperativo
  drivers/ScaleHardware.*       HX711, PN532 e OLED
  model/ScaleSnapshot.h         snapshot fixo da leitura
  network/NetworkManager.*      STA, fallback AP e mDNS
  storage/SettingsStore.*       EEPROM versionada e migração
  web/WebPortal.*               rotas HTTP
  web/WebAssets.h               interface armazenada na flash
```

## Pinagem

| Wemos D1 mini | GPIO | Módulo | Sinal |
|---|---:|---|---|
| D1 | GPIO5 | OLED + PN532 | SCL I2C |
| D2 | GPIO4 | OLED + PN532 | SDA I2C |
| D5 | GPIO14 | HX711 | SCK |
| D6 | GPIO12 | HX711 | DT/DOUT |
| D7 | GPIO13 | PN532 | IRQ |
| D0 | GPIO16 | PN532 | RST/RSTPDN |
| 3V3 | — | Todos | VCC compatível com 3,3 V |
| GND | — | Todos | GND comum |

O PN532 deve estar selecionado fisicamente para I2C. D3, D4 e D8 permanecem livres para não afetar o boot.

## Compilar, gravar e monitorar

```powershell
platformio run
platformio run -t upload
platformio device monitor -b 115200
```

Build validado com OTA: 32.240 bytes de RAM estática (39,4%) e 376.811 bytes de flash (36,1%).

## Atualização OTA

A primeira instalação deste firmware ainda deve ser feita pelo cabo USB. Depois dela:

1. Abra `http://balanca.local/update` ou `http://IP_DA_BALANCA/update`.
2. Entre com o usuário `admin` e senha `balanca-update`.
3. Selecione `.pio/build/d1_mini/firmware.bin`.
4. Aguarde a confirmação; a balança reinicia automaticamente após a gravação.

O atualizador aceita somente o binário da aplicação e rejeita imagens de sistema de arquivos. Wi-Fi, tara e fator de calibração permanecem preservados. Antes de usar o equipamento fora de uma rede confiável, altere `OTA_USERNAME` e `OTA_PASSWORD` em `include/config.h`, compile e instale a nova versão.

## Calibração

1. Abra a interface web.
2. Deixe a plataforma completamente vazia e pressione **Fazer tara**.
3. Coloque uma massa conhecida.
4. Aguarde o indicador **ESTÁVEL**.
5. Informe a massa em kg e pressione **Calibrar com peso conhecido**.
6. Confira com uma segunda massa.

O fator inicial continua em `HX711_CALIBRATION_FACTOR`, em `include/config.h`. O valor gravado pela interface prevalece nos boots seguintes.

## Observação de segurança

A atualização OTA exige autenticação, mas a tela de leitura e configuração continua destinada a uma rede local confiável e não possui autenticação. Não encaminhe a porta 80 da balança para a internet.
