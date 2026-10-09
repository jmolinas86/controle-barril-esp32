#include "app/ScaleApplication.h"

#include <Arduino.h>

#include "config.h"

namespace balanca::app {

ScaleApplication::ScaleApplication()
    : webPortal_(hardware_, network_, settingsStore_, settings_) {}

bool ScaleApplication::begin() {
  Serial.begin(115200U);
  Serial.println();
  Serial.printf("[BOOT] Control de Barril - %s / HTTP protocol v1\n", DEVICE_NAME);
  if (!settingsStore_.begin()) {
    Serial.println(F("[STORE] EEPROM initialization failed"));
    return false;
  }
  const bool currentFormat = settingsStore_.load(
      settings_, HX711_CALIBRATION_FACTOR, WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("[STORE] Settings=%s factor=%.4f\n",
                currentFormat ? "V2" : "DEFAULT/LEGACY",
                settings_.calibrationFactor);
  hardware_.begin(settings_);
  network_.begin(settings_.wifiSsid, settings_.wifiPassword);
  webPortal_.begin();
  return true;
}

void ScaleApplication::update() {
  const std::uint32_t nowMs = millis();
  webPortal_.update(nowMs);
  hardware_.update(nowMs);
  webPortal_.update(nowMs);
  network_.update(nowMs);
  webPortal_.update(nowMs);
  hardware_.updateDisplay(nowMs, network_.connected(), network_.provisioning(),
                          network_.rssiDbm(), network_.ip());
  yield();
}

}  // namespace balanca::app
