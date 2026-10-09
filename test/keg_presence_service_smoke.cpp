#include <cassert>
#include <cstdio>

#include "services/KegPresenceService.h"

namespace {

keezer::models::ScaleState scaleState(const bool online, const bool hasNfc,
                                      const std::int32_t weightGrams) {
  keezer::models::ScaleState state{};
  state.linkStatus = online ? keezer::models::ScaleLinkStatus::Online
                            : keezer::models::ScaleLinkStatus::Offline;
  state.hasStableNfcUid = hasNfc;
  state.hasFilteredWeight = true;
  state.filteredWeightGrams = weightGrams;
  state.stable = true;
  return state;
}

}  // namespace

int main() {
  using keezer::services::KegPresenceService;
  using keezer::services::KegPresenceState;
  using keezer::services::RemovalDecision;

  KegPresenceService service;
  assert(service.begin());
  service.synchronizeActiveKeg("KEG_001", 18'550, 0U);
  assert(service.state() == KegPresenceState::Active);

  auto scale = scaleState(true, true, 18'550);
  assert(service.update(100U, scale) == RemovalDecision::None);

  scale = scaleState(false, false, 0);
  assert(service.update(100'000U, scale) == RemovalDecision::None);
  assert(service.state() == KegPresenceState::Uncertain);

  scale = scaleState(true, true, 18'550);
  assert(service.update(101'000U, scale) == RemovalDecision::None);
  assert(service.state() == KegPresenceState::Active);

  scale = scaleState(true, false, 450);
  assert(service.update(102'000U, scale) == RemovalDecision::None);
  assert(service.state() == KegPresenceState::RemovalSuspected);
  assert(service.update(106'999U, scale) == RemovalDecision::None);
  assert(service.update(107'000U, scale) ==
         RemovalDecision::ConfirmedStrong);

  service.synchronizeActiveKeg(nullptr, 0, 107'001U);
  service.synchronizeActiveKeg("KEG_002", 10'650, 108'000U);
  scale = scaleState(true, false, 8'000);
  assert(service.update(108'000U, scale) == RemovalDecision::None);
  assert(service.update(123'000U, scale) == RemovalDecision::None);
  assert(service.state() == KegPresenceState::Active);

  // The legacy moderate decision remains opt-in. Hybrid tracking keeps it off
  // so normal accumulated consumption cannot remove a manually bound KEG.
  KegPresenceService legacyService;
  keezer::services::KegPresenceSettings legacySettings{};
  legacySettings.allowSignificantDropRemovalWithoutNfc = true;
  assert(legacyService.begin(legacySettings));
  legacyService.synchronizeActiveKeg("KEG_002", 10'650, 108'000U);
  assert(legacyService.update(108'000U, scale) == RemovalDecision::None);
  assert(legacyService.update(123'000U, scale) ==
         RemovalDecision::ConfirmedModerate);

  service.synchronizeActiveKeg(nullptr, 0, 123'001U);
  service.synchronizeActiveKeg("KEG_003", 4'750, 124'000U);
  scale = scaleState(true, false, 400);
  assert(service.update(124'000U, scale) == RemovalDecision::None);
  scale = scaleState(true, true, 4'750);
  assert(service.update(128'000U, scale) == RemovalDecision::None);
  scale = scaleState(true, false, 400);
  assert(service.update(129'000U, scale) == RemovalDecision::None);
  assert(service.update(133'999U, scale) == RemovalDecision::None);

  std::puts("KegPresenceService smoke test: PASS");
  return 0;
}
