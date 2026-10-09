#include "storage/DisplaySettingsStore.h"

#include <Preferences.h>

namespace keezer::storage {
namespace {

constexpr char kNamespace[] = "display_cfg";
constexpr char kBrightnessKey[] = "brightness";
constexpr char kTimeoutKey[] = "timeout_s";

bool valid(const models::DisplaySettings& settings) {
  if (settings.brightnessPercent < 10U ||
      settings.brightnessPercent > 100U) {
    return false;
  }
  switch (settings.timeoutSeconds) {
    case 0U:
    case 30U:
    case 60U:
    case 120U:
    case 300U:
      return true;
    default:
      return false;
  }
}

}  // namespace

DisplaySettingsResult DisplaySettingsStore::load(
    models::DisplaySettings& settings) {
  Preferences preferences;
  if (!preferences.begin(kNamespace, true)) {
    return DisplaySettingsResult::IoError;
  }
  if (!preferences.isKey(kBrightnessKey) ||
      !preferences.isKey(kTimeoutKey)) {
    preferences.end();
    return DisplaySettingsResult::NotFound;
  }
  models::DisplaySettings stored{};
  stored.brightnessPercent = preferences.getUChar(kBrightnessKey, 70U);
  stored.timeoutSeconds = preferences.getUShort(kTimeoutKey, 60U);
  preferences.end();
  if (!valid(stored)) {
    return DisplaySettingsResult::Invalid;
  }
  settings = stored;
  return DisplaySettingsResult::Ok;
}

DisplaySettingsResult DisplaySettingsStore::save(
    const models::DisplaySettings& settings) {
  if (!valid(settings)) {
    return DisplaySettingsResult::Invalid;
  }
  Preferences preferences;
  if (!preferences.begin(kNamespace, false)) {
    return DisplaySettingsResult::IoError;
  }
  const bool brightnessSaved =
      preferences.putUChar(kBrightnessKey, settings.brightnessPercent) == 1U;
  const bool timeoutSaved =
      preferences.putUShort(kTimeoutKey, settings.timeoutSeconds) == 2U;
  preferences.end();
  return brightnessSaved && timeoutSaved ? DisplaySettingsResult::Ok
                                          : DisplaySettingsResult::IoError;
}

}  // namespace keezer::storage
