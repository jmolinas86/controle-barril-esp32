#include "services/KegTrackingService.h"

#include <cstdio>
#include <cstring>

namespace keezer::services {
namespace {

std::int64_t absoluteDifference(const std::int32_t lhs,
                                const std::int32_t rhs) {
  const std::int64_t difference =
      static_cast<std::int64_t>(lhs) - static_cast<std::int64_t>(rhs);
  return difference < 0 ? -difference : difference;
}

}  // namespace

bool KegTrackingService::begin(const KegTrackingSettings& settings) {
  if (settings.minimumConsumptionGrams <= 0 ||
      settings.stableConfirmationMs == 0U ||
      settings.minimumRecordIntervalMs == 0U ||
      settings.suspiciousIncreaseGrams <= settings.candidateDeadbandGrams ||
      settings.suspiciousChangeGrams <= settings.minimumConsumptionGrams ||
      settings.removalWeightThresholdGrams < 0 ||
      settings.candidateDeadbandGrams <= 0) {
    return false;
  }
  settings_ = settings;
  activeKegId_ = {};
  baselineWeightGrams_ = 0;
  lastRecordAtMs_ = 0U;
  lastObservedReadingCount_ = 0U;
  resetCandidate();
  decisionIssued_ = false;
  state_ = KegTrackingState::Idle;
  revision_ = 1U;
  started_ = true;
  return true;
}

void KegTrackingService::observeActiveKeg(
    const char* const kegId, const std::int32_t baselineWeightGrams,
    const std::uint32_t nowMs) {
  if (!started_) return;
  const char* const normalized = kegId == nullptr ? "" : kegId;
  if (std::strcmp(activeKegId_.data(), normalized) == 0) return;

  std::snprintf(activeKegId_.data(), activeKegId_.size(), "%s", normalized);
  baselineWeightGrams_ = baselineWeightGrams < 0 ? 0 : baselineWeightGrams;
  lastRecordAtMs_ = nowMs;
  lastObservedReadingCount_ = 0U;
  resetCandidate();
  decisionIssued_ = false;
  setState(activeKegId_[0] == '\0' ? KegTrackingState::Idle
                                    : KegTrackingState::ChangePending);
}

void KegTrackingService::activate(const char* const kegId,
                                  const std::int32_t baselineWeightGrams,
                                  const std::uint32_t readingCount,
                                  const std::uint32_t nowMs) {
  if (!started_ || kegId == nullptr || kegId[0] == '\0' ||
      baselineWeightGrams < 0) {
    return;
  }
  std::snprintf(activeKegId_.data(), activeKegId_.size(), "%s", kegId);
  baselineWeightGrams_ = baselineWeightGrams;
  lastRecordAtMs_ = nowMs;
  lastObservedReadingCount_ = readingCount;
  resetCandidate();
  decisionIssued_ = false;
  setState(KegTrackingState::Monitoring);
}

KegTrackingDecision KegTrackingService::update(
    const std::uint32_t nowMs, const models::ScaleState& scale,
    const bool identityConflict, const bool identityVerified) {
  KegTrackingDecision decision{};
  if (!started_ || activeKegId_[0] == '\0') return decision;
  if (state_ == KegTrackingState::ChangePending || decisionIssued_) {
    return decision;
  }

  if (scale.linkStatus != models::ScaleLinkStatus::Online) {
    resetCandidate();
    setState(KegTrackingState::Paused);
    return decision;
  }
  if (identityConflict || scale.nfcAmbiguous) {
    resetCandidate();
    decisionIssued_ = true;
    setState(KegTrackingState::ChangePending);
    decision.action = KegTrackingAction::ConfirmKeg;
    return decision;
  }
  if (!scale.stable || !scale.hasFilteredWeight) {
    resetCandidate();
    setState(KegTrackingState::Acquiring);
    return decision;
  }
  if (scale.acceptedReadingCount == lastObservedReadingCount_) {
    return decision;
  }
  lastObservedReadingCount_ = scale.acceptedReadingCount;
  const std::int32_t weight = scale.filteredWeightGrams;
  if (weight < settings_.removalWeightThresholdGrams) {
    resetCandidate();
    setState(KegTrackingState::Acquiring);
    return decision;
  }

  if (!hasCandidate_ ||
      absoluteDifference(weight, candidateWeightGrams_) >=
          settings_.candidateDeadbandGrams) {
    candidateWeightGrams_ = weight;
    candidateSinceMs_ = nowMs;
    hasCandidate_ = true;
    setState(KegTrackingState::Acquiring);
    return decision;
  }
  if (nowMs - candidateSinceMs_ < settings_.stableConfirmationMs) {
    return decision;
  }

  const std::int64_t increase =
      static_cast<std::int64_t>(candidateWeightGrams_) -
      static_cast<std::int64_t>(baselineWeightGrams_);
  const std::int64_t consumption =
      static_cast<std::int64_t>(baselineWeightGrams_) -
      static_cast<std::int64_t>(candidateWeightGrams_);
  if (increase >= settings_.suspiciousIncreaseGrams ||
      (!identityVerified &&
       absoluteDifference(candidateWeightGrams_, baselineWeightGrams_) >=
           settings_.suspiciousChangeGrams)) {
    decisionIssued_ = true;
    setState(KegTrackingState::ChangePending);
    decision.action = KegTrackingAction::ConfirmKeg;
    decision.weightGrams = candidateWeightGrams_;
    return decision;
  }

  setState(KegTrackingState::Monitoring);
  if (consumption >= settings_.minimumConsumptionGrams &&
      nowMs - lastRecordAtMs_ >= settings_.minimumRecordIntervalMs) {
    decisionIssued_ = true;
    decision.action = KegTrackingAction::RecordConsumption;
    decision.weightGrams = candidateWeightGrams_;
  }
  return decision;
}

void KegTrackingService::completeRecord(const bool saved,
                                        const std::int32_t weightGrams,
                                        const std::uint32_t nowMs) {
  if (!started_ || !decisionIssued_) return;
  decisionIssued_ = false;
  if (!saved) {
    resetCandidate();
    setState(KegTrackingState::ChangePending);
    return;
  }
  baselineWeightGrams_ = weightGrams;
  lastRecordAtMs_ = nowMs;
  candidateWeightGrams_ = weightGrams;
  candidateSinceMs_ = nowMs;
  hasCandidate_ = true;
  setState(KegTrackingState::Monitoring);
}

KegTrackingState KegTrackingService::state() const { return state_; }

const char* KegTrackingService::activeKegId() const {
  return activeKegId_.data();
}

std::int32_t KegTrackingService::baselineWeightGrams() const {
  return baselineWeightGrams_;
}

std::uint32_t KegTrackingService::revision() const { return revision_; }

void KegTrackingService::resetCandidate() {
  candidateWeightGrams_ = 0;
  candidateSinceMs_ = 0U;
  hasCandidate_ = false;
}

void KegTrackingService::setState(const KegTrackingState state) {
  if (state_ == state) return;
  state_ = state;
  ++revision_;
}

const char* kegTrackingStateName(const KegTrackingState state) {
  switch (state) {
    case KegTrackingState::Idle:
      return "IDLE";
    case KegTrackingState::Acquiring:
      return "ACQUIRING";
    case KegTrackingState::Monitoring:
      return "MONITORING";
    case KegTrackingState::Paused:
      return "PAUSED";
    case KegTrackingState::ChangePending:
      return "CHANGE_PENDING";
  }
  return "IDLE";
}

const char* kegTrackingActionName(const KegTrackingAction action) {
  switch (action) {
    case KegTrackingAction::None:
      return "NONE";
    case KegTrackingAction::RecordConsumption:
      return "RECORD_CONSUMPTION";
    case KegTrackingAction::ConfirmKeg:
      return "CONFIRM_KEG";
  }
  return "NONE";
}

}  // namespace keezer::services
