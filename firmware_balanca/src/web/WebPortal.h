#pragma once

#include <ESP8266WebServer.h>

#include <cstdint>

#include "drivers/ScaleHardware.h"
#include "network/NetworkManager.h"
#include "storage/SettingsStore.h"

namespace balanca::web {

class WebPortal final {
 public:
  WebPortal(drivers::ScaleHardware& hardware, network::NetworkManager& network,
            storage::SettingsStore& store, storage::DeviceSettings& settings);

  void begin();
  void update(std::uint32_t nowMs);

 private:
  void handleReading();
  void handleStatus();
  void handleWifiScan();
  void handleWifiSave();
  void handleTare();
  void handleCalibrate();
  void handleRestart();
  void handleFactoryReset();
  void handleOtaPage();
  void handleOtaFinished();
  void handleOtaUpload();
  void handleNotFound();
  void sendResult(int status, bool ok, const char* message);
  static void escapeJson(const char* source, char* destination,
                         std::size_t destinationSize);

  ESP8266WebServer server_{80U};
  drivers::ScaleHardware& hardware_;
  network::NetworkManager& network_;
  storage::SettingsStore& store_;
  storage::DeviceSettings& settings_;
  bool rebootPending_{false};
  std::uint32_t rebootAtMs_{0U};
  bool otaUploadAuthorized_{false};
  bool otaUploadValid_{false};
  bool otaUploadSucceeded_{false};
};

}  // namespace balanca::web
