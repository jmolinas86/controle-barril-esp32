#include "storage/TemperatureSettingsStore.h"

#include <Preferences.h>

#include "BuildConfig.h"
#include "diagnostics/Logger.h"

namespace keezer::storage {
namespace {

constexpr char kLogTag[] = "TEMP_STORE";
constexpr char kNamespace[] = "temp_cfg";
constexpr char kSetpointKey[] = "setpoint_centi";

bool validSetpoint(const std::int32_t value) {
  return value >= config::kTemperatureMinimumSetpointCentiCelsius &&
         value <= config::kTemperatureMaximumSetpointCentiCelsius;
}

}  // namespace

TemperatureSettingsResult
TemperatureSettingsStore::loadSetpointCentiCelsius(
    std::int16_t& setpointCentiCelsius) {
  Preferences preferences;
  if (!preferences.begin(kNamespace, true)) {
    return TemperatureSettingsResult::IoError;
  }
  if (!preferences.isKey(kSetpointKey)) {
    preferences.end();
    return TemperatureSettingsResult::NotFound;
  }
  const std::int32_t stored = preferences.getInt(kSetpointKey, INT32_MIN);
  preferences.end();
  if (!validSetpoint(stored)) {
    KEEZER_LOG_WARN(kLogTag, "Stored setpoint rejected: %ld",
                    static_cast<long>(stored));
    return TemperatureSettingsResult::Invalid;
  }
  setpointCentiCelsius = static_cast<std::int16_t>(stored);
  KEEZER_LOG_INFO(kLogTag, "Setpoint loaded: %d.%02dC",
                  static_cast<int>(setpointCentiCelsius / 100),
                  static_cast<int>(setpointCentiCelsius >= 0
                                       ? setpointCentiCelsius % 100
                                       : -(setpointCentiCelsius % 100)));
  return TemperatureSettingsResult::Ok;
}

TemperatureSettingsResult
TemperatureSettingsStore::saveSetpointCentiCelsius(
    const std::int16_t setpointCentiCelsius) {
  if (!validSetpoint(setpointCentiCelsius)) {
    return TemperatureSettingsResult::Invalid;
  }
  Preferences preferences;
  if (!preferences.begin(kNamespace, false)) {
    return TemperatureSettingsResult::IoError;
  }
  const std::size_t written =
      preferences.putInt(kSetpointKey, setpointCentiCelsius);
  preferences.end();
  return written == sizeof(std::int32_t) ? TemperatureSettingsResult::Ok
                                         : TemperatureSettingsResult::IoError;
}

}  // namespace keezer::storage
