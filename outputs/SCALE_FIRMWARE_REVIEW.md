# Análise do firmware atual da balança

> Nota de atualização — Fase 15: este relatório registra a análise do firmware
> anterior. A balança já foi rearquitetada e agora entrega um snapshot atômico
> por `GET /api/v1/reading`, conforme `SCALE_PROTOCOL.md`. A recomendação de
> HTTP push abaixo foi superada pela implementação conjunta aprovada.

## Escopo

Análise somente de leitura do projeto `C:\Users\Jose\Documents\Control de Barril\Balanca`. Nenhum arquivo do firmware da balança foi alterado.

## Hardware e funções encontradas

- Wemos D1 mini com ESP8266;
- HX711 em D5/D6;
- PN532 por I2C em D1/D2, com IRQ em D7 e reset em D0;
- OLED SSD1306 opcional no mesmo I2C;
- portal cativo para configurar Wi-Fi;
- calibração e tara persistidas em EEPROM;
- página local de diagnóstico e calibração;
- publicação MQTT de peso, NFC, estado e RSSI.

A escolha dos pinos evita D3, D4 e D8, que são sensíveis durante o boot do ESP8266. O compartilhamento I2C entre OLED e PN532 está corretamente separado por endereço.

## Pontos positivos

- aquisição do HX711 não usa uma sequência longa de leituras bloqueantes;
- calibração possui validação e persiste fator/offset;
- estabilidade é calculada sobre uma janela de cinco amostras;
- existe zero tracking apenas para apresentação, sem alterar a tara salva;
- Wi-Fi e MQTT possuem reconexão;
- a balança continua medindo quando a comunicação falha;
- PN532 e OLED são detectados antes do uso;
- portal de configuração e diagnóstico facilitam instalação e manutenção.

## Incompatibilidades com o controlador

O MQTT atual publica quatro tópicos independentes:

```text
controlbarril/balanca1/peso
controlbarril/balanca1/nfc
controlbarril/balanca1/status
controlbarril/balanca1/rssi
```

Esse formato não forma uma fotografia atômica da medição. Peso e UID podem chegar em instantes diferentes, ser repetidos separadamente ou permanecer retidos de ciclos anteriores. Isso abre risco de associar peso novo a UID antigo.

Também faltam no envio atual:

- versão do protocolo;
- sequência monotônica por pacote;
- uptime do emissor;
- indicação de estabilidade junto do peso;
- ausência de NFC representada de forma inequívoca;
- confirmação de recebimento da medição completa.

O identificador atual `balanca1` não coincide com o `SCALE_MAIN` aceito pelo controlador. A balança admite até 300 kg, enquanto o limite inicial do controlador é 100 kg. Esses valores precisam ser harmonizados.

## Pontos a corrigir no firmware da balança

1. O estado MQTT vira `error` quando o PN532 está ausente. No modo híbrido sem NFC, a ausência do PN532 deve ser informativa, não uma falha da balança.
2. Um único timeout de 20 ms do HX711 marca o periférico como indisponível. Em módulos de 10 SPS esse limite é apertado; deve existir tolerância a atrasos consecutivos antes de declarar falha.
3. Após recuperar o HX711, a janela antiga de amostras deve ser descartada.
4. A média móvel da balança somada à mediana do controlador cria filtragem dupla e maior atraso. A balança deve enviar a amostra calibrada mais recente e o indicador de estabilidade; o controlador mantém mediana/deadband.
5. O endpoint e o MQTT apresentam peso com duas casas em kg. O protocolo deve enviar inteiro em gramas para evitar ambiguidade e conversão de ponto flutuante.
6. O uso de `String` para páginas grandes e JSON merece ser reduzido nos caminhos contínuos de comunicação do ESP8266, evitando fragmentação após longos períodos.

## Comparação de transportes

### MQTT atual

Exige broker externo e precisaria migrar para um único payload versionado. É útil se houver servidor de automação, mas adiciona uma dependência desnecessária ao funcionamento básico do keezer.

### HTTP consultado pelo controlador

O controlador poderia consultar `/api/status`, porém dependeria do IP da balança, teria atraso de polling e o endpoint atual não possui sequência nem autenticação. É uma adaptação rápida, mas não é a melhor solução definitiva.

### HTTP enviado pela balança

A balança envia um único `POST` com todos os campos ao controlador. Permite validação, sequência, confirmação e diagnóstico sem broker. Aproveita o Wi-Fi e o portal já existentes. É a opção recomendada para a arquitetura atual.

### ESP-NOW

Funciona entre ESP8266 e ESP32 sem roteador e pode ter baixa latência. Entretanto, exige pareamento, controle de canal e convivência cuidadosa com o Wi-Fi que será usado pela interface/rede do controlador. É uma boa alternativa futura para operação sem infraestrutura, não a primeira escolha para este firmware.

## Recomendação

Manter o Wi-Fi existente e substituir o MQTT como caminho principal por **HTTP push local**. MQTT pode permanecer opcional para telemetria externa, mas não deve determinar a pesagem do controlador.

Payload recomendado:

```json
{
  "version": 1,
  "scaleId": "SCALE_MAIN",
  "sequence": 1842,
  "nfcUid": "04A23F891C",
  "weightGrams": 18542,
  "stable": true,
  "rssi": -58,
  "uptimeMs": 12345678,
  "pn532Available": false
}
```

Sem tag ou sem PN532, `nfcUid` deve ser `null`. A capacidade de NFC é informada separadamente para que o modo manual continue saudável.

## Plano proposto para a Fase 15

1. Formalizar `outputs/SCALE_PROTOCOL.md`.
2. Criar no controlador um transporte HTTP que implemente `IScaleTransport`.
3. Receber e validar um pacote inteiro antes de entregá-lo ao `ScaleService`.
4. Aplicar limite de corpo, token local, sequência, timeout e respostas HTTP claras.
5. Adaptar o firmware da balança para enviar o pacote completo e aguardar ACK curto.
6. Manter fila curta/repetição segura quando o controlador estiver indisponível.
7. Permitir funcionamento com `nfcUid=null`.
8. Testar troca de KEGs, duplicidade, reinício da balança, perda de Wi-Fi e recuperação.

## Conclusão

O firmware é uma boa base de aquisição, calibração e manutenção. O ponto que precisa mudar para a Fase 15 é o contrato de comunicação: os tópicos MQTT separados não oferecem a atomicidade e a idempotência exigidas para associar peso e KEG com segurança.
