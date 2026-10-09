#include "storage/SettingsStore.h"

#include <Arduino.h>
#include <EEPROM.h>

#include <cmath>
#include <cstring>

namespace balanca::storage {
namespace {

constexpr std::size_t kEepromSize = 256U;
constexpr std::uint32_t kSettingsMagic = 0x43425332UL;  // CBS2
constexpr std::uint16_t kSettingsVersion = 2U;
constexpr std::uint32_t kLegacyCalibrationMagic = 0x43424C31UL;
constexpr std::uint32_t kLegacyWifiMagic = 0x43425731UL;
constexpr std::uint32_t kLegacyWifiClearedMagic = 0x43425730UL;
constexpr std::size_t kLegacyWifiAddress = 32U;

struct PersistedSettings {
  std::uint32_t magic;
  std::uint16_t version;
  std::uint16_t reserved;
  float calibrationFactor;
  std::int32_t offset;
  char wifiSsid[33];
  char wifiPassword[65];
  std::uint32_t checksum;
};

struct LegacyCalibration {
  std::uint32_t magic;
  float factor;
  std::int32_t offset;
};

struct LegacyWifi {
  std::uint32_t magic;
  char ssid[33];
  char password[65];
};

bool validFactor(const float value) {
  return std::isfinite(value) && std::fabs(value) >= 1.0F &&
         std::fabs(value) <= 10000000.0F;
}

void copyText(char* destination, const std::size_t destinationSize,
              const char* source) {
  if (destinationSize == 0U) return;
  ::snprintf(destination, destinationSize, "%s", source == nullptr ? "" : source);
}

}  // namespace

bool SettingsStore::begin() {
  EEPROM.begin(kEepromSize);
  return true;
}

bool SettingsStore::load(DeviceSettings& settings,
                         const float defaultCalibrationFactor,
                         const char* const defaultSsid,
                         const char* const defaultPassword) {
  settings = {};
  settings.calibrationFactor = defaultCalibrationFactor;
  copyText(settings.wifiSsid, sizeof(settings.wifiSsid), defaultSsid);
  copyText(settings.wifiPassword, sizeof(settings.wifiPassword), defaultPassword);

  PersistedSettings persisted{};
  EEPROM.get(0, persisted);
  const std::uint32_t storedChecksum = persisted.checksum;
  persisted.checksum = 0U;
  if (persisted.magic == kSettingsMagic &&
      persisted.version == kSettingsVersion &&
      storedChecksum == checksum(&persisted, sizeof(persisted)) &&
      validFactor(persisted.calibrationFactor)) {
    settings.calibrationFactor = persisted.calibrationFactor;
    settings.offset = persisted.offset;
    copyText(settings.wifiSsid, sizeof(settings.wifiSsid), persisted.wifiSsid);
    copyText(settings.wifiPassword, sizeof(settings.wifiPassword),
             persisted.wifiPassword);
    return true;
  }

  LegacyCalibration legacyCalibration{};
  LegacyWifi legacyWifi{};
  EEPROM.get(0, legacyCalibration);
  EEPROM.get(kLegacyWifiAddress, legacyWifi);
  if (legacyCalibration.magic == kLegacyCalibrationMagic &&
      validFactor(legacyCalibration.factor)) {
    settings.calibrationFactor = legacyCalibration.factor;
    settings.offset = legacyCalibration.offset;
  }
  if (legacyWifi.magic == kLegacyWifiMagic) {
    legacyWifi.ssid[sizeof(legacyWifi.ssid) - 1U] = '\0';
    legacyWifi.password[sizeof(legacyWifi.password) - 1U] = '\0';
    copyText(settings.wifiSsid, sizeof(settings.wifiSsid), legacyWifi.ssid);
    copyText(settings.wifiPassword, sizeof(settings.wifiPassword),
             legacyWifi.password);
  } else if (legacyWifi.magic == kLegacyWifiClearedMagic) {
    settings.wifiSsid[0] = '\0';
    settings.wifiPassword[0] = '\0';
  }
  return false;
}

bool SettingsStore::save(const DeviceSettings& settings) {
  PersistedSettings persisted{};
  persisted.magic = kSettingsMagic;
  persisted.version = kSettingsVersion;
  persisted.calibrationFactor = settings.calibrationFactor;
  persisted.offset = settings.offset;
  copyText(persisted.wifiSsid, sizeof(persisted.wifiSsid), settings.wifiSsid);
  copyText(persisted.wifiPassword, sizeof(persisted.wifiPassword),
           settings.wifiPassword);
  persisted.checksum = 0U;
  persisted.checksum = checksum(&persisted, sizeof(persisted));
  EEPROM.put(0, persisted);
  return EEPROM.commit();
}

bool SettingsStore::factoryReset() {
  for (std::size_t index = 0U; index < kEepromSize; ++index) {
    EEPROM.write(index, 0U);
  }
  LegacyWifi cleared{};
  cleared.magic = kLegacyWifiClearedMagic;
  EEPROM.put(kLegacyWifiAddress, cleared);
  return EEPROM.commit();
}

std::uint32_t SettingsStore::checksum(const void* const data,
                                      const std::size_t size) {
  const auto* bytes = static_cast<const std::uint8_t*>(data);
  std::uint32_t value = 2166136261UL;
  for (std::size_t index = 0U; index < size; ++index) {
    value ^= bytes[index];
    value *= 16777619UL;
  }
  return value;
}

}  // namespace balanca::storage
