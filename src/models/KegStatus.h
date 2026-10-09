#pragma once

#include <cstdint>

namespace keezer::models {

enum class KegStatus : std::uint8_t {
  Available = 0U,
  ActiveOnScale,
  Stored,
  Empty,
  Finished,
  Cleaning,
  Archived,
};

const char* kegStatusName(KegStatus status);
bool canActivateAutomatically(KegStatus status);

}  // namespace keezer::models
