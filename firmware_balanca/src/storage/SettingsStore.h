#pragma once

#include <cstddef>
#include <cstdint>

namespace balanca::storage {

struct DeviceSettings {
  float calibrationFactor{0.0F};
  std::int32_t offset{0};
  char wifiSsid[33]{};
  char wifiPassword[65]{};
};

class SettingsStore final {
 public:
  bool begin();
  bool load(DeviceSettings& settings, float defaultCalibrationFactor,
            const char* defaultSsid, const char* defaultPassword);
  bool save(const DeviceSettings& settings);
  bool factoryReset();

 private:
  static std::uint32_t checksum(const void* data, std::size_t size);
};

}  // namespace balanca::storage
