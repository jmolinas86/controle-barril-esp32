#pragma once

#include <cstdint>

namespace keezer::models {

struct DisplaySettings {
  std::uint8_t brightnessPercent{70U};
  std::uint16_t timeoutSeconds{60U};
};

}  // namespace keezer::models
