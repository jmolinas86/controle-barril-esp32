# Atualização OTA web

## Acesso

Depois que o controlador estiver conectado à rede, consulte o IP em
`CONFIG. > REDE` e abra no navegador:

```text
http://<IP-DO-CONTROLADOR>/update
```

O monitor de diagnóstico usa a mesma autenticação:

```text
http://<IP-DO-CONTROLADOR>/logs
```

A página utiliza autenticação HTTP Basic. As credenciais ficam nas macros
`KEEZER_OTA_USERNAME` e `KEEZER_OTA_PASSWORD`, em
`include/NetworkConfig.h`. A configuração inicial de bancada é:

```text
usuário: admin
senha: keezer-update
```

Troque a senha antes da instalação definitiva. Como HTTP Basic não cifra a
senha, use a OTA somente na rede local confiável e nunca exponha a porta 80 do
controlador à internet.

## Gerar e enviar o firmware

Compile:

```powershell
platformio run -e esp32_2432s028
```

Selecione na página OTA:

```text
.pio/build/esp32_2432s028/firmware.bin
```

Não envie `bootloader.bin`, `partitions.bin` nem uma imagem de filesystem. A
página aceita apenas `.bin`, e o serviço usa `U_FLASH` para preservar NVS,
LittleFS e microSD.

## Comportamento

1. o navegador envia o arquivo em blocos e mostra o progresso;
2. o tamanho declarado é comparado ao espaço do slot OTA e ao total recebido;
3. o controlador pausa medições/persistência durante a escrita;
4. o GPIO candidato do compressor é reafirmado em OFF;
5. a biblioteca `Update` valida escrita e finalização;
6. após sucesso, a resposta é enviada ao navegador;
7. o ESP32 reinicia cerca de 1,5 s depois na nova partição.

Queda da rede ou ausência de dados por 15 s aborta o upload. Uma atualização
incompleta não é selecionada para o próximo boot e a versão atual continua
operando. As partições `app0`, `app1` e `otadata` já estão configuradas.

## Logs esperados

```text
Web OTA ready: port=80 path=/update auth=ENABLED
OTA endpoint available at /update
OTA_MAINTENANCE business_writes=PAUSED outputs=SAFE_OFF
OTA_STARTED file=firmware.bin
OTA_SUCCESS bytes=<tamanho>
OTA_RESTART
Phase 19.1 initialized: ... ota=WEB_AUTH
```

Em falha:

```text
OTA_FAILED reason=<motivo> code=<codigo>
```

## Monitor de logs

A página `/logs` mantém em RAM as 64 mensagens mais recentes, atualiza a cada
segundo e permite pausar, acompanhar automaticamente e limpar o buffer. O
conteúdo é volátil: reiniciar o controlador apaga o histórico. Nada é gravado
na flash ou no microSD, evitando desgaste e interferência no controle.

O endpoint de texto `/api/logs` e a limpeza por `POST /api/logs/clear` usam a
mesma autenticação HTTP Basic da OTA. O acesso deve permanecer restrito à rede
local confiável.

## Primeira instalação

O firmware que cria a interface OTA precisa ser gravado uma vez pelo USB:

```powershell
platformio run -e esp32_2432s028 -t upload
```

Depois dessa primeira gravação, as versões seguintes podem ser enviadas pelo
navegador. O USB continua sendo o caminho de recuperação.

## Limite desta versão

O particionamento alternado evita sobrescrever a aplicação em execução e a
biblioteca rejeita imagens incompletas. Rollback automático após uma imagem
completa que inicia e falha posteriormente depende de suporte/configuração do
bootloader e ainda não possui teste de bancada nesta fase.
