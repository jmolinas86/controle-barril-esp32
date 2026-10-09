#pragma once

#include <cstdint>

#include "models/Temperature.h"
#include "temperature/ICompressorOutput.h"

namespace keezer::temperature {

struct CompressorSettings {
  std::int16_t setpointCentiCelsius{200};
  std::uint16_t hysteresisCentiCelsius{100U};
  std::uint32_t minimumOffTimeMs{180'000U};
  std::uint32_t minimumOnTimeMs{60'000U};
};

class CompressorController final {
 public:
  explicit CompressorController(ICompressorOutput& output);

  bool begin(std::uint32_t nowMs, const CompressorSettings& settings = {});
  void update(std::uint32_t nowMs, bool sensorHealthy,
              std::int16_t temperatureCentiCelsius);
  bool setSetpoint(std::int16_t setpointCentiCelsius);

  models::TemperatureControlState state() const;
  bool compressorOn() const;
  bool demandCooling() const;
  bool outputFault() const;
  std::uint32_t changedAtMs() const;
  std::uint32_t protectionRemainingMs() const;
  std::int16_t setpointCentiCelsius() const;

 private:
  void enterError(std::uint32_t nowMs);
  bool turnOn(std::uint32_t nowMs);
  bool turnOff(std::uint32_t nowMs);
  void updateProtection(std::uint32_t nowMs);

  ICompressorOutput& output_;
  CompressorSettings settings_{};
  models::TemperatureControlState state_{
      models::TemperatureControlState::Idle};
  bool demandCooling_{false};
  bool outputFault_{false};
  std::uint32_t outputChangedAtMs_{0U};
  std::uint32_t offSinceMs_{0U};
  std::uint32_t onSinceMs_{0U};
  std::uint32_t protectionRemainingMs_{0U};
  bool started_{false};
};

}  // namespace keezer::temperature
