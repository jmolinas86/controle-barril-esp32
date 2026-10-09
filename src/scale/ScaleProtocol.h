#pragma once

#include <cstdint>

#include "models/Scale.h"

namespace keezer::scale {

enum class ScaleProtocolError : std::uint8_t {
  None = 0U,
  UnsupportedVersion,
  InvalidScaleId,
  InvalidNfcUid,
  WeightOutOfRange,
  BatteryOutOfRange,
  RssiOutOfRange,
};

class ScaleProtocol final {
 public:
  static ScaleProtocolError validate(const models::ScalePacket& packet,
                                     std::uint64_t receivedAtMonotonicMs,
                                     std::int32_t maximumWeightGrams,
                                     models::ScaleReading& reading);
};

const char* scaleProtocolErrorName(ScaleProtocolError error);

}  // namespace keezer::scale
