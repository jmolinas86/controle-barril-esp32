#pragma once

#include <array>
#include <cstdint>

#include "models/Keg.h"
#include "models/Scale.h"

namespace keezer::services {

enum class KegTrackingState : std::uint8_t {
  Idle = 0U,
  Acquiring,
  Monitoring,
  Paused,
  ChangePending,
};

enum class KegTrackingAction : std::uint8_t {
  None = 0U,
  RecordConsumption,
  ConfirmKeg,
};

struct KegTrackingSettings {
  std::int32_t minimumConsumptionGrams{100};
  std::uint32_t stableConfirmationMs{3'000U};
  std::uint32_t minimumRecordIntervalMs{30'000U};
  std::int32_t suspiciousIncreaseGrams{300};
  std::int32_t suspiciousChangeGrams{2'000};
  std::int32_t removalWeightThresholdGrams{1'000};
  std::int32_t candidateDeadbandGrams{20};
};

struct KegTrackingDecision {
  KegTrackingAction action{KegTrackingAction::None};
  std::int32_t weightGrams{0};
};

class KegTrackingService final {
 public:
  bool begin(const KegTrackingSettings& settings = {});
  void observeActiveKeg(const char* kegId, std::int32_t baselineWeightGrams,
                        std::uint32_t nowMs);
  void activate(const char* kegId, std::int32_t baselineWeightGrams,
                std::uint32_t readingCount, std::uint32_t nowMs);
  KegTrackingDecision update(std::uint32_t nowMs,
                             const models::ScaleState& scale,
                             bool identityConflict,
                             bool identityVerified);
  void completeRecord(bool saved, std::int32_t weightGrams,
                      std::uint32_t nowMs);

  KegTrackingState state() const;
  const char* activeKegId() const;
  std::int32_t baselineWeightGrams() const;
  std::uint32_t revision() const;

 private:
  void resetCandidate();
  void setState(KegTrackingState state);

  KegTrackingSettings settings_{};
  std::array<char, models::kKegIdBytes> activeKegId_{};
  std::int32_t baselineWeightGrams_{0};
  std::int32_t candidateWeightGrams_{0};
  std::uint32_t candidateSinceMs_{0U};
  std::uint32_t lastRecordAtMs_{0U};
  std::uint32_t lastObservedReadingCount_{0U};
  std::uint32_t revision_{0U};
  KegTrackingState state_{KegTrackingState::Idle};
  bool hasCandidate_{false};
  bool decisionIssued_{false};
  bool started_{false};
};

const char* kegTrackingStateName(KegTrackingState state);
const char* kegTrackingActionName(KegTrackingAction action);

}  // namespace keezer::services
