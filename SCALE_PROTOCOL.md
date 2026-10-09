# Protocolo da balança — HTTP v1

## Contrato

A balança é o servidor HTTP e o controlador ESP32 é o cliente. A consulta é:

```http
GET /api/v1/reading HTTP/1.1
Host: <IP-ou-host-da-balanca>
Accept: application/json
Connection: close
```

O controlador consulta a cada 500 ms, limita a resposta completa a 767 bytes e
abandona uma resposta após 1.000 ms. O host e a porta são configuráveis em
`CONFIG. > REDE`; IP fixo, DNS e nome `.local` são aceitos.

## Resposta 200

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

| Campo | Tipo | Validação no controlador |
|---|---|---|
| `protocol_version` | inteiro | obrigatório e igual a `1` |
| `sequence` | uint32 | sequência da amostra; repetidas/antigas são rejeitadas |
| `scale_id` | texto | obrigatório e igual a `SCALE_MAIN` |
| `weight_grams` | inteiro | 0 a 100.000 g |
| `stable` | booleano | somente `true` alimenta o filtro |
| `nfc_uid` | texto ou `null` | hexadecimal; `:`, `-` e espaços são removidos |
| `rssi_dbm` | inteiro | −128 a 0 dBm |
| `uptime_ms` | uint64 | queda do valor indica reinício da balança |

`nfc_uid: null` é uma resposta normal quando não há tag ou PN532. Ela não
invalida o peso e mantém disponível o fluxo manual `DETALHE DO KEG > PESAR`.

## Sem leitura disponível

`503 Service Unavailable` significa que a balança respondeu, mas o HX711 ainda
não tem medição utilizável. Outros códigos, JSON incompleto, valor fora da
faixa, resposta grande ou timeout são rejeitados.

## Estados e filtragem

- `ONLINE`: houve leitura aceita nos últimos 30 s;
- `STALE`: mais de 30 s sem leitura aceita;
- `OFFLINE`: mais de 300 s sem leitura aceita;
- mediana de cinco amostras estáveis;
- deadband de 20 g;
- mudança de 2.000 g ou mais gera evento significativo, sem trocar KEG sozinha.

Após três falhas HTTP consecutivas, o endereço é resolvido novamente. O Wi-Fi
usa reconexão progressiva sem interromper a interface.

## Segurança e compatibilidade

A API v1 não possui autenticação; mantenha-a somente em uma LAN confiável e não
exponha a porta da balança à internet. Para testes sem balança real, compile com
`KEEZER_SCALE_USE_HTTP=0`; as regras do domínio permanecem iguais.

## Teste de integração

1. confirme controlador e balança na mesma rede;
2. veja `Scale resolved` e `Link state=ONLINE` no serial;
3. compare o peso da página da balança com `CONFIG. > BALANCA`;
4. teste `nfc_uid=null` e o botão `PESAR`;
5. teste UID conhecido, UID desconhecido e troca de tags;
6. reinicie e desligue a balança para verificar recuperação automática.

O relatório histórico da integração permanece em
`outputs/SCALE_PROTOCOL.md`.
