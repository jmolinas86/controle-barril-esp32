#pragma once

#include <cstdint>

#include "models/Network.h"

namespace keezer::storage {

enum class NetworkSettingsResult : std::uint8_t {
  Ok = 0U,
  NotFound,
  Invalid,
  IoError,
};

class NetworkSettingsStore final {
 public:
  static NetworkSettingsResult load(models::NetworkSettings& settings);
  static NetworkSettingsResult save(const models::NetworkSettings& settings);
};

}  // namespace keezer::storage
