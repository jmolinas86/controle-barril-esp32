#pragma once

#include <cstdint>

#include "models/Temperature.h"
#include "temperature/CompressorController.h"
#include "temperature/ICompressorOutput.h"
#include "temperature/ITemperatureSensor.h"

namespace keezer::services {

struct TemperatureSettings {
  std::int16_t setpointCentiCelsius{200};
  std::uint16_t hysteresisCentiCelsius{100U};
  std::uint32_t minimumOffTimeMs{180'000U};
  std::uint32_t minimumOnTimeMs{60'000U};
  std::uint32_t sensorTimeoutMs{10'000U};
  std::int16_t minimumValidCentiCelsius{-2'000};
  std::int16_t maximumValidCentiCelsius{5'000};
  std::uint8_t recoveryValidSamples{3U};
};

class TemperatureService final {
 public:
  TemperatureService(temperature::ITemperatureSensor& sensor,
                     temperature::ICompressorOutput& output);

  bool begin(std::uint32_t nowMs, const TemperatureSettings& settings = {});
  void update(std::uint32_t nowMs);
  bool setSetpointCentiCelsius(std::int16_t setpointCentiCelsius);

  const models::TemperatureState& state() const;
  std::uint32_t revision() const;

 private:
  void processNewSample(const models::TemperatureSample& sample);
  void setFault(models::TemperatureFault fault,
                models::TemperatureSampleStatus status);
  void clearFault();
  void publishControllerState(std::uint32_t nowMs);
  void logChanges(models::TemperatureControlState previousControlState,
                  bool previousCompressorOn,
                  models::TemperatureFault previousFault);

  temperature::ITemperatureSensor& sensor_;
  temperature::CompressorController controller_;
  models::TemperatureState state_{};
  std::uint32_t lastSeenSequence_{0U};
  std::uint32_t beganAtMs_{0U};
  std::uint32_t sensorTimeoutMs_{10'000U};
  std::int16_t minimumValidCentiCelsius_{-2'000};
  std::int16_t maximumValidCentiCelsius_{5'000};
  std::uint8_t recoveryValidSamples_{3U};
  std::uint8_t consecutiveRecoverySamples_{0U};
  std::uint8_t validSampleLogCounter_{0U};
  bool hasSeenSequence_{false};
  bool faultLatched_{false};
  bool started_{false};
};

}  // namespace keezer::services
