# Fase 19.3 — Consulta adaptativa da balança

## Implementado

- Consulta HTTP rápida a cada 500 ms durante atividade ou instabilidade.
- Permanência no modo rápido por 10 s depois da última alteração de pelo menos
  20 g.
- Consulta a cada 2 s com KEG presente e peso estável.
- Consulta a cada 5 s com plataforma vazia/ociosa, usando 1 kg como limite.
- Mudança automática de perfil sem alterar a API HTTP da balança.
- Telemetria `POLL_MODE mode=FAST|STABLE|IDLE period=...` no serial.

## Compatibilidade

O firmware da balança não precisa ser atualizado nesta etapa. Ela continua
respondendo a `GET /api/v1/reading`; somente a frequência das requisições feitas
pelo controlador mudou.

## Validação recomendada

1. No boot, observar o modo rápido de 500 ms.
2. Manter um KEG parado por mais de 10 s e observar `STABLE`/2.000 ms.
3. Retirar mais de 20 g e confirmar retorno para `FAST`/500 ms.
4. Esvaziar a plataforma, aguardar estabilização e observar `IDLE`/5.000 ms.
5. Recolocar um KEG; a detecção pode levar até 5 s no estado ocioso e depois
   deve voltar ao modo rápido.

## Build

- Ambiente: `esp32_2432s028r`.
- Resultado: aprovado pelo PlatformIO.
- RAM estática: 124.568 bytes (38,0%).
- Flash: 1.529.137 bytes (80,5%).
