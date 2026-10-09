#pragma once

#include <cstdint>

#include "models/Scale.h"

namespace keezer::scale {

class IScaleTransport {
 public:
  virtual ~IScaleTransport() = default;

  virtual bool begin(std::uint32_t nowMs) = 0;
  virtual void update(std::uint32_t nowMs) = 0;
  virtual bool isConnected() const = 0;
  virtual bool tryRead(models::ScalePacket& packet) = 0;
};

}  // namespace keezer::scale
