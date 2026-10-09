# Interface e operação

## Estrutura

A interface usa LVGL 9.5 em 240 × 320 vertical. Cabeçalho e menu inferior são
fixos; na HOME, o cartão do KEEZER também fica fixo e somente a lista de KEGs
rola. O tema usa fundo escuro, azul para ação/informação, verde para operação
normal, laranja para atenção e vermelho para falha ou ação destrutiva.

## HOME

- temperatura, setpoint e estado lógico do compressor;
- cartões dos KEGs não arquivados;
- volume, percentual, último SYNC e estado de leitura;
- toque no botão com ícone de lista para abrir detalhes;
- `NOVO KEG` abre o cadastro.

`LENDO` significa KEG ativo/acompanhado; `LIDO` indica KEG fora da leitura
ativa. Os valores são atualizados incrementalmente sem recriar toda a tela.

## Cadastro e edição

Campos: ID, cerveja, estilo, lote, data de envase, capacidade, tara, densidade,
UID NFC e observações. O teclado virtual abre sobre a metade inferior e a lista
continua rolável. ID é bloqueado durante edição.

Capacidade é digitada em litros, tara em kg e densidade em kg/L. O firmware
valida limites, ID/NFC duplicados e falha de armazenamento antes de confirmar.
O nome da cerveja pode ser alterado em `DETALHE > EDITAR`.

## Detalhe do KEG

- resumo de volume, percentual e barra;
- `PESAR` para seleção/medição manual sem NFC;
- `RETIRAR` para confirmar que saiu da balança;
- tara, densidade, volume, peso, último SYNC, capacidade, NFC e estado;
- gráfico com os sete volumes recentes disponíveis;
- `EDITAR`, `ASSOC. NFC`, `FINALIZAR` e `ARQUIVAR`.

`FINALIZAR` mostra aviso e apaga o registro. `ARQUIVAR` preserva o registro e
somente o oculta da HOME. O histórico no SD é preservado nos dois casos.

## KEEZER

Permite ajustar e salvar o setpoint, consultar temperatura, demanda, estado do
compressor, proteção restante, sensor e falhas. A implementação térmica atual
é simulada; consulte `TEMPERATURE_CONTROL.md`.

## CONFIG.

- `BALANCA`: link, último peso e NFC;
- `REDE`: abre formulário de SSID, senha, hostname, host/IP e porta da balança;
- `SISTEMA`: estado do SD e tamanho da fila em RAM;
- `BARRIS`, `TEMPERATURA` e `DISPLAY`: cartões reservados para expansão; nesta
  versão apenas `REDE` possui tela própria nesse grid.

## Fluxos operacionais

### Novo KEG

1. toque em `NOVO KEG`;
2. preencha nome, medidas e demais campos;
3. deixe NFC vazio se trabalhar sem PN532;
4. toque em `SALVAR` e confira o cartão na HOME;
5. reinicie para confirmar a persistência.

### Associar NFC

Abra o detalhe, toque em `ASSOC. NFC` e siga a leitura/edição apresentada. Um
UID só pode pertencer a um KEG.

### Pesar sem NFC

Abra o KEG correto, toque `PESAR`, espere leitura online/estável, confira peso e
volume e confirme. Ao trocar de KEG, selecione explicitamente o novo; peso
sozinho não identifica barris.

### Configurar a rede

Abra `CONFIG. > REDE`, informe dados e salve. Campo de senha vazio mantém a
senha existente. A conexão é reiniciada sem reiniciar todo o controlador.

## Desempenho visual

O perfil aprovado usa buffer único DMA de 24 linhas e clock de 80 MHz. A espera
do DMA eliminou as faixas pretas que surgiram quando o buffer era reutilizado.
O efeito cortina ficou quase imperceptível na avaliação de bancada; eventuais
ajustes estéticos não devem alterar esse caminho de flush sem novo teste.

## Atualização OTA

A OTA não acrescenta uma tela LVGL: sua interface leve abre no navegador em
`http://<IP-DO-CONTROLADOR>/update`. Consulte o IP em `CONFIG. > REDE` e siga
`OTA.md`.
