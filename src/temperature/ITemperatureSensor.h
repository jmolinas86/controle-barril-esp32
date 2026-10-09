#pragma once

#include <cstdint>

#include "models/Temperature.h"

namespace keezer::temperature {

class ITemperatureSensor {
 public:
  virtual ~ITemperatureSensor() = default;
  virtual bool begin(std::uint32_t nowMs) = 0;
  virtual void update(std::uint32_t nowMs) = 0;
  virtual bool latestSample(models::TemperatureSample& sample) const = 0;
};

}  // namespace keezer::temperature
