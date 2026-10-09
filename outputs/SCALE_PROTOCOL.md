# Protocolo da balança — HTTP v1

## Objetivo

Este documento é o contrato entre a balança ESP8266 e o controlador do
keezer ESP32. A balança produz uma fotografia atômica da medição; o
controlador apenas consulta, valida e entrega um `ScalePacket` ao
`ScaleService`. `KegService` não conhece HTTP, JSON ou Wi-Fi.

## Transporte

- rede: Wi-Fi local confiável, sem dependência de internet;
- servidor: balança;
- cliente: controlador;
- endereço padrão: `http://balanca.local`;
- endereço configurado nesta instalação: `http://10.0.0.188`;
- consulta: `GET /api/v1/reading` a cada 500 ms;
- timeout de resposta no controlador: 1.000 ms;
- limite do controlador: 767 bytes para cabeçalhos e corpo completos;
- conexão HTTP fechada após cada resposta.

Quando configurado com `balanca.local`, o controlador resolve o nome por mDNS,
armazena o IP em cache e refaz a resolução após falhas consecutivas. Nesta
instalação ele usa diretamente `10.0.0.188`, definido em
`include/NetworkConfig.h`.

## Requisição

```http
GET /api/v1/reading HTTP/1.1
Host: balanca.local
Accept: application/json
Connection: close
```

## Resposta válida

Código HTTP `200`, com todos os campos abaixo:

```json
{
  "protocol_version": 1,
  "sequence": 1842,
  "scale_id": "SCALE_MAIN",
  "weight_grams": 18540,
  "stable": true,
  "nfc_uid": "04A23F891C",
  "rssi_dbm": -58,
  "uptime_ms": 928441
}
```

| Campo | Tipo | Regra |
|---|---|---|
| `protocol_version` | inteiro sem sinal | obrigatório e igual a `1` |
| `sequence` | inteiro sem sinal de 32 bits | monotônico por amostra válida |
| `scale_id` | string | obrigatório e igual a `SCALE_MAIN` |
| `weight_grams` | inteiro | gramas; controlador aceita de 0 a 100.000 g |
| `stable` | booleano | somente `true` entra no filtro de peso |
| `nfc_uid` | string ou `null` | hexadecimal; separadores são normalizados |
| `rssi_dbm` | inteiro | faixa de `-128` a `0` dBm |
| `uptime_ms` | inteiro sem sinal | relógio monotônico da balança |

Sem tag, ou quando o PN532 não está instalado, `nfc_uid` deve ser `null`.
Isso é operação normal do modo híbrido e não invalida peso, estabilidade ou
pesagem manual.

## Respostas sem medição

- `503 Service Unavailable`: o servidor está acessível, mas o HX711 ainda não
  possui uma leitura válida. O controlador não cria `ScalePacket` e conserva
  o último dado aceito até os timeouts de `STALE` e `OFFLINE`.
- outros códigos HTTP, JSON incompleto, corpo excedido ou timeout: resposta
  rejeitada e registrada no diagnóstico.

## Validação e idempotência

1. `HttpScaleTransport` aceita apenas uma resposta completa e bem formada.
2. `ScaleHttpJson` converte o JSON para estrutura de tamanho fixo.
3. `ScaleProtocol` valida versão, identidade, peso, RSSI e UID.
4. `ScaleService` rejeita sequência repetida ou antiga.
5. Se `uptime_ms` diminuir, o controlador reconhece reinício da balança e
   permite que a sequência recomece.
6. Somente depois dessas etapas NFC, filtro, presença, volume e histórico são
   atualizados.

O controlador marca a balança `ONLINE` após uma leitura aceita, `STALE` após
30 segundos sem leitura aceita e `OFFLINE` após 5 minutos. Esses estados não
apagam nem trocam um KEG automaticamente.

## Segurança

A API v1 instalada não possui autenticação. Ela deve permanecer em uma LAN
confiável e a porta 80 da balança não deve ser exposta à internet. Senha Wi-Fi
não aparece em logs nem no ViewModel. Autenticação poderá ser acrescentada em
uma nova versão do contrato, sem acoplar o domínio ao transporte.

## Compatibilidade

O firmware do controlador mantém `IScaleTransport`. Para retornar ao
simulador em bancada, compile com `KEEZER_SCALE_USE_HTTP=0`; nenhuma regra de
KEG, cálculo ou interface precisa ser alterada.

## Teste de aceitação

1. Colocar balança e controlador na mesma rede.
2. Confirmar no serial `Wi-Fi connected` e `Scale resolved`.
3. Confirmar `Link state=ONLINE` e pesos reais recebidos.
4. Verificar o mesmo peso na página da balança e no cartão `BALANCA` em
   `CONFIG.`.
5. Aproximar uma tag cadastrada e confirmar identificação/ativação do KEG.
6. Retirar a tag e confirmar operação sem NFC; o botão `PESAR` deve continuar
   funcionando.
7. Reiniciar a balança e confirmar `Scale sender restart detected`, seguido de
   novas leituras.
8. Desligar a balança, confirmar perda da leitura sem travar a tela, religar e
   confirmar recuperação automática.
