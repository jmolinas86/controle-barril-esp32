#pragma once

#include <cstdint>

#include "temperature/ITemperatureSensor.h"

namespace keezer::temperature {

class DemoTemperatureSensor final : public ITemperatureSensor {
 public:
  bool begin(std::uint32_t nowMs) override;
  void update(std::uint32_t nowMs) override;
  bool latestSample(models::TemperatureSample& sample) const override;

 private:
  void prepareSample(std::uint32_t nowMs);

  models::TemperatureSample sample_{};
  std::uint32_t startedAtMs_{0U};
  std::uint32_t lastSampleAtMs_{0U};
};

}  // namespace keezer::temperature
