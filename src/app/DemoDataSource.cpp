#include "app/DemoDataSource.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "app/DemoData.h"

namespace keezer::app::demo {
namespace {

bool deadlineReached(const std::uint32_t nowMs,
                     const std::uint32_t deadlineMs) {
  return static_cast<std::int32_t>(nowMs - deadlineMs) >= 0;
}

}  // namespace

void DemoDataSource::begin(AppViewState& state, const std::uint32_t nowMs) {
  state.freezer = kFreezer;
  state.kegs = kKegs;
  state.kegCount = kDemoKegCount;
  state.revision = 1U;

  for (std::size_t index = 0U; index < state.kegCount; ++index) {
    state.kegs[index].historyPointCount =
        static_cast<std::uint8_t>(state.kegs[index].historyDeciliters.size());
  }

  nextConsumptionAtMs_ = nowMs + kConsumptionPeriodMs;
  presenceCycleStartedAtMs_ = nowMs;
  simulatedMinute_ = 0U;
  activeKegIndex_ = 0;
  selectActiveKeg(state, activeKegIndex_);
}

bool DemoDataSource::update(AppViewState& state,
                            const std::uint32_t nowMs) {
  bool changed = updatePresence(state, nowMs);
  changed = updateConsumption(state, nowMs) || changed;
  return changed;
}

void DemoDataSource::selectActiveKeg(AppViewState& state,
                                     const std::int8_t index) {
  activeKegIndex_ = index;
  for (std::size_t kegIndex = 0U; kegIndex < state.kegCount; ++kegIndex) {
    const bool active = static_cast<std::int8_t>(kegIndex) == index;
    state.kegs[kegIndex].onScale = active;
    if (state.kegs[kegIndex].status != models::KegStatus::Finished) {
      state.kegs[kegIndex].status =
          active ? models::KegStatus::ActiveOnScale
                 : models::KegStatus::Stored;
    }
  }
}

void DemoDataSource::updateTimestamp(AppViewState& state,
                                     const std::size_t index) {
  constexpr std::int64_t kDemoInitialSyncUtc = 1'788'442'920LL;
  state.kegs[index].lastSyncUtc =
      kDemoInitialSyncUtc + static_cast<std::int64_t>(simulatedMinute_) * 60LL;
}

bool DemoDataSource::updatePresence(AppViewState& state,
                                    const std::uint32_t nowMs) {
  const std::uint32_t phase =
      ((nowMs - presenceCycleStartedAtMs_) / kPresencePeriodMs) % 4U;
  const std::int8_t requestedIndex =
      phase == 0U ? 0 : (phase == 2U ? 1 : -1);
  if (requestedIndex == activeKegIndex_) {
    return false;
  }
  selectActiveKeg(state, requestedIndex);
  return true;
}

bool DemoDataSource::updateConsumption(AppViewState& state,
                                       const std::uint32_t nowMs) {
  if (!deadlineReached(nowMs, nextConsumptionAtMs_)) {
    return false;
  }
  nextConsumptionAtMs_ = nowMs + kConsumptionPeriodMs;
  if (activeKegIndex_ < 0 ||
      static_cast<std::size_t>(activeKegIndex_) >= state.kegCount) {
    return false;
  }

  KegViewData& keg = state.kegs[static_cast<std::size_t>(activeKegIndex_)];
  keg.volumeL = std::max(0.0F, keg.volumeL - 0.1F);
  keg.weightKg = keg.tareKg + (keg.volumeL * keg.densityKgPerL);
  const float percentage =
      keg.capacityL <= 0.0F ? 0.0F : (keg.volumeL / keg.capacityL) * 100.0F;
  keg.percentage = static_cast<std::uint8_t>(
      std::clamp(std::lround(percentage), 0L, 100L));
  keg.historyDeciliters.back() =
      static_cast<std::int16_t>(std::lround(keg.volumeL * 10.0F));
  ++simulatedMinute_;
  updateTimestamp(state, static_cast<std::size_t>(activeKegIndex_));
  return true;
}

}  // namespace keezer::app::demo
