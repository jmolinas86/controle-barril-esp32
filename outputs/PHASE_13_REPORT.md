# Relatório da Fase 13 — Interface de temperatura

## Resultado

A rota `KEEZER` agora possui uma tela térmica completa e incremental. Ela apresenta o estado produzido pelo `TemperatureService`, permite preparar e salvar um novo setpoint e nunca acessa a saída do compressor diretamente.

## Interface entregue

- temperatura atual e setpoint em destaque;
- rótulo compacto `TEMP ATUAL`, valor centralizado e fontes ampliadas;
- botões `−` e `+` com passo de 0,1 °C;
- faixa operacional de −5,0 a 15,0 °C;
- estado pendente em laranja antes da gravação;
- confirmação `SETPOINT SALVO` ou erro destacado;
- estados localizados do compressor e indicação ON/OFF;
- estado ativo exibido como `COOLING`, mantendo distância da indicação lógica;
- tempo restante da proteção anti-ciclo;
- demanda de resfriamento;
- conexão, qualidade e falha do sensor;
- atualização incremental a cada revisão do ViewModel.

## Persistência

O `TemperatureSettingsStore` grava apenas a confirmação do usuário no namespace NVS `temp_cfg`. No boot, um valor válido substitui o padrão de 2,00 °C. Valor ausente usa o padrão; valor corrompido ou fora da faixa é rejeitado.

## Segurança

- a UI envia o pedido ao `AppViewModel`;
- o valor é validado antes da gravação;
- falha de NVS não altera o controlador;
- falha do serviço restaura o valor persistido anterior;
- a mudança de setpoint não zera os tempos mínimos;
- `NullCompressorOutput` continua sendo a saída térmica;
- `ON` significa comando lógico de simulação, não relé energizado.

## Memória e build

Os buffers parciais RGB565 foram ajustados para dois blocos de 240 × 16 pixels. A resolução continua 240 × 320 e o modo continua sendo renderização parcial dupla.

- ambiente: `esp32_2432s028r`;
- resultado: sucesso, sem warnings do código do projeto;
- RAM: 120.428 de 327.680 bytes (36,8%);
- flash: 949.133 de 1.900.544 bytes (49,9%);
- SHA-256: `319749677180C6D5CE46A4E652E9D421D6C0A7D9968B2C46871A4B391C00C608`.

## Teste na placa

1. Abrir `KEEZER` e verificar todos os estados visuais.
2. Alterar 2,0 para 2,1 °C e salvar.
3. Confirmar `SETPOINT_SAVED value=2.10C` no serial.
4. Reiniciar e confirmar `Setpoint loaded: 2.10C`.
5. Verificar que o cartão da HOME também mostra 2,1 °C.
6. Observar `AGUARDANDO`, a contagem de proteção, `RESFRIANDO` e a falha simulada.

## Limites

- sensor e compressor permanecem simulados;
- a polaridade e o GPIO do relé ainda exigem validação elétrica;
- o teste visual e de touch precisa ser confirmado na placa física.
