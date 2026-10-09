#include <cassert>
#include <cstdio>

#include "services/KegTrackingService.h"

namespace {

keezer::models::ScaleState reading(const std::uint32_t count,
                                   const std::int32_t weight,
                                   const bool online = true,
                                   const bool stable = true) {
  keezer::models::ScaleState scale{};
  scale.linkStatus = online ? keezer::models::ScaleLinkStatus::Online
                            : keezer::models::ScaleLinkStatus::Offline;
  scale.hasFilteredWeight = true;
  scale.filteredWeightGrams = weight;
  scale.stable = stable;
  scale.acceptedReadingCount = count;
  return scale;
}

}  // namespace

int main() {
  using keezer::services::KegTrackingAction;
  using keezer::services::KegTrackingService;
  using keezer::services::KegTrackingState;

  KegTrackingService service;
  assert(service.begin());
  service.observeActiveKeg("KEG_001", 18'500, 0U);
  assert(service.state() == KegTrackingState::ChangePending);

  service.activate("KEG_001", 18'500, 1U, 0U);
  assert(service.state() == KegTrackingState::Monitoring);
  assert(service.update(31'000U, reading(2U, 18'300), false, false).action ==
         KegTrackingAction::None);
  assert(service.update(33'999U, reading(3U, 18'300), false, false).action ==
         KegTrackingAction::None);
  const auto consumed =
      service.update(34'000U, reading(4U, 18'300), false, false);
  assert(consumed.action == KegTrackingAction::RecordConsumption);
  assert(consumed.weightGrams == 18'300);
  service.completeRecord(true, consumed.weightGrams, 34'000U);
  assert(service.baselineWeightGrams() == 18'300);

  assert(service.update(40'000U, reading(5U, 18'700), false, false).action ==
         KegTrackingAction::None);
  const auto increased =
      service.update(43'000U, reading(6U, 18'700), false, false);
  assert(increased.action == KegTrackingAction::ConfirmKeg);
  assert(service.state() == KegTrackingState::ChangePending);

  service.activate("KEG_001", 18'700, 6U, 44'000U);
  assert(service.update(45'000U, reading(7U, 18'700, false), false, false)
             .action == KegTrackingAction::None);
  assert(service.state() == KegTrackingState::Paused);

  assert(service.update(46'000U, reading(8U, 16'500), false, false).action ==
         KegTrackingAction::None);
  const auto uncertain =
      service.update(49'000U, reading(9U, 16'500), false, false);
  assert(uncertain.action == KegTrackingAction::ConfirmKeg);

  service.activate("KEG_001", 18'700, 9U, 50'000U);
  assert(service.update(81'000U, reading(10U, 16'500), false, true).action ==
         KegTrackingAction::None);
  const auto verified =
      service.update(84'000U, reading(11U, 16'500), false, true);
  assert(verified.action == KegTrackingAction::RecordConsumption);

  std::puts("KegTrackingService smoke test: PASS");
  return 0;
}
