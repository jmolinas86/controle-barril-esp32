# Relatório da Fase 16 — Rede

## Resultado

A conexão Wi-Fi foi retirada do `HttpScaleTransport` e centralizada no
`NetworkService`. A rede agora possui ciclo de vida próprio, estado observável,
RSSI, reconexão automática e configuração persistente. A balança continua
usando HTTP pull, mas uma falha nela não é confundida com falha do Wi-Fi.

## Arquitetura entregue

- `models/Network`: configurações, estado e enumeração de conexão;
- `INetworkAdapter`: porta independente de Arduino;
- `Esp32WiFiAdapter`: implementação física para o ESP32;
- `NetworkService`: máquina de conexão e backoff;
- `NetworkSettingsStore`: persistência NVS versionada com CRC32;
- `NetworkViewData`: snapshot sem senha para a interface;
- `NetworkSettingsScreen`: edição touchscreen da rede e endpoint da balança;
- `HttpScaleTransport`: consumidor do serviço de rede, sem `WiFi.begin()`.

## Máquina de conexão

1. `CONNECTING`: inicia uma tentativa assíncrona.
2. Se conectar, publica IP, SSID real e RSSI em `CONNECTED`.
3. Se não conectar em 20 s, encerra a tentativa e entra em `RECONNECTING`.
4. Repete depois de 5 s, dobrando a espera até o máximo de 60 s.
5. Ao recuperar a rede, zera o backoff e libera novamente o transporte HTTP.

O loop permanece cooperativo. A espera entre tentativas não usa `delay()` e
não bloqueia LVGL, controle térmico, armazenamento ou acompanhamento do KEG.

## Configuração touchscreen

Em `CONFIG. > REDE` estão disponíveis:

- nome da rede (SSID);
- nova senha Wi-Fi;
- hostname do controlador;
- IP ou hostname da balança;
- porta HTTP da balança.

O campo de senha começa vazio e mascarado. Vazio significa conservar a senha
atual; uma senha digitada substitui a anterior. A senha persistida fica no
namespace NVS `network_cfg`, nunca entra no `NetworkViewData` e nunca aparece
nos logs.

As configurações de compilação continuam como fallback de primeiro boot.
Depois de salvar pela tela, o bloco NVS válido passa a ter prioridade.

## Diagnóstico

O serial publica transições sem revelar a senha:

```text
NETWORK_STATE status=CONECTANDO retry_ms=0
NETWORK_CONNECTED ssid=<rede> ip=<endereco> rssi=<valor>dBm
NETWORK_SETTINGS_SAVED ssid=<rede> host=<balanca> port=<porta>
```

O cartão `REDE` mostra estado e RSSI. O cabeçalho colore o indicador Wi-Fi de
acordo com o estado. O cartão `BALANCA` continua mostrando o estado do servidor
de pesagem separadamente.

## Validação de software

- build PlatformIO `esp32_2432s028r`: sucesso;
- RAM estática: 119.160 de 327.680 bytes (36,4%);
- flash: 1.473.209 de 1.900.544 bytes (77,5%);
- binário: 1.479.792 bytes;
- SHA-256: `45339BC472268583A20DA78536834877419CB85E61A437C6A714D8DC9B255E0E`;
- `test/network_service_smoke.cpp` cobre conexão, perda, timeout, backoff,
  recuperação, troca de endpoint e rejeição de porta inválida;
- o smoke test nativo não foi executado porque não existe compilador C++ de
  desktop instalado nesta máquina.

## Critério de aprovação física

A fase será aprovada quando a placa confirmar conexão inicial, leitura correta
de IP/RSSI, salvamento e restauração após reboot, reconexão depois da perda do
ponto de acesso e independência entre o estado do Wi-Fi e o estado da balança.

## Limites

- não existe portal cativo nesta fase;
- não existe interface web administrativa completa;
- não há busca visual de redes próximas; o SSID é digitado;
- rede Wi-Fi aberta previamente configurada continua suportada, mas a tela
  atual trata senha vazia como “manter a senha existente”;
- autenticação da API da balança continua adiada; a instalação deve permanecer
  em uma LAN confiável.
