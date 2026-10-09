#pragma once

#include <cstdint>

namespace keezer::storage {

enum class TemperatureSettingsResult : std::uint8_t {
  Ok = 0U,
  NotFound,
  Invalid,
  IoError,
};

class TemperatureSettingsStore final {
 public:
  static TemperatureSettingsResult loadSetpointCentiCelsius(
      std::int16_t& setpointCentiCelsius);
  static TemperatureSettingsResult saveSetpointCentiCelsius(
      std::int16_t setpointCentiCelsius);
};

}  // namespace keezer::storage
