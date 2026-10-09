#pragma once

#include <cstdint>

#include "app/ViewData.h"

namespace keezer::app::demo {

class DemoDataSource final {
 public:
  void begin(AppViewState& state, std::uint32_t nowMs);
  bool update(AppViewState& state, std::uint32_t nowMs);

 private:
  static constexpr std::uint32_t kConsumptionPeriodMs = 5'000U;
  static constexpr std::uint32_t kPresencePeriodMs = 20'000U;

  void selectActiveKeg(AppViewState& state, std::int8_t index);
  void updateTimestamp(AppViewState& state, std::size_t index);
  bool updatePresence(AppViewState& state, std::uint32_t nowMs);
  bool updateConsumption(AppViewState& state, std::uint32_t nowMs);

  std::uint32_t nextConsumptionAtMs_{0U};
  std::uint32_t presenceCycleStartedAtMs_{0U};
  std::uint32_t simulatedMinute_{0U};
  std::int8_t activeKegIndex_{0};
};

}  // namespace keezer::app::demo
