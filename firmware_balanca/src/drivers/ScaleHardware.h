#pragma once

#include <Adafruit_PN532.h>
#include <Adafruit_SSD1306.h>
#include <HX711.h>
#include <IPAddress.h>

#include <cstdint>

#include "model/ScaleSnapshot.h"
#include "storage/SettingsStore.h"
#include "config.h"

namespace balanca::drivers {

class ScaleHardware final {
 public:
  ScaleHardware();

  void begin(const storage::DeviceSettings& settings);
  void update(std::uint32_t nowMs);
  bool tare(storage::DeviceSettings& settings);
  bool calibrate(std::int32_t referenceGrams,
                 storage::DeviceSettings& settings);
  void updateDisplay(std::uint32_t nowMs, bool wifiConnected,
                     bool provisioning, std::int32_t rssiDbm,
                     const IPAddress& ip);

  const model::ScaleSnapshot& snapshot() const;
  bool hx711Healthy() const;
  bool pn532Healthy() const;
  bool oledHealthy() const;
  float calibrationFactor() const;

 private:
  void updateWeight(std::uint32_t nowMs);
  void updateNfc(std::uint32_t nowMs);
  void clearWeightWindow();
  bool weightWindowStable() const;
  bool i2cDevicePresent(std::uint8_t address) const;

  HX711 scale_;
  Adafruit_PN532 nfc_;
  Adafruit_SSD1306 display_;
  model::ScaleSnapshot snapshot_{};
  float calibrationFactor_{0.0F};
  float rawSamples_[HX711_READ_SAMPLES]{};
  std::int32_t weightSamples_[HX711_READ_SAMPLES]{};
  std::uint8_t sampleCount_{0U};
  std::uint8_t sampleIndex_{0U};
  bool hx711Healthy_{false};
  bool pn532Healthy_{false};
  bool oledHealthy_{false};
  std::uint32_t lastNfcPollAtMs_{0U};
  std::uint32_t lastTagSeenAtMs_{0U};
  std::uint32_t lastDisplayAtMs_{0U};
};

}  // namespace balanca::drivers
