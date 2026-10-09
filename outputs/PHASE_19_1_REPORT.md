# Relatório da Fase 19.1 — OTA web local

## Implementação

- novo `OtaService` isolado em `src/ota`;
- servidor HTTP nativo na porta 80, sem dependência externa;
- página HTML/CSS/JavaScript embutida no firmware;
- rota autenticada `GET /update`;
- upload autenticado `POST /update`;
- atualização exclusiva de aplicação com `Update` e `U_FLASH`;
- extensão `.bin`, tamanho declarado/recebido, espaço do slot e resultado da
  escrita validados;
- timeout de 15 s e aborto quando a rede cai;
- reinício adiado para permitir que o navegador receba a confirmação;
- processamento de balança, catálogo e histórico pausado durante upload;
- GPIO candidato do compressor reafirmado em estado seguro;
- credenciais não aparecem nos logs.

## Preservação

A atualização troca apenas a partição de aplicativo. NVS, catálogo LittleFS e
histórico no microSD não são gravados pela OTA.

## Compilação

Comando validado:

```powershell
platformio run -e esp32_2432s028r
```

Resultado: sucesso, 123.968 bytes de RAM estática (37,8%) e 1.521.773 bytes de
flash (80,1%). A aplicação ocupa menos que o limite de 1.900.544 bytes de cada
slot OTA.

O `firmware.bin` possui 1.528.352 bytes e SHA-256
`B8A2720260258C52CA283DA819B2074367D45DC190D4AE56342B177F324A4C29`.

## Aceitação no equipamento

Em 12/09/2026, o usuário confirmou que a interface OTA está funcionando
corretamente no controlador real. O acesso autenticado, envio do firmware,
reinício e retorno do sistema foram aprovados na bancada.

Estado da fase: **APROVADA**.

Rollback de uma imagem completa que falhe somente depois do boot não foi
declarado como validado; permanece uma melhoria futura do bootloader.
