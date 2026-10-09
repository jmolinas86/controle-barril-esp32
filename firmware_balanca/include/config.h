#pragma once

// Credenciais opcionais para o primeiro boot. Se ficarem vazias, a balanca
// abre o portal cativo "BC Balanca" em http://192.168.4.1/.
#define WIFI_SSID       ""
#define WIFI_PASSWORD   ""

#define DEVICE_NAME     "balanca1"
#define DEVICE_HOSTNAME "balanca"
#define SCALE_ID        "SCALE_MAIN"

// Troque estas credenciais antes de instalar fora de uma rede confiavel.
#define OTA_USERNAME    "admin"
#define OTA_PASSWORD    "balanca-update"

// HX711: determine este valor pelo procedimento de calibracao no README.
// O sinal pode precisar ser negativo, conforme a montagem das celulas.
#define HX711_CALIBRATION_FACTOR  -21000.0f
#define HX711_READ_SAMPLES        5     // janela da media movel (nao bloqueia por 5 leituras)
#define WEIGHT_DEADBAND           0.010f  // kg: ignora variacoes menores que 10 g
#define CALIBRATION_STABILITY_KG   0.030f  // variacao maxima para aceitar calibracao
#define ZERO_TRACK_THRESHOLD_KG    0.050f  // apresenta zero para residuo de ate 50 g apos tara

// Limite de seguranca da instalacao, nao e a capacidade presumida da balanca.
#define MAX_ABSOLUTE_WEIGHT_KG    300.0f
#define MIN_VALID_WEIGHT_KG       -5.0f

#define NFC_INTERVAL_MS           150UL
#define DISPLAY_INTERVAL_MS       500UL
#define NFC_REMOVE_TIMEOUT_MS     800UL
#define HX711_STALE_TIMEOUT_MS    1500UL
#define WIFI_CONNECT_TIMEOUT_MS   20000UL
#define WIFI_RETRY_INTERVAL_MS    10000UL
