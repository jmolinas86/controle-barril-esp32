# Relatório da Fase 5 — Domínio e armazenamento de KEGs

## Resultado

A Fase 5 implementa o catálogo persistente de KEGs sem alterar o visual aprovado na Fase 2.5 nem o comportamento dinâmico da Fase 4.

No primeiro boot, o firmware inicializa o LittleFS e cadastra os três KEGs de demonstração. Nos boots seguintes, o catálogo é lido da flash e os nomes, capacidades, taras, densidades e UIDs usados pelo `AppViewModel` vêm do `KegService`.

## Componentes entregues

- `models/Keg.h`: entidade, rascunho de cadastro e catálogo estático para até 32 registros;
- `models/KegStatus.h/.cpp`: ciclo de vida completo e regras de ativação automática;
- `models/KegMeasurement.h` e `domain/KegCalculator.h/.cpp`: cálculo inteiro de peso, volume e percentual, com estados explícitos de validade;
- `services/KegService.h/.cpp`: validação, busca, cadastro, edição, exclusão autorizada, arquivamento, finalização, marcação de vazio, vínculo NFC e troca atômica do ativo;
- `storage/IKegRepository.h`: contrato independente do meio físico;
- `storage/LittleFsKegRepository.h/.cpp`: implementação persistente A/B com geração e CRC32;
- `storage/MemoryKegRepository.h/.cpp`: implementação volátil para testes futuros;
- integração do catálogo com `AppController` e `AppViewModel`.

## Regras garantidas

1. IDs de KEG são únicos.
2. UIDs NFC são normalizados para hexadecimal maiúsculo sem separadores e não podem se repetir.
3. Há no máximo um `ACTIVE_ON_SCALE`.
4. A ativação de outro KEG armazena o anterior na mesma transação de domínio.
5. Estados finalizado, limpeza ou arquivado exigem confirmação para reativação.
6. Uma falha de persistência restaura a alteração em memória.
7. A exclusão exige autorização explícita e nunca ocorre por uma leitura física.
8. Capacidade, tara e densidade são validadas antes do cadastro.

## Estratégia de recuperação

Cada gravação escolhe o slot inativo, incrementa a geração, grava o snapshot completo, fecha o arquivo e relê todo o conteúdo para conferir tamanho e CRC. O slot anterior só deixa de ser o principal depois dessa validação. No boot, o repositório examina os dois arquivos e escolhe a maior geração válida.

Para evitar apagar dados após uma falha de montagem, o LittleFS só pode ser formatado automaticamente quando ainda não existe a marca de primeira inicialização no NVS.

A montagem usa explicitamente o rótulo `littlefs`, igual ao declarado em `partitions.csv`. Isso evita que a biblioteca procure o rótulo padrão `spiffs` e inicialize o armazenamento em modo degradado.

## Integração com a demonstração

Os dados cadastrais exibidos vêm do catálogo persistente. Temperatura, consumo acelerado e presença alternada continuam transitórios no `DemoDataSource`, portanto não geram uma gravação na flash a cada animação.

## Verificação

Comando:

```powershell
platformio run -e esp32_2432s028r
```

Resultado: compilação concluída com sucesso.

- RAM: 105.796 de 327.680 bytes (32,3%);
- flash: 858.857 de 1.900.544 bytes (45,2%);
- firmware: 859.216 bytes;
- SHA-256: `8A6FE8B13B0435377336D7D7B9FDBDD8673D07393193FC99507A23C20A6B139A`.

O teste de fumaça de `KegService` também foi compilado para a arquitetura Xtensa sem avisos. A execução das operações persistentes em LittleFS ainda deve ser confirmada na placa, porque o firmware não foi enviado automaticamente.

## Teste recomendado na placa

1. Grave o firmware e abra o monitor serial.
2. No primeiro boot, procure `First mount; formatting LittleFS once` e quatro gerações salvas (três cadastros e uma ativação).
3. Reinicie a placa sem regravar o filesystem.
4. Confirme no serial `Loaded generation=4, kegs=3`.
5. Confira que a HOME e os detalhes continuam iguais e que a simulação segue atualizando sem novas mensagens `Saved generation`.

O upload não foi executado automaticamente; o binário foi apenas compilado.
