# Fase 17 — Robustez

## Resultado

A fase 17 consolida o comportamento fail-safe do controlador. Uma falha de
rede, balança, NFC, temperatura ou microSD não pode atribuir uma medição ao KEG
errado, apagar registros nem energizar o compressor em condição incerta.

## Proteções validadas no código

- `NetworkService`: timeout de conexão e reconexão exponencial sem bloquear o loop;
- `ScaleService`: estados `ONLINE/STALE/OFFLINE`, rejeição de pacotes inválidos,
  peso negativo, sequência antiga e UID malformado;
- `KegPresenceService` e `KegTrackingService`: comunicação perdida pausa a
  decisão e preserva o KEG ativo;
- `TemperatureService`: desconexão, erro, timeout e faixa impossível desligam a
  saída; a recuperação exige três amostras válidas;
- `KegCalculator` e `KegService`: tara/capacidade/densidade inválidas não são
  persistidas e uma medição inválida não modifica o catálogo;
- `LittleFsKegRepository`: snapshots alternados A/B, geração, CRC32, verificação
  após escrita e recuperação pelo slot íntegro;
- `NetworkSettingsStore`: configuração versionada e protegida por CRC32;
- `TemperatureSettingsStore`: valor restaurado somente dentro da faixa permitida.

## Recuperação do histórico no microSD

O histórico continua usando uma fila circular de 64 registros em RAM. Nesta
fase, uma falha de escrita passa a classificar o armazenamento como:

- `SD_OK`;
- `SD_MISSING`;
- `SD_FULL`;
- `SD_ERROR`.

O `AppController` solicita uma remontagem pelo `BoardSupport` a cada 15 segundos
enquanto o armazenamento estiver degradado. Quando o cartão retorna, o
`HistoryService` grava a fila em ordem e volta a `SD_OK`. O cartão reserva 4 KiB
para evitar iniciar uma linha quando praticamente não existe espaço.

As linhas novas terminam com um CRC32 calculado sobre os doze campos do registro.
O leitor permanece compatível com linhas antigas sem checksum. Uma linha com
checksum ou campos inválidos é descartada individualmente; as demais continuam
disponíveis e o serial informa `HISTORY_CORRUPT`.

Limite conhecido: medições que estejam somente na fila RAM durante uma ausência
do SD não sobrevivem a uma queda de energia. O último peso e volume do KEG já
foram gravados no catálogo LittleFS antes da fila de histórico, portanto o estado
operacional reaparece após o boot, mas o ponto histórico pendente pode faltar.

## Interface e diagnósticos

O cartão `SISTEMA`, em `CONFIG.`, mostra o estado do SD e `FILA N`. O serial
publica transições como:

```text
BOOT_RECOVERY reset=... outputs=SAFE_OFF
STORAGE_STATE status=SD_MISSING pending=1
STORAGE_RECOVERY_WAIT status=SD_MISSING pending=1
STORAGE_RECOVERED pending=1
HISTORY_FLUSHED storage=SD
CATALOG_RECOVERED selected=A generation=... damaged=B
CATALOG_CORRUPT ... action=PRESERVE_AND_STOP
```

## Testes seguros de bancada

1. Execute os dez passos do roteiro da fase 17 no `README.md`.
2. Para simular SD cheio, use exclusivamente um cartão de teste quase cheio,
   deixando menos de 4 KiB livres. Confirme `SD CHEIO`; não use o cartão real.
3. Para testar corrupção de histórico, copie o SD, altere um caractere de uma
   linha CSV nova na cópia e abra o detalhe daquele KEG. Deve surgir
   `HISTORY_CORRUPT`; as linhas válidas permanecem no gráfico.
4. A corrupção do catálogo exige manipulação da partição LittleFS e não deve ser
   provocada no controlador de uso. A política foi inspecionada no repositório:
   um slot inválido recupera pelo outro; dois slots inválidos são preservados e
   bloqueiam novas gravações automáticas.

## Compilação

Ambiente `esp32_2432s028r` compilado com sucesso:

- RAM estática: 119.176 / 327.680 bytes (36,4%);
- flash: 1.475.813 / 1.900.544 bytes (77,7%);
- `firmware.bin`: 1.482.384 bytes;
- SHA-256: `BDC4AD825911EC562D8B819514E44E231EB3403E0B0B90E625E194CA2E45B4E8`.

Os smoke tests existentes cobrem rede, balança, filtro, presença, rastreamento,
cálculo, temperatura, catálogo e histórico. Nesta fase foram acrescentados os
casos de peso negativo, sensor explicitamente desconectado e recuperação da fila
após retorno do repositório. O ambiente local não possui compilador C++ nativo;
por isso os testes host não foram executados aqui, mas o firmware completo foi
compilado com `-Wall -Wextra` sem erros.
