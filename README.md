# Keezer Controller — Fase 19.3

Firmware local para a placa ESP32-2432S028 de dois USB, com ESP32-WROOM-32, painel ST7789, touch resistivo XPT2046 e interface LVGL em 240 × 320 portrait.

A Fase 19 consolida a documentação final da versão atual. A Fase 19.1 acrescenta
uma interface OTA web local, protegida por autenticação, para atualizar somente
o firmware sem apagar catálogo, rede, setpoint ou histórico. A Fase 19.2 completa
as configurações locais com consulta dos KEGs arquivados, exclusão confirmada,
atalho OTA e brilho/tempo de repouso persistentes. O controlador possui
catálogo persistente de até 32 KEGs, comunicação HTTP com a balança, operação
híbrida com ou sem NFC, histórico em microSD, acompanhamento automático de
consumo, interface touchscreen e controle térmico lógico com proteções.

> **Estado do hardware térmico:** esta versão usa um DS18B20 real no GPIO 22 e
> um módulo de relé ativo em HIGH no GPIO 27. O relé inicia sempre em OFF e
> somente pode ligar após leituras válidas e o tempo de proteção do compressor.

## Documentação da versão

- [Arquitetura](ARCHITECTURE.md)
- [Hardware e pinagem](HARDWARE.md)
- [Protocolo da balança](SCALE_PROTOCOL.md)
- [Modelo e ciclo de vida dos KEGs](KEG_MODEL.md)
- [Persistência e histórico](STORAGE.md)
- [Controle de temperatura](TEMPERATURE_CONTROL.md)
- [Interface e operação](UI.md)
- [Atualização OTA](OTA.md)
- [Compilação, gravação e testes](TESTING.md)
- [Relatório da Fase 19](outputs/PHASE_19_REPORT.md)
- [Relatório da Fase 19.1](outputs/PHASE_19_1_REPORT.md)
- [Relatório da Fase 19.2](outputs/PHASE_19_2_REPORT.md)
- [Relatório da Fase 19.3](outputs/PHASE_19_3_REPORT.md)

As credenciais de Wi-Fi nunca devem ser publicadas. Copie
`include/NetworkConfig.example.h` para `include/NetworkConfig.h` e preencha o
arquivo local, que e ignorado pelo Git. Ele serve apenas como configuracao
inicial; depois do primeiro salvamento em
`CONFIG. > REDE`, a configuração válida da NVS passa a ter prioridade.

## Situação visual atual

O teste aprovado usa SPI do display a 80 MHz, um buffer DMA de 240 × 24 linhas
e espera explícita do término da transferência. Isso reduziu quase por completo
o efeito cortina observado na HOME. Ajustes puramente estéticos serão tratados
depois da Fase 19.

## OTA web — Fase 19.1

- validada e aprovada no controlador real em 12/09/2026;
- página leve em `http://<IP-DO-CONTROLADOR>/update`;
- autenticação HTTP Basic obrigatória;
- aceita somente arquivo com extensão `.bin`;
- grava exclusivamente a partição de aplicativo inativa;
- apresenta progresso no navegador e reinicia após sucesso;
- falha de rede ou 15 s sem dados aborta a operação;
- durante o upload, gravações de negócio ficam pausadas e o GPIO 27 do
  compressor é reafirmado em estado seguro (OFF/LOW);
- credenciais iniciais ficam em `include/NetworkConfig.h` e devem ser trocadas
  antes de uso fora da bancada.

## Configurações locais — Fase 19.2

- `BARRIS` lista apenas os KEGs arquivados e abre o histórico individual;
- a exclusão definitiva exige confirmação e remove o cadastro, preservando o
  histórico bruto no microSD como cópia de segurança;
- o cartão sem função `TEMPERATURA` foi substituído por `OTA`, que mostra o
  endereço local de atualização;
- `DISPLAY` ajusta brilho de 10% a 100% e repouso em 30 s, 1, 2 ou 5 min, além
  da opção sempre ligado;
- as preferências do display sobrevivem à reinicialização;
- depois do repouso, o primeiro toque somente acorda a tela e não aciona a UI;
- o estado da balança foi reposicionado abaixo do título para melhorar a leitura.

## Otimização de comunicação — Fase 19.3

O controlador consulta a balança de forma adaptativa, sem mudar o protocolo:

- `FAST`: 500 ms durante mudanças, instabilidade e pelos 10 s seguintes;
- `STABLE`: 2 s quando há peso estável acima de 1 kg;
- `IDLE`: 5 s quando a plataforma está estável e com até 1 kg;
- uma diferença de pelo menos 20 g retorna automaticamente ao modo rápido;
- o serial registra cada transição como `POLL_MODE`.

Somente o firmware do controlador precisa ser atualizado nesta etapa. A
economia interna da balança — OLED, NFC, HX711 e sono do Wi-Fi — exigirá uma
etapa própria e uma atualização posterior do firmware da balança.

A Fase 18 mede o desempenho do firmware no hardware real antes de ampliar as
otimizações. O serial passa a informar heap, fragmentação, duração do loop,
atividade do LVGL, flush do display e acessos ao microSD em janelas de 30 s.

## Otimização medida — Fase 18

- `PERF_HEAP` informa heap livre, mínimo histórico, maior bloco e fragmentação;
- `PERF_LOOP` informa chamadas, duração média/máxima e loops acima de 10 ms;
- `PERF_UI` informa FPS, chamadas/tempo do LVGL e custo dos flushes do display;
- `PERF_SD` informa leituras, escritas, falhas, bytes e duração das operações;
- o LVGL agora é executado no prazo solicitado pelo próprio gerenciador, com
  intervalo máximo de 5 ms para preservar a resposta do touch;
- após o retardo observado na bancada, o touch passou de 20 para 10 ms, a
  atualização visual de 33 para 20 ms, o SPI do display de 40 para 60 MHz e os
  buffers parciais de 8 para 12 linhas;
- o arraste começa após 5 pixels e mantém inércia curta para acompanhar o dedo;
- teste isolado atual: clock de escrita em 80 MHz e um único buffer DMA de
  240 × 24 pixels, mantendo os mesmos 11.520 bytes antes usados por dois
  buffers de 240 × 12;
- por existir somente um buffer, cada faixa aguarda a conclusão do DMA antes
  de ser liberada ao LVGL, evitando que a imagem seja sobrescrita em trânsito;
- após o vídeo de bancada confirmar *screen tearing*, o ST7789 passou a usar
  sincronização pela leitura da linha de varredura, transferência DMA e buffer
  `RGB565_SWAPPED` sem conversão durante o flush;
- atualizações visuais de sensores ficam adiadas somente durante drag/inércia,
  sem interromper aquisição, controle térmico ou comunicação com a balança;
- nenhuma redução de qualidade gráfica, buffer ou recurso foi feita sem medição.

A otimização do agendamento gráfico foi aplicada porque a telemetria anterior
mostrou cerca de 1,4 milhão de chamadas ao handler em 30 s. As próximas decisões
dependem dos blocos `PERF_*` obtidos no controlador real.

## Robustez — Fase 17

- Wi-Fi perdido mantém a interface e o controle local ativos, com reconexão progressiva;
- balança sem comunicação pausa presença e acompanhamento sem trocar o KEG;
- NFC ausente passa por debounce; UID desconhecido não recebe peso nem altera o catálogo;
- sensor desconectado, com timeout ou fora da faixa força o compressor a `OFF`;
- peso negativo ou acima de 100 kg é rejeitado antes do filtro e não substitui a última leitura;
- tara, capacidade ou densidade inválida são recusadas no cadastro e no cálculo;
- remoção ou falha do SD mantém até 64 registros numa fila circular em RAM;
- o SD é remontado automaticamente a cada 15 s e a fila é gravada quando ele retorna;
- SD cheio é distinguido de SD ausente, mantendo uma reserva mínima de 4 KiB;
- novas linhas do histórico recebem CRC32 individual; linhas danificadas são ignoradas sem esconder as válidas;
- `CONFIG. > SISTEMA` mostra `SD OK`, `SD AUSENTE`, `SD CHEIO` ou `SD ERRO`, além do tamanho da fila;
- catálogo interno continua em snapshots A/B; um slot íntegro recupera o outro corrompido;
- se os dois slots estiverem inválidos, os arquivos são preservados e o catálogo não é sobrescrito;
- o boot registra a causa do reset e começa com a saída do compressor em estado seguro.

## Rede — Fase 16

- `NetworkService` possui a conexão e uma porta `INetworkAdapter` testável;
- `Esp32WiFiAdapter` é o único adaptador que chama `WiFi.begin()` e reconecta;
- estados `CONECTANDO`, `CONECTADA`, `RECONECTANDO` e `CONFIG. INVALIDA`;
- tentativa de conexão limitada a 20 s;
- reconexão com espera progressiva de 5, 10, 20, 40 e no máximo 60 s;
- IP e RSSI atualizados a cada 2 s;
- `CONFIG. > REDE` permite alterar SSID, senha, hostname e IP/host/porta da balança;
- senha existente nunca é mostrada: campo vazio significa manter a atual;
- configuração salva em NVS como bloco versionado e protegido por CRC32;
- a nova configuração reinicia somente o Wi-Fi e a conexão da balança;
- cabeçalho e cartão `REDE` indicam o estado real da conexão;
- a senha nunca é registrada no serial nem exposta no snapshot da interface;
- nenhuma interface web completa foi criada nesta fase.

## Acompanhamento automático — Fase 15.5

- a sessão começa somente depois de NFC conhecido ou pesagem manual confirmada;
- sem NFC, o controlador nunca escolhe outro KEG apenas pelo peso;
- redução acumulada mínima de 100 g inicia uma possível medição;
- o novo peso precisa permanecer estável por 3 s;
- existe um intervalo mínimo de 30 s entre gravações automáticas;
- cada gravação atualiza peso, volume, percentual e histórico com origem `AUTO_TRACKING`;
- aumento de 300 g ou mais pausa a sessão e pede confirmação;
- salto de 2.000 g ou mais sem NFC validado também pede confirmação;
- balança `STALE` ou `OFFLINE` pausa o acompanhamento sem trocar o KEG ativo;
- leitura próxima de zero continua sendo tratada como retirada física;
- `MONITORANDO`, `ESTABILIZANDO`, `PAUSADO` e `VERIFICAR` aparecem na HOME e no detalhe;
- ao reiniciar com um KEG ainda marcado como ativo, é necessário abrir seu detalhe e confirmar `PESAR` para estabelecer uma nova referência segura.

## Comunicação real — Fase 15

- consulta `GET http://10.0.0.188/api/v1/reading` a cada 500 ms nesta instalação;
- descoberta por mDNS com IP em cache e nova resolução após falhas;
- timeout de 1 s e tamanho máximo de resposta limitado;
- `HTTP 503` tratado como balança acessível ainda sem leitura válida;
- JSON convertido para estruturas fixas e validado antes de chegar ao domínio;
- detecção de duplicidade por `sequence` e de reinício por `uptime_ms`;
- `nfc_uid=null` é válido e mantém a alternativa manual pelo botão `PESAR`;
- contrato completo em `outputs/SCALE_PROTOCOL.md`.

## Histórico — Fase 14

- `HistoryService` independente do catálogo e do controle térmico;
- `SdHistoryRepository` com um diretório separado para cada `kegId`;
- arquivos `/history/<kegId>/unsynced-<bootId>.csv` enquanto não existe relógio civil sincronizado;
- cada linha registra versão, sequência, boot, tempo monotônico, peso bruto/filtrado, volume, percentual, origem, validade e CRC32;
- pesagens NFC repetidas em menos de 60 s e com variação inferior a 50 mL são descartadas;
- pesagens manuais confirmadas sempre geram um ponto;
- buffer circular de 64 registros em RAM para funcionamento degradado sem cartão;
- gravação imediata após cada pesagem confirmada, com remontagem e recuperação automática a cada 15 s quando o cartão falha;
- estouro do buffer descarta o registro mais antigo e gera alerta explícito no serial;
- gráfico da tela de detalhes mostra as sete pesagens reais mais recentes, de `ANTERIOR` a `RECENTE`;
- excluir ou finalizar um KEG não apaga automaticamente os arquivos do histórico.

## Interface térmica — Fase 13

- tela `CONTROLE KEEZER` acessível pelo menu inferior;
- temperatura atual e setpoint em destaque;
- ajuste por toque em passos de 0,1 °C;
- faixa permitida de −5,0 a 15,0 °C;
- alteração pendente fica laranja e só é aplicada ao tocar em `SALVAR SETPOINT`;
- setpoint salvo em NVS e restaurado no próximo boot;
- compressor apresentado como `PARADO`, `AGUARDANDO`, `RESFRIANDO` ou `ERRO`;
- contador em segundos durante a proteção anti-ciclo;
- sensor apresentado como conectado, sem leitura, sem comunicação, desconectado, fora da faixa ou em erro;
- mensagens visuais para confirmação, limite e falha de gravação;
- a interface chama o `TemperatureService`; nenhum componente visual acessa GPIO.

## Controle térmico — Fase 12

- setpoint inicial: 2,00 °C;
- histerese total: 1,00 °C, ligando em 2,50 °C e desligando em 1,50 °C;
- proteção de produção: mínimo de 180 s desligado e 60 s ligado;
- build atual usa exclusivamente os tempos de produção;
- timeout do sensor: 10 s;
- faixa válida: −20,00 °C a +50,00 °C;
- estados: `IDLE`, `WAITING`, `COOLING` e `ERROR`;
- falha crítica desliga imediatamente, mesmo durante o tempo mínimo ligado;
- recuperação exige três novas amostras válidas consecutivas;
- alterar setpoint não ignora as proteções anti-ciclo;
- `GpioCompressorOutput` comanda o módulo ativo em HIGH no GPIO 27.

## Filtro de peso — Fase 11

- `rawWeightGrams` conserva a última leitura válida recebida, inclusive quando instável;
- somente amostras marcadas como estáveis entram no filtro;
- a mediana usa uma janela fixa de cinco amostras, sem alocação dinâmica;
- diferenças inferiores a 20 g não alteram `filteredWeightGrams`;
- mudanças superiores a 2.000 g geram `SIGNIFICANT_WEIGHT_CHANGE`;
- uma mudança significativa não troca, retira, finaliza nem exclui um KEG por conta própria;
- janela, deadband e limiar de mudança são configuráveis em `BuildConfig.h`.

## Cálculo de volume — Fase 10

```text
pesoCervejaG = max(0, pesoBrutoG - taraG)
volumeMl = pesoCervejaG * 1000 / densidadeGPorLitro
volumeExibidoMl = clamp(volumeMl, 0, capacidadeMl)
percentualBasisPoints = volumeExibidoMl * 10000 / capacidadeMl
```

- tara, densidade e capacidade são específicas de cada KEG;
- peso negativo ou acima de 100 kg é inválido e não altera o cadastro;
- capacidade aceita: 1 a 100 L;
- densidade aceita: 0,900 a 1,300 kg/L;
- peso igual ou abaixo da tara produz 0 L e o aviso `BELOW_TARE`;
- peso acima da capacidade mais 500 g de tolerância produz `ABOVE_EXPECTED_MAXIMUM`;
- volume fica limitado à capacidade e percentual a 100%, sem esconder o peso bruto;
- a identificação automática por NFC grava a medição calculada somente no KEG identificado;
- a pesagem manual mostra peso e volume antes de gravar e permite cancelar;
- a interface arredonda apenas a apresentação do percentual inteiro; o domínio mantém duas casas decimais.

## Presença, troca e retirada — Fase 9

- apenas um KEG pode estar em `ACTIVE_ON_SCALE`;
- ao reconhecer outro UID conhecido e peso estável, a troca A → B é atômica: A passa a `STORED` e B a `ACTIVE_ON_SCALE`;
- retirada forte: NFC ausente e peso abaixo de 1.000 g continuamente por 5 s;
- no modo híbrido da Fase 15.5, uma queda de 2.000 g sem NFC não remove o KEG: ela pausa o acompanhamento e exige confirmação;
- balança `STALE` ou `OFFLINE` coloca a presença em estado incerto, preservando o KEG ativo;
- o botão `RETIRAR`, na tela de detalhes, pede confirmação e permite substituição manual dos sinais;
- toda retirada automática ou manual é persistida; falha de gravação restaura o estado anterior;
- o serial informa `KEG_ACTIVATED`, `KEG_REMOVED_FROM_SCALE` e o motivo da decisão.

## Identificação híbrida — Fase 8 mantida

- o mesmo UID precisa aparecer em três mensagens consecutivas ou por 1,5 s antes de ser aceito;
- ausência, troca direta e leituras alternadas passam pelo mesmo debounce;
- UID conhecido calcula a medição e ativa atomicamente o KEG correspondente;
- UID desconhecido não altera nenhum KEG e oferece `CADASTRAR` ou `IGNORAR`;
- `CADASTRAR` abre o formulário de novo KEG com o UID já preenchido;
- o botão `PESAR`, no início da tela de detalhes, seleciona manualmente o KEG;
- a pesagem manual exige uma leitura nova, online e estável;
- peso e volume calculado são mostrados antes da gravação;
- a confirmação manual prevalece sobre uma identificação NFC divergente, mas a tela mostra o aviso;
- somente a confirmação grava o peso/volume e ativa o KEG; cancelamento não modifica o cadastro;
- a identificação por NFC e a pesagem manual alimentam a máquina de presença da Fase 9.

## Balança

- `ScalePacket`, `ScaleReading` e `ScaleState` usam memória fixa;
- `HttpScaleTransport` consulta `http://balanca.local/api/v1/reading` a cada 500 ms em uma máquina de estados cooperativa, com esperas de rede estritamente limitadas;
- `IScaleTransport` mantém o simulador selecionável sem alterar o domínio;
- `ScaleProtocol` aceita apenas versão 1 e `SCALE_MAIN`, normaliza o UID e rejeita peso, bateria e RSSI inválidos;
- `ScaleService` descarta sequências duplicadas/antigas e reconhece reinício do emissor pelo uptime;
- o peso publicável passa pela mediana de cinco amostras e pelo deadband de 20 g;
- em produção, `ONLINE` vale até 30 s e `OFFLINE` começa após 5 min;
- no simulador, os limites são reduzidos para 3 s e 7 s, permitindo observar o ciclo completo rapidamente;
- o cartão `BALANCA` em `CONFIG.` mostra conexão, último peso e presença de NFC.

O transporte HTTP é selecionado por `KEEZER_SCALE_USE_HTTP=1`. Os valores de
`include/NetworkConfig.h` são usados somente quando ainda não existe uma
configuração válida na NVS. Depois do primeiro salvamento em `CONFIG. > REDE`,
a configuração persistida passa a ter prioridade. Se o SSID inicial ficar
vazio, o ESP32 tenta reutilizar credenciais Wi-Fi já presentes no módulo.

## Catálogo de KEGs

- até 32 KEGs, com memória de tamanho previsível e sem `String` no domínio;
- ID, UID NFC, nome, estilo, lote, observações, capacidade, tara e densidade;
- valores persistidos em gramas e mililitros, evitando arredondamento de `float`;
- estados `AVAILABLE`, `ACTIVE_ON_SCALE`, `STORED`, `EMPTY`, `FINISHED`, `CLEANING` e `ARCHIVED`;
- criação, edição, exclusão autorizada, arquivamento, finalização e marcação de vazio;
- associação e remoção de tag NFC, com UID normalizado e exclusivo;
- ativação atômica: ao ativar um KEG, o anterior passa para armazenado;
- rollback em memória quando uma gravação falha.

## Persistência segura

- partição LittleFS interna de 320 KiB;
- slots alternados `/kegs_a.bin` e `/kegs_b.bin`;
- cabeçalho com versão, geração, quantidade e CRC32 do payload;
- o novo slot é fechado, relido e validado antes de ser aceito;
- no boot, vence a maior geração válida; um slot corrompido não invalida o outro;
- formatação automática ocorre somente na primeira inicialização conhecida;
- `MemoryKegRepository` disponível para testes isolados.

## Interface implementada

- cabeçalho fixo com Wi-Fi e horário simulados;
- HOME com `FreezerCard`, três `KegCard` reutilizáveis e botão `Novo Barril`;
- área central com scroll vertical pelo touchscreen;
- modal visual de novo barril;
- tela de detalhe para cada barril, com dados em grid e gráfico LVGL;
- placeholders de Temperatura e Configurações;
- menu inferior fixo com HOME, TEMPERATURA e CONFIG.;
- tema técnico/industrial preto e azul, com estados verde, âmbar e vermelho;
- ícone leve de barril desenhado por widgets, sem bitmap grande;
- dados DEMO acessados somente por `AppViewModel`;
- atualização incremental da HOME a cada 250 ms, sem redraw completo;
- estado vazio preparado para quando não houver KEGs cadastrados;
- cadastro e edição de nome, estilo, lote, data de envase, capacidade, tara, densidade, NFC e observações;
- teclado touchscreen adaptado ao tipo do campo e mensagens de validação;
- aviso e confirmação explícita antes de finalizar e apagar permanentemente;
- confirmação explícita antes de arquivar;
- KEGs arquivados ficam ocultos da lista sem serem apagados;
- estado do compressor com cores para resfriando, aguardando, parado e erro;
- telemetria serial de heap, FPS aproximado e duração média/máxima do `lv_timer_handler()`.

## Simulação opcional

O build padrão usa `KEEZER_DEMO_MODE=0`, DS18B20 e relé reais. Os componentes
de simulação permanecem no código apenas para testes deliberados sem carga.

Com a animação ativa:

- o KEG ativo perde 0,1 L a cada 5 segundos;
- volume, peso, percentual, SYNC e gráfico são recalculados;
- a presença alterna a cada 20 segundos: KEG 1, nenhum, KEG 2, nenhum;
- somente um KEG pode aparecer como `LENDO`.

Para congelar os valores sem desativar o modo demonstração, use:

```ini
-D KEEZER_DEMO_AUTOPLAY=0
```

Em um build de bancada explicitamente configurado com o sensor demonstrativo,
o simulador térmico executa um ciclo de 60 segundos:

1. começa em 2,70 °C e aguarda a proteção mínima desligado;
2. inicia `COOLING` após 10 segundos;
3. simula erro de leitura entre 13 e 17 segundos e desliga imediatamente;
4. recupera após três amostras válidas e volta a respeitar o min-off;
5. reduz para 1,40 °C aos 28 segundos e encerra o resfriamento;
6. retorna a 2,70 °C aos 45 segundos e inicia um novo ciclo.

Quando `KEEZER_SCALE_USE_HTTP=0`, o simulador da balança executa um ciclo de bancada de 75 segundos. Em cada período de envio ele adiciona pequenas oscilações e um pico isolado de 1,5 kg, que deve aparecer como peso bruto sem contaminar o peso filtrado:

1. KEG 1, UID `04A23F891C`, aproximadamente 18,55 kg;
2. intervalo sem comunicação;
3. KEG 2, UID `04B17D2210`, aproximadamente 10,65 kg;
4. novo intervalo sem comunicação;
5. balança vazia, sem UID, aproximadamente 0,45 kg;
6. intervalo e reinício do ciclo.

Cada cenário transmite por cerca de 12 segundos e fica silencioso até completar sua janela de 25 segundos. Isso permite observar troca, estados `STALE`/`OFFLINE` e retirada automática forte sem hardware NFC.

## Compilar

```powershell
platformio run -e esp32_2432s028r
```

Build HTTP/OTA da Fase 19.1 validado: 123.968 bytes de RAM estática (37,8%) e
1.521.773 bytes de flash (80,1%). Para acomodar a pilha
Wi-Fi, o pool LVGL usa 40 KB e o buffer de renderização usa 24 linhas.

## Gravar e monitorar

```powershell
platformio run -e esp32_2432s028r -t upload
platformio device monitor -e esp32_2432s028r
```

O monitor serial informa também mudanças da conexão da balança, UID NFC normalizado e pesos recebidos.

## Roteiro de teste — Fase 18

1. Grave o firmware, abra o monitor serial e confirme `Phase 18 initialized`.
   Nesta placa, `Display scanline synchronization: FALLBACK` é esperado.
2. Deixe a tela HOME parada por 90 s e copie três conjuntos consecutivos de
   `PERF_HEAP`, `PERF_UI`, `PERF LOOP` e `PERF SD`.
3. Navegue por HOME, detalhe do KEG, KEEZER e CONFIG.; role listas e use o touch.
   A interface deve acompanhar o dedo sem efeito cortina, retardo perceptível ou falhas
   visuais. Riscos, linhas ou cores incorretas indicam instabilidade no SPI.
4. Faça e confirme uma pesagem para provocar persistência; aguarde o próximo
   conjunto `PERF_*` e confirme `PERF SD ... failures=0 ... pending=0`.
5. Em repouso, `handler_calls` deve ficar na ordem de poucos milhares por 30 s,
   não milhões. O heap mínimo e a fragmentação não devem piorar continuamente
   entre as três janelas.
6. `slow_over_10000us` deve ser zero ou eventual em repouso. Uma operação de
   rede, display ou SD pode elevar o máximo pontualmente; crescimento contínuo
   exige investigação antes de qualquer nova otimização.

O relatório e a tabela para registrar os resultados estão em
`outputs/PHASE_18_REPORT.md`.

## Roteiro de teste — Fase 17

1. Grave o firmware e confirme `Phase 17 initialized` e
   `BOOT_RECOVERY ... outputs=SAFE_OFF` no serial.
2. Desligue o ponto de acesso por mais de 20 s. Rede e balança devem perder
   comunicação, mas touch, catálogo e controle térmico devem continuar ativos.
3. Religue o ponto de acesso e confirme `NETWORK_CONNECTED`, resolução da
   balança e retorno a `ONLINE`, sem reiniciar o controlador.
4. Desligue apenas a balança. O KEG ativo deve permanecer preservado e o
   acompanhamento passar para `PAUSADO`; religue-a e confirme a recuperação.
5. Sem NFC, retire a tag mantendo peso: a retirada não pode ser instantânea.
   Com uma tag desconhecida, confirme `UNKNOWN_NFC_TAG` e que nenhum KEG mudou.
6. Desconecte o DS18B20 com a carga de potência isolada. Deve aparecer
   `TEMPERATURE_SENSOR_ERROR ... output=OFF`; após reconectar e receber três
   amostras válidas, o controle deve recuperar respeitando a proteção.
7. Retire o SD, faça uma pesagem manual confirmada e abra `CONFIG.`. Em
   `SISTEMA`, confira `SD AUSENTE`/`SD ERRO` e `FILA 1` ou maior.
8. Recoloque o SD. Em até 15 s, confirme `STORAGE_RECOVERED`,
   `HISTORY_FLUSHED storage=SD`, `SD OK` e `FILA 0`.
9. Reinicie o controlador com o SD presente. Cadastro, último volume,
   configurações e histórico devem reaparecer; o serial deve informar a causa
   do reset e o catálogo carregado.
10. Tara inválida é testada pelo formulário: valores negativos ou fora da faixa
    devem ser recusados sem modificar o cadastro. Peso negativo enviado pela
    balança deve gerar `Reading rejected: WEIGHT_OUT_OF_RANGE`.

Os testes destrutivos de SD cheio e corrupção de ambos os snapshots não devem
ser feitos no cartão/catálogo principal. Use uma cópia de teste; o procedimento
controlado está em `outputs/PHASE_17_REPORT.md`.

## Roteiro de regressão — Fase 16

1. Grave o firmware e confirme no serial `Phase 16 initialized`.
2. Aguarde `NETWORK_CONNECTED` com SSID, IP e RSSI, seguido de
   `Scale resolved` e `Link state=ONLINE`.
3. Abra `CONFIG.` e confirme que o cartão `REDE` mostra `CONECTADA` e o RSSI.
4. Toque em `REDE`, confira SSID, hostname, endereço e porta da balança. A
   senha deve permanecer oculta e vazia para edição.
5. Sem alterar os dados, toque em `SALVAR`: deve aparecer
   `CONFIGURACAO SALVA - RECONECTANDO`, a rede deve reconectar e a balança
   retornar a `ONLINE` automaticamente.
6. Reinicie o controlador e confirme que a configuração salva foi preservada.
7. Desligue o ponto de acesso por mais de 20 s. A tela deve mostrar
   `RECONECTANDO`, sem travar touch, temperatura, catálogo ou histórico.
8. Ligue o ponto de acesso e confirme recuperação automática de rede e balança.
9. Informe temporariamente um host de balança inválido: o Wi-Fi deve continuar
   `CONECTADA`, mas somente a balança deve ficar offline. Restaure o endereço
   correto ao final.

## Teste de regressão — Fase 15.5

1. Configure em `include/NetworkConfig.h` a mesma rede usada pela balança e
   grave o controlador.
2. Reinicie e confirme `NETWORK_CONNECTED`, `Scale resolved` e
   `Link state=ONLINE` no serial.
3. Compare o peso mostrado no cartão `BALANCA`, em `CONFIG.`, com a página
   `http://balanca.local/`.
4. Sem NFC, coloque o KEG correto na balança, abra seus detalhes, toque
   `PESAR`, aguarde `PESO PRONTO` e confirme a pesagem. O estado deve passar
   para `MONITORANDO`.
5. Retire pelo menos 100 g de cerveja, recoloque o KEG e mantenha-o imóvel.
   Após 3 s estáveis e respeitado o intervalo de 30 s, confirme no serial
   `AUTO_CONSUMPTION_RECORDED` e confira a redução de volume na tela.
6. Retire menos de 100 g: não deve existir nova gravação. O valor acumula até
   a redução alcançar o limite.
7. Adicione 300 g ou troque por outro KEG sem NFC. A tela deve mostrar
   `VERIFICAR`/`CONFIRMAR KEG`, sem atribuir o peso automaticamente.
8. Confirme o mesmo KEG com `PESAR`, ou abra o detalhe do KEG correto e faça a
   pesagem manual. O acompanhamento deve retomar com a nova referência.
9. Desligue a balança: o estado deve virar `PAUSADO`, sem alterar catálogo ou
   histórico. Ao religar, o sistema reavalia o peso antes de retomar.
10. Com uma tag cadastrada, confirme UID estável, ativação do KEG e cálculo do
   volume. Uma troca de UID deve continuar selecionando o KEG correto.
11. Reinicie somente a balança e confirme `Scale sender restart detected`,
   seguido de novas leituras.
12. Reinicie o controlador com um KEG previamente ativo. Ele deve mostrar
   `VERIFICAR`, aguardando uma confirmação manual ou NFC antes de gravar novas
   variações.

Observação: após reiniciar, um UID que esteja fisicamente presente pode ativar novamente seu KEG depois do debounce. Isso é esperado e não significa que a retirada manual falhou.

Para forçar uma nova calibração do touch, adicione temporariamente a `build_flags`:

```ini
-D KEEZER_FORCE_TOUCH_CALIBRATION=1
```

## Limites desta fase

- Temperatura e acionamento do relé são reais; valide a instalação elétrica antes de conectar a carga.
- A fonte de balança padrão deste build é HTTP; a confirmação final depende das duas placas na mesma rede.
- O UID do PN532 físico chega pelo contrato HTTP da balança.
- Os limites de 100 g, 3 s, 30 s, 300 g, 1.000 g e 2.000 g são iniciais e deverão ser calibrados com uso real.
- Sem NFC, dois KEGs de peso parecido não podem ser distinguidos fisicamente; por isso toda troca exige seleção/confirmação manual.
- A velocidade de consumo e troca de KEG é acelerada apenas para teste visual.
- Sem NTP/RTC, os arquivos são identificados pelo boot e o gráfico mostra ordem relativa, não datas civis.
- O histórico mantido apenas no buffer RAM durante ausência/falha do cartão não sobrevive a um reinício.
- A fidelidade final de cores, espaçamento, fluidez e touch precisa ser confirmada na placa física.

A arquitetura está registrada em `outputs/ARCHITECTURE.md`, o contrato em
`outputs/SCALE_PROTOCOL.md`, a integração HTTP em `outputs/PHASE_15_REPORT.md`,
o acompanhamento automático em `outputs/PHASE_15_5_REPORT.md`, a camada de
rede em `outputs/PHASE_16_REPORT.md` e a robustez em
`outputs/PHASE_17_REPORT.md`.
