#pragma once

#include "models/Scale.h"

namespace keezer::scale {

class ScaleHttpJson final {
 public:
  static bool parseReading(const char* json, models::ScalePacket& packet);
};

}  // namespace keezer::scale
