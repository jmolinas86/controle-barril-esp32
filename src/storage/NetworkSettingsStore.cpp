#include "storage/NetworkSettingsStore.h"

#include <Preferences.h>

#include <cstddef>
#include <cstdint>

#include "services/NetworkService.h"

namespace keezer::storage {
namespace {

constexpr char kNamespace[] = "network_cfg";
constexpr char kSettingsKey[] = "settings";
constexpr std::uint32_t kMagic = 0x4B4E4554U;  // KNET
constexpr std::uint16_t kVersion = 1U;

struct PersistedSettings {
  std::uint32_t magic{kMagic};
  std::uint16_t version{kVersion};
  std::uint16_t reserved{0U};
  models::NetworkSettings settings{};
  std::uint32_t crc32{0U};
};

std::uint32_t crc32(const std::uint8_t* data, const std::size_t size) {
  std::uint32_t crc = 0xFFFFFFFFU;
  for (std::size_t index = 0U; index < size; ++index) {
    crc ^= data[index];
    for (std::uint8_t bit = 0U; bit < 8U; ++bit) {
      crc = (crc >> 1U) ^ (0xEDB88320U &
                           static_cast<std::uint32_t>(
                               -static_cast<std::int32_t>(crc & 1U)));
    }
  }
  return ~crc;
}

std::uint32_t payloadCrc(const PersistedSettings& stored) {
  return crc32(reinterpret_cast<const std::uint8_t*>(&stored),
               offsetof(PersistedSettings, crc32));
}

}  // namespace

NetworkSettingsResult NetworkSettingsStore::load(
    models::NetworkSettings& settings) {
  Preferences preferences;
  if (!preferences.begin(kNamespace, true)) {
    return NetworkSettingsResult::IoError;
  }
  const std::size_t length = preferences.getBytesLength(kSettingsKey);
  if (length == 0U) {
    preferences.end();
    return NetworkSettingsResult::NotFound;
  }
  if (length != sizeof(PersistedSettings)) {
    preferences.end();
    return NetworkSettingsResult::Invalid;
  }
  PersistedSettings stored{};
  const std::size_t read =
      preferences.getBytes(kSettingsKey, &stored, sizeof(stored));
  preferences.end();
  if (read != sizeof(stored) || stored.magic != kMagic ||
      stored.version != kVersion || stored.crc32 != payloadCrc(stored) ||
      !services::NetworkService::validSettings(stored.settings)) {
    return NetworkSettingsResult::Invalid;
  }
  settings = stored.settings;
  return NetworkSettingsResult::Ok;
}

NetworkSettingsResult NetworkSettingsStore::save(
    const models::NetworkSettings& settings) {
  if (!services::NetworkService::validSettings(settings)) {
    return NetworkSettingsResult::Invalid;
  }
  PersistedSettings stored{};
  stored.settings = settings;
  stored.crc32 = payloadCrc(stored);
  Preferences preferences;
  if (!preferences.begin(kNamespace, false)) {
    return NetworkSettingsResult::IoError;
  }
  const std::size_t written =
      preferences.putBytes(kSettingsKey, &stored, sizeof(stored));
  preferences.end();
  return written == sizeof(stored) ? NetworkSettingsResult::Ok
                                    : NetworkSettingsResult::IoError;
}

}  // namespace keezer::storage
