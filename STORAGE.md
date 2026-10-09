# Persistência e histórico

## Mapa de armazenamento

| Dado | Meio | Formato/local |
|---|---|---|
| catálogo de KEGs | LittleFS interno | `/kegs_a.bin` e `/kegs_b.bin` |
| rede e balança | NVS | namespace `network_cfg` |
| setpoint | NVS | namespace `temp_cfg` |
| calibração do touch | NVS | namespace `touch-cal` |
| histórico de pesagens | microSD | `/history/<kegId>/unsynced-<bootId>.csv` |

A OTA grava somente o slot de aplicativo inativo. Ela não substitui as
partições NVS/LittleFS nem grava no microSD.

A tabela de partições reserva 320 KiB para LittleFS, dois slots OTA de
1.856 KiB cada e 20 KiB para NVS.

## Catálogo LittleFS

Cada salvamento grava o slot inválido ou mais antigo, incrementa a geração,
fecha o arquivo e o relê para validar. O cabeçalho contém assinatura `KEGS`,
versão, tamanho do registro, geração, quantidade e CRC32 do payload.

No boot, vence a maior geração válida. Se um slot estiver corrompido, o outro é
usado. Se os dois estiverem inválidos, os arquivos são preservados e o firmware
se recusa a sobrescrever o catálogo. A formatação automática só é permitida no
primeiro mount conhecido.

## NVS

Rede e endereço da balança são gravados como um bloco versionado com CRC32. A
senha existente nunca volta para a tela: deixar `NOVA SENHA` vazia mantém o
valor atual. O setpoint é persistido em centésimos de grau Celsius e validado
entre −5,00 e 15,00 °C.

Evite distribuir ou versionar `include/NetworkConfig.h` com credenciais reais.
Use valores de exemplo em cópias públicas e configure o aparelho pela tela.

## Histórico no microSD

Cada KEG tem um diretório próprio. Enquanto não existe relógio civil
sincronizado, o nome usa o identificador do boot e o registro guarda tempo
monotônico. Uma linha contém:

```text
versao,sequencia,boot,utc,monotonico,peso_bruto,peso_filtrado,volume,
percentual_bp,origem,validade,flags,crc32
```

Origens: NFC, manual e acompanhamento automático. Cada linha recebe CRC32;
linhas inválidas são ignoradas na leitura, preservando as demais. Os sete
pontos recentes alimentam o gráfico do detalhe do KEG.

## Operação degradada

Sem SD, até 64 registros ficam em fila circular na RAM. O firmware tenta
remontar o cartão a cada 15 s e descarrega a fila quando ele volta. Se lotar,
o registro mais antigo é descartado e o serial informa
`HISTORY_BUFFER_OVERFLOW`. O sistema reserva pelo menos 4 KiB livres no cartão.

Consulte `CONFIG. > SISTEMA`:

- `SD OK`: gravando normalmente;
- `SD AUSENTE`: cartão não montado;
- `SD CHEIO`: sem espaço seguro;
- `SD ERRO`: falha de I/O ou dados;
- `FILA N`: registros aguardando persistência.

## Preservação e cópia

Para preservar o histórico, desligue o controlador antes de retirar o cartão e
copie toda a pasta `/history` para o computador. Excluir/finalizar/arquivar um
KEG não remove seus CSVs. Não edite os binários do LittleFS manualmente; o
snapshot de desenvolvimento do projeto deve ser feito copiando a pasta do
firmware ou usando controle de versão.
