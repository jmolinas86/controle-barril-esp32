#pragma once

#include <cstdint>

#include "models/Display.h"

namespace keezer::storage {

enum class DisplaySettingsResult : std::uint8_t {
  Ok = 0U,
  NotFound,
  Invalid,
  IoError,
};

class DisplaySettingsStore final {
 public:
  static DisplaySettingsResult load(models::DisplaySettings& settings);
  static DisplaySettingsResult save(const models::DisplaySettings& settings);
};

}  // namespace keezer::storage
