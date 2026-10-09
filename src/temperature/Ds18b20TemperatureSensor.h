#pragma once

#include <OneWire.h>

#include <array>
#include <cstdint>

#include "temperature/ITemperatureSensor.h"

namespace keezer::temperature {

class Ds18b20TemperatureSensor final : public ITemperatureSensor {
 public:
  explicit Ds18b20TemperatureSensor(std::uint8_t dataPin);

  bool begin(std::uint32_t nowMs) override;
  void update(std::uint32_t nowMs) override;
  bool latestSample(models::TemperatureSample& sample) const override;

 private:
  bool discoverSensor();
  bool requestConversion(std::uint32_t nowMs);
  models::TemperatureSampleQuality readTemperature(
      std::int16_t& centiCelsius);
  void publish(std::uint32_t nowMs, std::int16_t centiCelsius,
               models::TemperatureSampleQuality quality);

  static constexpr std::uint32_t kConversionTimeMs = 750U;
  static constexpr std::uint32_t kSamplePeriodMs = 1'000U;
  static constexpr std::uint32_t kDiscoveryRetryMs = 1'000U;

  std::uint8_t dataPin_;
  OneWire bus_;
  std::array<std::uint8_t, 8U> address_{};
  models::TemperatureSample sample_{};
  std::uint32_t conversionRequestedAtMs_{0U};
  std::uint32_t lastSampleAtMs_{0U};
  std::uint32_t lastDiscoveryAtMs_{0U};
  bool sensorPresent_{false};
  bool addressValid_{false};
  bool conversionPending_{false};
  bool started_{false};
};

}  // namespace keezer::temperature
