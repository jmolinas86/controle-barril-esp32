#pragma once

#include <cstdint>

namespace keezer::models {

enum class TemperatureSampleQuality : std::uint8_t {
  Valid = 0U,
  Disconnected,
  ReadError,
};

enum class TemperatureSampleStatus : std::uint8_t {
  NoData = 0U,
  Valid,
  Stale,
  Disconnected,
  OutOfRange,
  Error,
};

enum class TemperatureControlState : std::uint8_t {
  Idle = 0U,
  Waiting,
  Cooling,
  Error,
};

enum class TemperatureFault : std::uint8_t {
  None = 0U,
  NoSample,
  Timeout,
  Disconnected,
  OutOfRange,
  SensorError,
  CompressorOutputError,
};

const char* temperatureSampleStatusName(TemperatureSampleStatus status);
const char* temperatureControlStateName(TemperatureControlState state);
const char* temperatureFaultName(TemperatureFault fault);

struct TemperatureSample {
  std::uint32_t sequence{0U};
  std::uint32_t sampledAtMonotonicMs{0U};
  std::int16_t centiCelsius{0};
  TemperatureSampleQuality quality{TemperatureSampleQuality::ReadError};
};

struct TemperatureState {
  bool hasCurrentTemperature{false};
  std::int16_t currentCentiCelsius{0};
  std::int16_t setpointCentiCelsius{200};
  TemperatureSampleStatus sampleStatus{TemperatureSampleStatus::NoData};
  TemperatureControlState controlState{TemperatureControlState::Idle};
  TemperatureFault fault{TemperatureFault::None};
  bool compressorOn{false};
  bool demandCooling{false};
  std::uint32_t lastSampleMonotonicMs{0U};
  std::uint32_t compressorChangedAtMs{0U};
  std::uint32_t protectionRemainingMs{0U};
  std::uint32_t revision{0U};
};

}  // namespace keezer::models
