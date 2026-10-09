#pragma once

#include <array>
#include <cstdint>

#include "models/Keg.h"
#include "models/Scale.h"

namespace keezer::services {

enum class KegPresenceState : std::uint8_t {
  None = 0U,
  Active,
  RemovalSuspected,
  Uncertain,
};

enum class RemovalDecision : std::uint8_t {
  None = 0U,
  ConfirmedStrong,
  ConfirmedModerate,
};

struct KegPresenceSettings {
  std::int32_t removalWeightThresholdGrams{1'000};
  std::int32_t significantDropGrams{2'000};
  std::uint32_t strongConfirmationMs{5'000U};
  std::uint32_t moderateConfirmationMs{15'000U};
  bool allowSignificantDropRemovalWithoutNfc{false};
};

class KegPresenceService final {
 public:
  bool begin(const KegPresenceSettings& settings = {});
  void synchronizeActiveKeg(const char* activeKegId,
                            std::int32_t referenceWeightGrams,
                            std::uint32_t nowMs);
  RemovalDecision update(std::uint32_t nowMs,
                         const models::ScaleState& scale);

  KegPresenceState state() const;
  const char* activeKegId() const;
  std::uint32_t revision() const;

 private:
  void setState(KegPresenceState state);
  void resetEvidence();

  KegPresenceSettings settings_{};
  std::array<char, models::kKegIdBytes> activeKegId_{};
  std::int32_t referenceWeightGrams_{0};
  KegPresenceState state_{KegPresenceState::None};
  std::uint32_t nfcAbsentSinceMs_{0U};
  std::uint32_t lowWeightSinceMs_{0U};
  std::uint32_t revision_{0U};
  bool nfcAbsenceTiming_{false};
  bool lowWeightTiming_{false};
  bool decisionIssued_{false};
  bool started_{false};
};

const char* kegPresenceStateName(KegPresenceState state);
const char* removalDecisionName(RemovalDecision decision);

}  // namespace keezer::services
