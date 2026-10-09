#include "web/WebPortal.h"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Updater.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "config.h"
#include "web/WebAssets.h"

namespace balanca::web {

WebPortal::WebPortal(drivers::ScaleHardware& hardware,
                     network::NetworkManager& network,
                     storage::SettingsStore& store,
                     storage::DeviceSettings& settings)
    : hardware_(hardware),
      network_(network),
      store_(store),
      settings_(settings) {}

void WebPortal::begin() {
  server_.on("/", HTTP_GET, [this]() {
    server_.sendHeader("Cache-Control", "no-store");
    server_.send_P(200, "text/html; charset=utf-8", kIndexHtml);
  });
  server_.on("/api/v1/reading", HTTP_GET, [this]() { handleReading(); });
  server_.on("/api/v1/status", HTTP_GET, [this]() { handleStatus(); });
  server_.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
  server_.on("/api/v1/wifi/scan", HTTP_GET, [this]() { handleWifiScan(); });
  server_.on("/api/v1/config/wifi", HTTP_POST,
             [this]() { handleWifiSave(); });
  server_.on("/wifi/save", HTTP_POST, [this]() { handleWifiSave(); });
  server_.on("/api/v1/actions/tare", HTTP_POST, [this]() { handleTare(); });
  server_.on("/calibration/tare", HTTP_POST, [this]() { handleTare(); });
  server_.on("/api/v1/actions/calibrate", HTTP_POST,
             [this]() { handleCalibrate(); });
  server_.on("/calibration/known-weight", HTTP_POST,
             [this]() { handleCalibrate(); });
  server_.on("/api/v1/actions/restart", HTTP_POST,
             [this]() { handleRestart(); });
  server_.on("/api/v1/actions/factory-reset", HTTP_POST,
             [this]() { handleFactoryReset(); });
  server_.on("/factory-reset", HTTP_POST,
             [this]() { handleFactoryReset(); });
  server_.on("/update", HTTP_GET, [this]() { handleOtaPage(); });
  server_.on("/update", HTTP_POST, [this]() { handleOtaFinished(); },
             [this]() { handleOtaUpload(); });
  server_.onNotFound([this]() { handleNotFound(); });
  server_.begin();
  Serial.printf("[HTTP] Ready on port 80 - /api/v1/reading\n");
}

void WebPortal::handleOtaPage() {
  if (!server_.authenticate(OTA_USERNAME, OTA_PASSWORD)) {
    server_.requestAuthentication();
    return;
  }
  server_.sendHeader("Cache-Control", "no-store");
  server_.send_P(200, "text/html; charset=utf-8", kOtaHtml);
}

void WebPortal::handleOtaFinished() {
  if (!otaUploadAuthorized_) {
    server_.requestAuthentication();
    return;
  }
  server_.sendHeader("Connection", "close");
  server_.sendHeader("Cache-Control", "no-store");
  if (!otaUploadSucceeded_) {
    server_.send(500, "text/plain; charset=utf-8",
                 "Atualizacao rejeitada. O firmware anterior foi mantido.");
    otaUploadAuthorized_ = false;
    otaUploadValid_ = false;
    return;
  }
  server_.send(200, "text/plain; charset=utf-8",
               "Atualizacao concluida. A balanca sera reiniciada.");
  otaUploadAuthorized_ = false;
  otaUploadValid_ = false;
  rebootPending_ = true;
  rebootAtMs_ = millis();
}

void WebPortal::handleOtaUpload() {
  HTTPUpload& upload = server_.upload();
  if (upload.status == UPLOAD_FILE_START) {
    otaUploadAuthorized_ =
        server_.authenticate(OTA_USERNAME, OTA_PASSWORD);
    otaUploadValid_ = otaUploadAuthorized_ &&
                      upload.filename.endsWith(".bin") &&
                      !upload.filename.endsWith(".spiffs.bin") &&
                      !upload.filename.endsWith(".littlefs.bin");
    otaUploadSucceeded_ = false;
    if (!otaUploadAuthorized_) return;
    if (!otaUploadValid_) {
      Serial.println(F("[OTA] Rejected: select an application .bin"));
      return;
    }
    const std::uint32_t maximumSketchSpace =
        (ESP.getFreeSketchSpace() - 0x1000U) & 0xFFFFF000U;
    otaUploadValid_ = Update.begin(maximumSketchSpace, U_FLASH);
    if (!otaUploadValid_) Update.printError(Serial);
    Serial.printf("[OTA] Upload started: %s\n", upload.filename.c_str());
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (!otaUploadAuthorized_ || !otaUploadValid_) return;
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      otaUploadValid_ = false;
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (!otaUploadAuthorized_ || !otaUploadValid_) return;
    otaUploadSucceeded_ = Update.end(true);
    if (otaUploadSucceeded_) {
      Serial.printf("[OTA] Success: %lu bytes\n",
                    static_cast<unsigned long>(upload.totalSize));
    } else {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    // ESP8266 Updater cancels and clears an incomplete update with end(false).
    Update.end(false);
    otaUploadValid_ = false;
    otaUploadSucceeded_ = false;
    Serial.println(F("[OTA] Upload aborted"));
  }
}

void WebPortal::update(const std::uint32_t nowMs) {
  server_.handleClient();
  if (rebootPending_ && nowMs - rebootAtMs_ >= 1000U) ESP.restart();
}

void WebPortal::handleReading() {
  const model::ScaleSnapshot& reading = hardware_.snapshot();
  if (!reading.weightValid) {
    server_.sendHeader("Cache-Control", "no-store");
    server_.send(503, "application/json",
                 "{\"error\":\"WEIGHT_UNAVAILABLE\"}");
    return;
  }
  char json[384]{};
  char nfc[40]{};
  if (reading.hasNfcUid) {
    std::snprintf(nfc, sizeof(nfc), "\"%s\"", reading.nfcUid);
  } else {
    std::snprintf(nfc, sizeof(nfc), "null");
  }
  std::snprintf(
      json, sizeof(json),
      "{\"protocol_version\":1,\"sequence\":%lu,\"scale_id\":\"%s\","
      "\"weight_grams\":%ld,\"stable\":%s,\"nfc_uid\":%s,"
      "\"rssi_dbm\":%ld,\"uptime_ms\":%lu}",
      static_cast<unsigned long>(reading.sequence), SCALE_ID,
      static_cast<long>(reading.weightGrams), reading.stable ? "true" : "false",
      nfc, static_cast<long>(network_.rssiDbm()),
      static_cast<unsigned long>(millis()));
  server_.sendHeader("Cache-Control", "no-store");
  server_.send(200, "application/json", json);
}

void WebPortal::handleStatus() {
  const model::ScaleSnapshot& reading = hardware_.snapshot();
  char escapedSsid[96]{};
  escapeJson(network_.ssid(), escapedSsid, sizeof(escapedSsid));
  char nfc[40]{};
  if (reading.hasNfcUid) {
    std::snprintf(nfc, sizeof(nfc), "\"%s\"", reading.nfcUid);
  } else {
    std::snprintf(nfc, sizeof(nfc), "null");
  }
  const IPAddress address = network_.ip();
  char json[640]{};
  std::snprintf(
      json, sizeof(json),
      "{\"device_name\":\"%s\",\"scale_id\":\"%s\","
      "\"sequence\":%lu,\"weight_grams\":%ld,\"weight_valid\":%s,"
      "\"stable\":%s,\"nfc_uid\":%s,\"wifi_connected\":%s,"
      "\"provisioning\":%s,\"wifi_ssid\":\"%s\","
      "\"rssi_dbm\":%ld,\"ip\":\"%u.%u.%u.%u\","
      "\"hx711_ok\":%s,\"pn532_ok\":%s,\"oled_ok\":%s,"
      "\"calibration_factor\":%.4f,\"free_heap\":%lu,\"uptime_ms\":%lu}",
      DEVICE_NAME, SCALE_ID, static_cast<unsigned long>(reading.sequence),
      static_cast<long>(reading.weightGrams), reading.weightValid ? "true" : "false",
      reading.stable ? "true" : "false", nfc,
      network_.connected() ? "true" : "false",
      network_.provisioning() ? "true" : "false", escapedSsid,
      static_cast<long>(network_.rssiDbm()), address[0], address[1], address[2],
      address[3], hardware_.hx711Healthy() ? "true" : "false",
      hardware_.pn532Healthy() ? "true" : "false",
      hardware_.oledHealthy() ? "true" : "false",
      hardware_.calibrationFactor(), static_cast<unsigned long>(ESP.getFreeHeap()),
      static_cast<unsigned long>(millis()));
  server_.sendHeader("Cache-Control", "no-store");
  server_.send(200, "application/json", json);
}

void WebPortal::handleWifiScan() {
  const int count = WiFi.scanNetworks();
  String json(F("{\"networks\":["));
  json.reserve(64U + (count > 0 ? static_cast<unsigned>(count) * 64U : 0U));
  for (int index = 0; index < count; ++index) {
    char escaped[96]{};
    escapeJson(WiFi.SSID(index).c_str(), escaped, sizeof(escaped));
    if (index != 0) json += ',';
    json += F("{\"ssid\":\"");
    json += escaped;
    json += F("\",\"rssi_dbm\":");
    json += WiFi.RSSI(index);
    json += '}';
  }
  WiFi.scanDelete();
  json += F("]}");
  server_.send(200, "application/json", json);
}

void WebPortal::handleWifiSave() {
  if (!server_.hasArg("ssid") || server_.arg("ssid").isEmpty() ||
      server_.arg("ssid").length() > 32U ||
      server_.arg("password").length() > 64U) {
    sendResult(400, false, "SSID ou senha invalido.");
    return;
  }
  const String requestedSsid = server_.arg("ssid");
  const String requestedPassword = server_.arg("password");
  const bool keepExistingPassword =
      requestedPassword.isEmpty() && requestedSsid == settings_.wifiSsid;
  std::snprintf(settings_.wifiSsid, sizeof(settings_.wifiSsid), "%s",
                requestedSsid.c_str());
  if (!keepExistingPassword) {
    std::snprintf(settings_.wifiPassword, sizeof(settings_.wifiPassword), "%s",
                  requestedPassword.c_str());
  }
  if (!store_.save(settings_)) {
    sendResult(500, false, "Nao foi possivel salvar a configuracao.");
    return;
  }
  network_.applyCredentials(settings_.wifiSsid, settings_.wifiPassword, millis());
  sendResult(200, true, "Rede salva. Tentando conectar.");
}

void WebPortal::handleTare() {
  if (!hardware_.tare(settings_)) {
    sendResult(503, false, "HX711 indisponivel para tara.");
    return;
  }
  if (!store_.save(settings_)) {
    sendResult(500, false, "Tara feita, mas nao foi possivel salvar.");
    return;
  }
  sendResult(200, true, "Tara concluida e salva.");
}

void WebPortal::handleCalibrate() {
  std::int32_t referenceGrams = 0;
  if (server_.hasArg("reference_grams")) {
    referenceGrams = server_.arg("reference_grams").toInt();
  } else if (server_.hasArg("reference_kg")) {
    referenceGrams = static_cast<std::int32_t>(
        std::lround(server_.arg("reference_kg").toFloat() * 1000.0F));
  }
  if (referenceGrams <= 0 ||
      referenceGrams > static_cast<std::int32_t>(MAX_ABSOLUTE_WEIGHT_KG * 1000.0F)) {
    sendResult(400, false, "Massa de referencia invalida.");
    return;
  }
  if (!hardware_.calibrate(referenceGrams, settings_)) {
    sendResult(409, false,
               "Aguarde uma leitura estavel antes de calibrar.");
    return;
  }
  if (!store_.save(settings_)) {
    sendResult(500, false, "Calibracao feita, mas nao foi possivel salvar.");
    return;
  }
  sendResult(200, true, "Calibracao concluida e salva.");
}

void WebPortal::handleRestart() {
  sendResult(200, true, "Reiniciando a balanca...");
  rebootPending_ = true;
  rebootAtMs_ = millis();
}

void WebPortal::handleFactoryReset() {
  if (!store_.factoryReset()) {
    sendResult(500, false, "Nao foi possivel apagar a configuracao.");
    return;
  }
  sendResult(200, true, "Configuracao apagada. Reiniciando...");
  rebootPending_ = true;
  rebootAtMs_ = millis();
}

void WebPortal::handleNotFound() {
  if (network_.provisioning()) {
    server_.sendHeader("Location", "/");
    server_.send(302, "text/plain", "Configure o Wi-Fi");
    return;
  }
  server_.send(404, "application/json", "{\"error\":\"NOT_FOUND\"}");
}

void WebPortal::sendResult(const int status, const bool ok,
                           const char* const message) {
  char escaped[220]{};
  escapeJson(message, escaped, sizeof(escaped));
  char json[280]{};
  std::snprintf(json, sizeof(json), "{\"ok\":%s,\"message\":\"%s\"}",
                ok ? "true" : "false", escaped);
  server_.send(status, "application/json", json);
}

void WebPortal::escapeJson(const char* const source, char* const destination,
                           const std::size_t destinationSize) {
  if (destinationSize == 0U) return;
  std::size_t output = 0U;
  for (std::size_t input = 0U; source != nullptr && source[input] != '\0'; ++input) {
    const char value = source[input];
    if ((value == '\\' || value == '\"') && output + 2U < destinationSize) {
      destination[output++] = '\\';
      destination[output++] = value;
    } else if (static_cast<unsigned char>(value) >= 0x20U &&
               output + 1U < destinationSize) {
      destination[output++] = value;
    }
  }
  destination[output] = '\0';
}

}  // namespace balanca::web
