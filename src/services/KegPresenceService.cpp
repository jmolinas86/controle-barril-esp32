#include "services/KegPresenceService.h"

#include <cstdio>
#include <cstring>

namespace keezer::services {

bool KegPresenceService::begin(const KegPresenceSettings& settings) {
  if (settings.removalWeightThresholdGrams < 0 ||
      settings.significantDropGrams <= 0 ||
      settings.strongConfirmationMs == 0U ||
      settings.moderateConfirmationMs <= settings.strongConfirmationMs) {
    return false;
  }
  settings_ = settings;
  activeKegId_ = {};
  referenceWeightGrams_ = 0;
  state_ = KegPresenceState::None;
  resetEvidence();
  revision_ = 1U;
  started_ = true;
  return true;
}

void KegPresenceService::synchronizeActiveKeg(
    const char* const activeKegId, const std::int32_t referenceWeightGrams,
    const std::uint32_t nowMs) {
  (void)nowMs;
  if (!started_) {
    return;
  }
  const char* const normalizedId = activeKegId == nullptr ? "" : activeKegId;
  if (std::strcmp(activeKegId_.data(), normalizedId) == 0) {
    if (referenceWeightGrams > referenceWeightGrams_) {
      referenceWeightGrams_ = referenceWeightGrams;
    }
    return;
  }

  std::snprintf(activeKegId_.data(), activeKegId_.size(), "%s",
                normalizedId);
  referenceWeightGrams_ = referenceWeightGrams;
  resetEvidence();
  setState(activeKegId_[0] == '\0' ? KegPresenceState::None
                                    : KegPresenceState::Active);
}

RemovalDecision KegPresenceService::update(
    const std::uint32_t nowMs, const models::ScaleState& scale) {
  if (!started_ || activeKegId_[0] == '\0' || decisionIssued_) {
    return RemovalDecision::None;
  }

  if (scale.linkStatus != models::ScaleLinkStatus::Online) {
    resetEvidence();
    setState(KegPresenceState::Uncertain);
    return RemovalDecision::None;
  }

  if (scale.hasStableNfcUid) {
    resetEvidence();
    setState(KegPresenceState::Active);
    return RemovalDecision::None;
  }

  const bool validStableWeight = scale.stable && scale.hasFilteredWeight;
  const bool weightLow =
      validStableWeight &&
      scale.filteredWeightGrams < settings_.removalWeightThresholdGrams;
  if (weightLow) {
    setState(KegPresenceState::RemovalSuspected);
    if (!lowWeightTiming_) {
      lowWeightTiming_ = true;
      lowWeightSinceMs_ = nowMs;
    }
    if (nowMs - lowWeightSinceMs_ >= settings_.strongConfirmationMs) {
      decisionIssued_ = true;
      return RemovalDecision::ConfirmedStrong;
    }
  } else {
    lowWeightTiming_ = false;
    setState(KegPresenceState::Active);
  }

  if (!settings_.allowSignificantDropRemovalWithoutNfc) {
    return RemovalDecision::None;
  }
  if (!nfcAbsenceTiming_) {
    nfcAbsenceTiming_ = true;
    nfcAbsentSinceMs_ = nowMs;
  }
  const bool significantDrop =
      validStableWeight && referenceWeightGrams_ > settings_.significantDropGrams &&
      scale.filteredWeightGrams <=
          referenceWeightGrams_ - settings_.significantDropGrams;
  if (significantDrop &&
      nowMs - nfcAbsentSinceMs_ >= settings_.moderateConfirmationMs) {
    setState(KegPresenceState::RemovalSuspected);
    decisionIssued_ = true;
    return RemovalDecision::ConfirmedModerate;
  }
  return RemovalDecision::None;
}

KegPresenceState KegPresenceService::state() const { return state_; }

const char* KegPresenceService::activeKegId() const {
  return activeKegId_.data();
}

std::uint32_t KegPresenceService::revision() const { return revision_; }

void KegPresenceService::setState(const KegPresenceState state) {
  if (state_ == state) {
    return;
  }
  state_ = state;
  ++revision_;
}

void KegPresenceService::resetEvidence() {
  nfcAbsentSinceMs_ = 0U;
  lowWeightSinceMs_ = 0U;
  nfcAbsenceTiming_ = false;
  lowWeightTiming_ = false;
  decisionIssued_ = false;
}

const char* kegPresenceStateName(const KegPresenceState state) {
  switch (state) {
    case KegPresenceState::None:
      return "NONE";
    case KegPresenceState::Active:
      return "ACTIVE";
    case KegPresenceState::RemovalSuspected:
      return "REMOVAL_SUSPECTED";
    case KegPresenceState::Uncertain:
      return "UNCERTAIN";
  }
  return "NONE";
}

const char* removalDecisionName(const RemovalDecision decision) {
  switch (decision) {
    case RemovalDecision::None:
      return "NONE";
    case RemovalDecision::ConfirmedStrong:
      return "STRONG";
    case RemovalDecision::ConfirmedModerate:
      return "MODERATE";
  }
  return "NONE";
}

}  // namespace keezer::services
