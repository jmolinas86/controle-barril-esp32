# Relatório da Fase 6 — Interface de gerenciamento de KEGs

## Resultado

A Fase 6 transforma o catálogo persistente da Fase 5 em uma interface touchscreen funcional. É possível cadastrar e editar KEGs, informar manualmente a TAG NFC, finalizar e apagar um registro, ou arquivar um cadastro sem acessar o armazenamento diretamente pela UI.

## Componentes entregues

- `ui/screens/KegListScreen.h/.cpp`: lista reutilizável, estado vazio, scroll e acesso ao novo cadastro;
- `ui/screens/KegEditScreen.h/.cpp`: formulário rolável, teclado LVGL alfanumérico/numérico, conversão das medidas e mensagens de validação;
- `KegDetailScreen`: ações `EDITAR`, `ASSOC. NFC`, `FINALIZAR` e `ARQUIVAR`;
- `Modal`: confirmação explícita para mudanças de ciclo de vida;
- `ScreenManager`: rotas de criação/edição, seleção estável por ID e encaminhamento das intenções;
- `AppViewModel`: comandos de UI para o `KegService`, geração automática do próximo ID e conversão da data de envase;
- catálogo visual ampliado para até 32 KEGs e filtro de registros arquivados.

## Fluxo implementado

```text
Touchscreen
    -> ScreenManager
    -> AppViewModel
    -> KegService
    -> LittleFsKegRepository
    -> atualização do AppViewModel
    -> UI
```

A UI continua sem acessar o LittleFS diretamente. Em uma falha de validação ou persistência, o formulário permanece aberto e exibe o motivo. Um salvamento bem-sucedido abre o detalhe do KEG gravado.

## Campos do formulário

- ID, preenchido automaticamente no novo cadastro e bloqueado durante edição;
- nome da cerveja;
- estilo;
- lote;
- data de envase no formato `DD/MM/AAAA`;
- capacidade em litros;
- tara em quilogramas;
- densidade em quilogramas por litro;
- UID NFC manual;
- observações.

Faixas aceitas: capacidade de 1 a 100 L, tara de 0 a menos de 100 kg, densidade de 0,900 a 1,300 kg/L e data entre 2000 e 2199. A UID é normalizada pelo domínio e deve ser exclusiva.

## Persistência e ciclo de vida

- criar e editar produzem um novo snapshot A/B validado por CRC;
- finalizar, após aviso e confirmação, apaga permanentemente todo o registro;
- arquivar muda o estado para `ARCHIVED` e oculta o KEG da lista principal;
- nenhuma dessas ações exclui o cadastro;
- confirmações são exigidas antes de finalizar ou arquivar.

## Verificação

Comando:

```powershell
platformio run -e esp32_2432s028r
```

Resultado: compilação concluída com sucesso e sem avisos do código da aplicação.

- RAM: 123.700 de 327.680 bytes (37,8%);
- flash: 890.373 de 1.900.544 bytes (46,8%);
- firmware: 890.736 bytes;
- SHA-256: `8A70DD7BE7BE495882ECBA5078124EEF3B9395CFAE7ECE566C2E6BAA384A8805`.

## Teste recomendado na placa

1. Grave o firmware e confirme `Phase 6 initialized: kegs=3 storage=OK`.
2. Toque em `NOVO KEG` e verifique o ID sugerido `KEG_004`.
3. Preencha nome, estilo, lote, data, capacidade, tara, densidade, NFC e observações; salve.
4. Confirme que o detalhe do novo KEG abre e que a HOME passa a listar quatro registros.
5. Reinicie e confirme que o cadastro permanece.
6. Edite o nome e as medidas; reinicie novamente e confira os valores.
7. Tente usar uma NFC que já pertence a outro KEG e confirme a rejeição por duplicidade.
8. Finalize um KEG de teste e confirme que o aviso informa a exclusão permanente e que o registro é apagado.
9. Arquive o KEG de teste e confirme que ele desaparece da lista após a confirmação.

## Limites atuais

- `ASSOC. NFC` permite digitação manual; captura automática depende da balança simulada da Fase 7 e do fluxo da Fase 8;
- arquivados permanecem persistidos, mas ainda não há uma tela para restaurá-los;
- exclusão definitiva foi exposta em `FINALIZAR`, com aviso explícito e confirmação obrigatória;
- peso, presença, temperatura, horário e gráfico continuam simulados;
- o comportamento visual e a ergonomia do teclado ainda precisam ser confirmados na placa física.
