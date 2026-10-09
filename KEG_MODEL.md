# Modelo e ciclo de vida dos KEGs

## Limites e campos

O catálogo suporta até 32 KEGs. O domínio usa gramas, mililitros e densidade em
gramas por litro para evitar erro de ponto flutuante na persistência.

| Campo | Regra |
|---|---|
| ID | obrigatório, único, até 16 caracteres úteis |
| NFC UID | opcional, hexadecimal normalizado, único, até 20 caracteres úteis |
| cerveja, estilo e lote | até 32 caracteres úteis cada |
| observações | até 160 caracteres úteis |
| capacidade | 1.000 a 100.000 mL |
| tara | peso vazio em gramas; deve ser compatível com a capacidade |
| densidade | 900 a 1.300 g/L |
| último peso/volume | última medição confirmada |
| datas | criação, envase, ativação e último contato; `-1` quando desconhecida |

## Estados

- `AVAILABLE`: cadastrado e disponível;
- `ACTIVE_ON_SCALE`: KEG selecionado sobre a balança;
- `STORED`: armazenado fora da balança;
- `EMPTY`: vazio;
- `FINISHED`: estado de domínio preservado para compatibilidade;
- `CLEANING`: em limpeza;
- `ARCHIVED`: oculto da HOME, mas preservado.

Somente um KEG pode estar `ACTIVE_ON_SCALE`. Ativar outro armazena o anterior
na mesma transação lógica e a gravação tem rollback se a persistência falhar.

Na interface atual, **FINALIZAR** confirma e apaga permanentemente o registro do
catálogo. **ARQUIVAR** apenas muda o estado para `ARCHIVED`: some da HOME, mas
continua persistido. Nenhuma dessas ações apaga automaticamente os arquivos de
histórico do cartão SD.

## Fórmulas

```text
pesoCervejaG = max(0, pesoBrutoG - taraG)
volumeMl = pesoCervejaG * 1000 / densidadeGPorLitro
volumeExibidoMl = limitar(volumeMl, 0, capacidadeMl)
percentualBasisPoints = volumeExibidoMl * 10000 / capacidadeMl
```

Dez mil basis points equivalem a 100,00%. A interface arredonda o percentual
para inteiro, mas o domínio preserva duas casas.

Validades possíveis: `VALID`, `BELOW_TARE`, `ABOVE_EXPECTED_MAXIMUM`,
`INVALID_WEIGHT` e `INVALID_KEG_CONFIGURATION`. Existe tolerância de 500 g
acima do peso esperado antes do aviso de excesso.

## Identificação híbrida

### Com NFC

O mesmo UID deve aparecer por três leituras consecutivas ou 1,5 s. UID conhecido
seleciona o KEG e permite registrar a medição; UID desconhecido oferece cadastro
ou cancelamento. A troca A → B só ocorre após estabilização do novo UID/peso.

### Sem NFC

Abra o detalhe do KEG desejado e toque em `PESAR`. O sistema espera uma leitura
nova, online e estável, mostra peso/volume calculados e só grava após confirmação.
Essa confirmação cria uma referência para o acompanhamento automático.

Sem NFC, peso sozinho nunca identifica qual KEG foi colocado. Aumento de pelo
menos 300 g, salto de 2.000 g ou troca ambígua pausa a sessão e pede confirmação.

## Acompanhamento automático

- consumo acumulado mínimo: 100 g;
- confirmação estável: 3 s;
- intervalo mínimo entre registros: 30 s;
- retirada forte: leitura abaixo de 1.000 g por 5 s;
- mudança grande sem NFC não remove automaticamente o KEG;
- após reiniciar, confirme `PESAR` novamente para criar referência segura.

Os nomes de cerveja são editados em `DETALHE DO KEG > EDITAR`. Alterar tara,
capacidade ou densidade muda o cálculo das próximas medições.
