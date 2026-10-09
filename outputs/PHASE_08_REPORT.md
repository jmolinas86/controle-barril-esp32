# Relatório da Fase 8 — Identificação híbrida

## Resultado

A identificação do KEG agora funciona em modo híbrido. O controlador aceita a seleção automática por NFC estabilizado e oferece seleção manual pelo botão `PESAR` na tela de detalhes. A ausência física do PN532 não impede o uso da balança.

## Fluxo NFC

1. `ScaleService` recebe o UID normalizado.
2. O UID precisa repetir três vezes ou permanecer por 1,5 segundo.
3. `AppController` consulta `KegService::findByNfcUid()`.
4. Um UID conhecido ativa somente o KEG correspondente.
5. Um UID desconhecido publica `UNKNOWN_NFC_TAG` sem alterar o catálogo.
6. A interface permite ignorar a tag ou cadastrar um KEG com o UID preenchido.

Ausência e troca direta também são estabilizadas. Alternância rápida entre tags mantém a identificação anterior e sinaliza ambiguidade. A retirada automática foi deliberadamente mantida fora desta fase.

## Fluxo manual

1. O usuário abre o detalhe do KEG e toca em `PESAR`.
2. A sessão descarta a leitura antiga e espera uma nova leitura online e estável.
3. O domínio calcula o volume usando tara e densidade já cadastradas.
4. A tela mostra peso e volume antes de salvar.
5. Se o NFC estabilizado indicar outro KEG, a confirmação mostra um alerta.
6. `SALVAR` grava peso/volume e troca o KEG ativo atomicamente.
7. `CANCELAR` não altera o catálogo.

A espera expira depois de 60 segundos. Peso inválido ou configuração inválida não são persistidos. Leituras abaixo da tara e acima do máximo esperado exigem confirmação visual explícita.

## Persistência e segurança

- peso e volume não são gravados a cada pacote da balança;
- apenas a confirmação manual persiste uma medição;
- a ativação e a medição manual usam uma única gravação do catálogo;
- falha de armazenamento restaura em memória o KEG anterior e o KEG selecionado;
- o peso nunca identifica sozinho o KEG;
- o modo manual tem precedência durante uma sessão iniciada pelo usuário.

## Verificação realizada

- build PlatformIO `esp32_2432s028r`: sucesso;
- RAM: 124.260 de 327.680 bytes (37,9%);
- flash: 898.421 de 1.900.544 bytes (47,3%);
- `firmware.bin` SHA-256: `F545E4EABC8FB46ECFA45A871B495C14660183A49D06283D74937B4FF585A3BA`;
- verificação sintática dos testes de `ScaleService` e `KegService`: sucesso;
- casos cobertos: estabilização, remoção, troca de UID, ambiguidade, medição persistida e exclusividade do ativo.

## Teste na placa

1. Grave o firmware e abra o monitor serial.
2. Aguarde `NFC stable: 04A23F891C` e `NFC identified: uid=04A23F891C keg=KEG_001`.
3. Abra um KEG e toque em `PESAR`.
4. Confira `AGUARDANDO PESO...` e depois o modal com peso e volume.
5. Confirme em `SALVAR` e verifique o novo peso/volume na tela e somente esse KEG como `LENDO`.
6. Reinicie e confirme que a medição permaneceu salva.
7. Repita usando `CANCELAR` e confirme que os dados anteriores permanecem.
8. Teste um UID desconhecido alterando temporariamente a associação do UID simulado e valide `CADASTRAR`/`IGNORAR`.

## Próxima fase

A Fase 9 implementará retirada, timeout de presença e troca física completa do barril. O driver PN532 real poderá ser adicionado ao firmware da balança posteriormente sem mudar o fluxo híbrido do controlador.
