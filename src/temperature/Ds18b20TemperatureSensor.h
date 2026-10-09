#pragma once

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
  bool requestConversion(std::uint32_t nowMs);
  models::TemperatureSampleQuality readTemperature(
      std::int16_t& centiCelsius);
  bool busReset();
  void busWriteBit(bool value);
  bool busReadBit();
  void busWriteByte(std::uint8_t value);
  std::uint8_t busReadByte();
  static std::uint8_t crc8(const std::uint8_t* data, std::uint8_t size);
  void publish(std::uint32_t nowMs, std::int16_t centiCelsius,
               models::TemperatureSampleQuality quality);

  static constexpr std::uint32_t kConversionTimeMs = 750U;
  static constexpr std::uint32_t kSamplePeriodMs = 1'000U;
  static constexpr std::uint32_t kDiscoveryRetryMs = 1'000U;

  std::uint8_t dataPin_;
  models::TemperatureSample sample_{};
  std::uint32_t conversionRequestedAtMs_{0U};
  std::uint32_t lastSampleAtMs_{0U};
  std::uint32_t lastDiscoveryAtMs_{0U};
  bool sensorPresent_{false};
  bool conversionPending_{false};
  bool started_{false};
};

}  // namespace keezer::temperature
