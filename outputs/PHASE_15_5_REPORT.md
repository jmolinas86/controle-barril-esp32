# Relatório da Fase 15.5 — Acompanhamento automático de consumo

## Resultado

A Fase 15.5 implementa uma sessão de acompanhamento para o KEG que foi
identificado por NFC ou confirmado manualmente. Enquanto a sessão permanece
confiável, reduções reais e estáveis de peso são convertidas em volume e
gravadas automaticamente. O sistema não tenta descobrir qual KEG está na
balança comparando pesos, pois isso permitiria associar uma medição ao cadastro
errado.

## Fluxo sem NFC

1. O operador coloca o KEG na balança.
2. Abre a tela de detalhes do KEG correto e toca em `PESAR`.
3. Uma leitura nova, online e estável é apresentada para confirmação.
4. Depois de confirmada, ela vira a referência da sessão e a tela mostra
   `MONITORANDO`.
5. A cada consumo, o peso precisa ficar estável por 3 s.
6. Uma queda acumulada de pelo menos 100 g, respeitando 30 s desde a última
   gravação, gera um registro `AUTO_TRACKING`.

O limite é acumulativo: várias retiradas pequenas passam a ser gravadas quando
a diferença total em relação à última referência salva alcançar 100 g.

## Proteções de identidade

- aumento igual ou superior a 300 g: `CHANGE_PENDING`;
- variação absoluta igual ou superior a 2.000 g sem NFC validado:
  `CHANGE_PENDING`;
- UID conhecido diferente do KEG ativo: o fluxo NFC faz a troca atômica;
- UID desconhecido ou ambíguo: nenhuma medição é atribuída automaticamente;
- comunicação `STALE` ou `OFFLINE`: acompanhamento `PAUSED`;
- reinício do controlador com cadastro ainda ativo: exige uma nova confirmação;
- peso abaixo de 1.000 g por 5 s: retirada física forte, tratada pelo serviço
  de presença.

Uma queda de 2.000 g deixou de ser usada como prova de retirada no modo
híbrido. Ela pode representar consumo legítimo e agora exige confirmação em
vez de remover silenciosamente o KEG ativo.

## Estados visuais

- `ESTABILIZANDO`: aguardando peso novo e estável;
- `MONITORANDO`: sessão segura e apta a registrar consumo;
- `PAUSADO`: balança sem comunicação; tocar em `PESAR` permite retomar;
- `VERIFICAR`: ocorreu uma mudança incompatível com a referência;
- `CONFIRMAR KEG`: ação exibida no detalhe quando é necessária nova pesagem.

## Componentes entregues

- `KegTrackingService`, máquina de estados sem dependência de Arduino ou UI;
- integração no `AppController` para sessões iniciadas por NFC e pesagem manual;
- nova origem de histórico `AutomaticTracking`;
- estado de acompanhamento exposto pelo `AppViewModel`;
- sinalização na HOME e na tela de detalhes;
- modo de presença híbrido, no qual consumo grande não remove o KEG;
- smoke tests de consumo, estabilidade, timeout, aumento, salto suspeito e
  queda grande validada por NFC.

## Parâmetros iniciais

Todos estão centralizados em `include/BuildConfig.h`:

- consumo mínimo: 100 g;
- confirmação estável: 3.000 ms;
- intervalo mínimo de gravação: 30.000 ms;
- aumento suspeito: 300 g;
- mudança suspeita sem identidade: 2.000 g;
- peso de plataforma removida: abaixo de 1.000 g;
- deadband do candidato: 20 g.

## Validação de software

- build PlatformIO `esp32_2432s028r`: sucesso;
- RAM estática: 116.696 de 327.680 bytes (35,6%);
- flash: 1.440.397 de 1.900.544 bytes (75,8%);
- binário: 1.446.976 bytes;
- SHA-256: `E18D6C724CE3E189EE70DDA7290AA572E22662089C01EB0DAFFB0E46EA46B1E2`;
- os smoke tests foram adicionados, mas não executados no desktop porque esta
  máquina não possui um compilador C++ nativo disponível.

## Critério para aprovação física

A fase estará aprovada depois que o controlador real demonstrar: início manual
da sessão, uma gravação automática após consumo maior que 100 g, ausência de
gravação abaixo do limite, pausa ao perder comunicação, pedido de confirmação
em aumento/troca suspeita e retomada correta após uma nova pesagem.

## Limite físico inevitável

Sem NFC ou outro identificador, a balança mede apenas massa. Dois KEGs podem
ter o mesmo peso, portanto nenhuma regra de software consegue distingui-los de
forma garantida. O modo sem NFC é seguro porque mantém o último KEG confirmado
e interrompe a automação quando há evidência de troca; o operador escolhe e
confirma o próximo KEG.
