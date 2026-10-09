# Fase 18 — Otimização medida

## Resultado do firmware

A fase adiciona instrumentação de desempenho com baixo volume de logs e aplica
uma primeira otimização baseada em uma medição real. A telemetria anterior
registrou aproximadamente 1,4 milhão de chamadas ao handler LVGL em 30 segundos,
embora o conteúdo visível variasse por volta de 1,6 FPS.

O `UIManager` agora respeita o próximo prazo indicado pelo LVGL e limita a espera
a 5 ms. Isso elimina chamadas inúteis sem sacrificar a leitura do touch.

A rolagem com retardo observada no hardware justificou o segundo ajuste medido:
touch a cada 10 ms, refresh a cada 20 ms, início do arraste após 5 pixels e SPI
do ST7789 a 60 MHz. O vídeo de bancada permitiu separar retardo de *screen
tearing*. A primeira correção usa a leitura `GET_SCANLINE` do ST7789 como
referência de varredura, DMA, saída `RGB565_SWAPPED` e dois buffers parciais de
12 linhas. A tentativa anterior de 16 linhas foi descartada porque excedeu a
região DRAM em 2.400 bytes; 12 linhas compila com margem. Atualizações apenas
visuais de dados são adiadas durante o gesto. Nenhuma aquisição ou controle é
interrompido e não houve redução de resolução ou qualidade gráfica.

No boot, `Display scanline synchronization: AVAILABLE` confirma a estratégia
principal. `FALLBACK` informa que o painel não respondeu à leitura e que o DMA
continua ativo sem sincronização. Três falhas consecutivas em execução geram
`Display scanline synchronization lost; using DMA fallback`.

O controlador real respondeu `FALLBACK` e o DMA continua ativo sem sincronização
de linha; este é o ponto de referência restaurado para novos testes isolados.

### Teste isolado — buffer DMA

Após 80 MHz produzir melhora leve sem artefatos relatados, a organização dos
11.520 bytes de renderização foi alterada de dois buffers de 240 × 12 pixels
para um buffer de 240 × 24 pixels. O objetivo é reduzir a quantidade de faixas
e transações sem aumentar a RAM. O primeiro ensaio quase eliminou o tearing,
mas revelou faixas pretas porque o LVGL reutilizava o buffer antes do fim do
DMA. O firmware agora aguarda cada transferência antes de liberar esse buffer.
A aceitação depende de novo ensaio físico.

## Telemetria disponível

Os quatro registros são publicados a cada 30 segundos:

```text
PERF_HEAP free=... minimum=... largest=... fragmentation=...%
PERF_UI fps=... handler_calls=... avg=...us max=...us flush_calls=... flush_avg=...us flush_max=...us pixels=...
PERF LOOP calls=... avg=...us max=...us slow_over_10000us=...
PERF SD writes=... reads=... failures=... bytes_write=... bytes_read=... avg=...us max=...us pending=...
```

`free`, `minimum` e `largest` estão em bytes; tempos estão em microssegundos. A
fragmentação representa a parcela do heap livre que não pertence ao maior bloco
contíguo. `pending` é a fila de histórico aguardando retorno do cartão.

## Validação no controlador

1. Grave o firmware e confirme `Phase 18 initialized`.
2. Deixe HOME em repouso por 90 segundos e guarde três janelas consecutivas.
3. Navegue, role e abra todas as telas principais durante mais uma janela.
4. Confirme uma pesagem para provocar escrita no SD e guarde a janela seguinte.
5. Confirme no boot `Display scanline synchronization: FALLBACK`.
6. Verifique se a lista acompanha o dedo sem corte horizontal, se a inércia
   termina suavemente, se as cores continuam corretas e se não existem riscos.
7. Confirme que temperatura, peso e status se atualizam após soltar a tela e
   que `PERF SD failures=0`.

Tabela para preencher com os dados reais:

| Cenário | Heap livre/mínimo | Fragmentação | Handler calls | Loop máx./lentos | Flush máx. | SD falhas/máx. |
|---|---:|---:|---:|---:|---:|---:|
| HOME 30 s | — | — | — | — | — | — |
| HOME 60 s | — | — | — | — | — | — |
| HOME 90 s | — | — | — | — | — | — |
| Navegação | — | — | — | — | — | — |
| Pesagem/SD | — | — | — | — | — | — |

Critério inicial: chamadas do handler na ordem de poucos milhares por 30 s, heap
e fragmentação sem degradação contínua, loops lentos ausentes ou eventuais em
repouso, SD sem falhas e fila zerada. Os números reais decidirão se existe um
segundo ponto que justifique otimização.

## Compilação

Ambiente `esp32_2432s028r` compilado com sucesso:

- RAM estática: 123.144 / 327.680 bytes (37,6%);
- flash: 1.477.837 / 1.900.544 bytes (77,8%);
- `firmware.bin`: 1.484.416 bytes;
- SHA-256: `9E0C6058A606D02FF6B22315D7588F6858D727CEEA09B85F3355B24687F0E8F5`.

A aceitação funcional da Fase 18 depende da coleta das janelas acima no
controlador real.
