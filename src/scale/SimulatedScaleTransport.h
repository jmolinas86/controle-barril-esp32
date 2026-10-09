#pragma once

#include <cstdint>

#include "scale/IScaleTransport.h"

namespace keezer::scale {

// Phase 11 bench source: alternates KEG 1, KEG 2 and an empty scale, adds
// repeatable jitter/outliers and exercises STALE/OFFLINE. It never applies
// keg business rules.
class SimulatedScaleTransport final : public IScaleTransport {
 public:
  bool begin(std::uint32_t nowMs) override;
  void update(std::uint32_t nowMs) override;
  bool isConnected() const override;
  bool tryRead(models::ScalePacket& packet) override;

 private:
  void preparePacket(std::uint32_t nowMs);

  models::ScalePacket pendingPacket_{};
  std::uint32_t startedAtMs_{0U};
  std::uint32_t lastEmissionAtMs_{0U};
  std::uint32_t sequence_{0U};
  std::uint32_t currentScenario_{3U};
  std::uint8_t scenarioSampleIndex_{0U};
  bool pending_{false};
  bool emittedAtLeastOnce_{false};
  bool connected_{false};
};

}  // namespace keezer::scale
