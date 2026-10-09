#include "services/ScaleService.h"

#include <cstring>

#include "diagnostics/Logger.h"
#include "scale/ScaleProtocol.h"

namespace keezer::services {
namespace {

constexpr char kLogTag[] = "SCALE";
constexpr std::size_t kMaximumPacketsPerUpdate = 8U;

const char* nfcText(const models::ScaleReading& reading) {
  return reading.hasNfcUid ? reading.nfcUid.data() : "NONE";
}

std::int64_t absoluteDifference(const std::int32_t lhs,
                                const std::int32_t rhs) {
  const std::int64_t difference =
      static_cast<std::int64_t>(lhs) - static_cast<std::int64_t>(rhs);
  return difference < 0 ? -difference : difference;
}

}  // namespace

ScaleService::ScaleService(scale::IScaleTransport& transport)
    : transport_(transport) {}

bool ScaleService::begin(const std::uint32_t nowMs,
                         const ScaleSettings& settings) {
  settings_ = settings;
  if (settings_.onlineTimeoutMs == 0U ||
      settings_.offlineTimeoutMs <= settings_.onlineTimeoutMs ||
      settings_.maximumWeightGrams <= 0 ||
      settings_.nfcMinimumConsecutiveReadings == 0U ||
      settings_.nfcStabilizationMs == 0U ||
      settings_.nfcAmbiguityWindowMs == 0U ||
      !weightFilter_.begin(
          {settings_.weightFilterWindowSize, settings_.weightDeadbandGrams,
           settings_.significantWeightChangeGrams})) {
    KEEZER_LOG_ERROR(kLogTag, "Invalid scale settings");
    return false;
  }
  state_ = {};
  latestStableReading_ = {};
  hasLatestStableReading_ = false;
  hasSenderUptime_ = false;
  lastSenderUptimeMs_ = 0U;
  nfcCandidateInitialized_ = false;
  nfcCandidatePresent_ = false;
  nfcCandidateUid_ = {};
  nfcCandidateCount_ = 0U;
  nfcCandidateSinceMs_ = 0U;
  nfcCandidateAlternations_ = 0U;
  nfcAmbiguityWindowStartedAtMs_ = 0U;
  revision_ = 1U;
  started_ = transport_.begin(nowMs);
  KEEZER_LOG_INFO(
      kLogTag,
      "Scale service started: source=%s state=OFFLINE filter=MEDIAN/%u deadband=%ldg significant=%ldg",
      started_ ? "READY" : "FAILED",
      static_cast<unsigned int>(settings_.weightFilterWindowSize),
      static_cast<long>(settings_.weightDeadbandGrams),
      static_cast<long>(settings_.significantWeightChangeGrams));
  return started_;
}

void ScaleService::update(const std::uint32_t nowMs) {
  if (!started_) {
    return;
  }
  transport_.update(nowMs);
  models::ScalePacket packet{};
  std::size_t consumed = 0U;
  while (consumed < kMaximumPacketsPerUpdate && transport_.tryRead(packet)) {
    consumePacket(packet, nowMs);
    ++consumed;
  }
  updateLinkStatus(nowMs);
}

const models::ScaleState& ScaleService::state() const { return state_; }

const models::ScaleReading* ScaleService::latestStableReading() const {
  return hasLatestStableReading_ ? &latestStableReading_ : nullptr;
}

std::uint32_t ScaleService::revision() const { return revision_; }

void ScaleService::consumePacket(const models::ScalePacket& packet,
                                 const std::uint32_t nowMs) {
  models::ScaleReading reading{};
  const scale::ScaleProtocolError validation = scale::ScaleProtocol::validate(
      packet, nowMs, settings_.maximumWeightGrams, reading);
  if (validation != scale::ScaleProtocolError::None) {
    ++state_.validationErrorCount;
    ++revision_;
    KEEZER_LOG_WARN(kLogTag, "Reading rejected: %s",
                    scale::scaleProtocolErrorName(validation));
    return;
  }

  const bool senderRestarted =
      hasSenderUptime_ && reading.senderUptimeMs < lastSenderUptimeMs_;
  if (state_.hasLastSequence && !senderRestarted &&
      reading.sequence <= state_.lastSequence) {
    ++state_.duplicateReadingCount;
    ++revision_;
    KEEZER_LOG_WARN(kLogTag, "Duplicate/old reading ignored: sequence=%lu",
                    static_cast<unsigned long>(reading.sequence));
    return;
  }
  if (senderRestarted) {
    KEEZER_LOG_INFO(kLogTag, "Scale sender restart detected");
  }

  const bool nfcChanged =
      state_.hasDetectedNfcUid != reading.hasNfcUid ||
      (reading.hasNfcUid &&
       std::strcmp(state_.detectedNfcUid.data(), reading.nfcUid.data()) != 0);
  state_.hasDetectedNfcUid = reading.hasNfcUid;
  state_.detectedNfcUid = reading.nfcUid;
  updateNfcDebounce(reading, nowMs);
  state_.hasRawWeight = true;
  state_.rawWeightGrams = reading.weightGrams;
  state_.stable = reading.stable;
  if (reading.stable) {
    const scale::WeightFilterResult filtered =
        weightFilter_.addSample(reading.weightGrams);
    state_.weightFilterSampleCount = weightFilter_.sampleCount();
    if (filtered.hasOutput) {
      state_.hasFilteredWeight = true;
      state_.filteredWeightGrams = filtered.outputGrams;
      latestStableReading_ = reading;
      latestStableReading_.weightGrams = filtered.outputGrams;
      hasLatestStableReading_ = true;
      if (filtered.outputChanged) {
        ++state_.filteredWeightUpdateCount;
      }
      if (filtered.significantChange) {
        ++state_.significantWeightChangeRevision;
        state_.significantWeightFromGrams = filtered.previousOutputGrams;
        state_.significantWeightToGrams = filtered.outputGrams;
        KEEZER_LOG_WARN(
            kLogTag,
            "SIGNIFICANT_WEIGHT_CHANGE from=%ldg to=%ldg delta=%ldg",
            static_cast<long>(filtered.previousOutputGrams),
            static_cast<long>(filtered.outputGrams),
            static_cast<long>(filtered.deltaGrams));
      }
      if (filtered.outputChanged ||
          absoluteDifference(reading.weightGrams, filtered.outputGrams) >=
              settings_.weightDeadbandGrams) {
        KEEZER_LOG_INFO(
            kLogTag,
            "WEIGHT_FILTER raw=%ldg filtered=%ldg samples=%u published=%s",
            static_cast<long>(reading.weightGrams),
            static_cast<long>(filtered.outputGrams),
            static_cast<unsigned int>(state_.weightFilterSampleCount),
            filtered.outputChanged ? "YES" : "NO");
      }
    }
  }
  state_.hasBatteryPercentage = reading.hasBatteryPercentage;
  state_.batteryPercentage = reading.batteryPercentage;
  state_.hasRssiDbm = reading.hasRssiDbm;
  state_.rssiDbm = reading.rssiDbm;
  state_.hasLastSequence = true;
  state_.lastSequence = reading.sequence;
  state_.hasLastUpdate = true;
  state_.lastUpdateMonotonicMs = reading.receivedAtMonotonicMs;
  ++state_.acceptedReadingCount;
  hasSenderUptime_ = true;
  lastSenderUptimeMs_ = reading.senderUptimeMs;
  setLinkStatus(models::ScaleLinkStatus::Online, nowMs);
  ++revision_;

  if (nfcChanged) {
    KEEZER_LOG_INFO(kLogTag, "NFC detected: %s", nfcText(reading));
  }
  if (state_.acceptedReadingCount == 1U ||
      (state_.acceptedReadingCount % 5U) == 0U) {
    if (state_.hasFilteredWeight) {
      KEEZER_LOG_INFO(
          kLogTag,
          "Weight received: raw=%ldg filtered=%ldg stable=%s nfc=%s battery=%u%% rssi=%d",
          static_cast<long>(reading.weightGrams),
          static_cast<long>(state_.filteredWeightGrams),
          reading.stable ? "YES" : "NO", nfcText(reading),
          static_cast<unsigned int>(reading.batteryPercentage),
          static_cast<int>(reading.rssiDbm));
    } else {
      KEEZER_LOG_INFO(
          kLogTag,
          "Weight received: raw=%ldg filtered=NONE stable=%s nfc=%s battery=%u%% rssi=%d",
          static_cast<long>(reading.weightGrams),
          reading.stable ? "YES" : "NO", nfcText(reading),
          static_cast<unsigned int>(reading.batteryPercentage),
          static_cast<int>(reading.rssiDbm));
    }
  }
}

void ScaleService::updateNfcDebounce(const models::ScaleReading& reading,
                                     const std::uint32_t nowMs) {
  const bool sameCandidate =
      nfcCandidateInitialized_ &&
      nfcCandidatePresent_ == reading.hasNfcUid &&
      (!reading.hasNfcUid ||
       std::strcmp(nfcCandidateUid_.data(), reading.nfcUid.data()) == 0);

  if (!sameCandidate) {
    if (nfcCandidateInitialized_) {
      if (nowMs - nfcAmbiguityWindowStartedAtMs_ >
          settings_.nfcAmbiguityWindowMs) {
        nfcAmbiguityWindowStartedAtMs_ = nowMs;
        nfcCandidateAlternations_ = 1U;
      } else if (nfcCandidateAlternations_ < 255U) {
        ++nfcCandidateAlternations_;
      }
      if (nfcCandidateAlternations_ >= 3U && !state_.nfcAmbiguous) {
        state_.nfcAmbiguous = true;
        KEEZER_LOG_WARN(kLogTag,
                        "NFC ambiguous: rapidly alternating readings");
      }
    } else {
      nfcAmbiguityWindowStartedAtMs_ = nowMs;
    }
    nfcCandidateInitialized_ = true;
    nfcCandidatePresent_ = reading.hasNfcUid;
    nfcCandidateUid_ = reading.nfcUid;
    nfcCandidateCount_ = 1U;
    nfcCandidateSinceMs_ = nowMs;
  } else if (nfcCandidateCount_ < 255U) {
    ++nfcCandidateCount_;
  }

  const bool stabilized =
      nfcCandidateCount_ >= settings_.nfcMinimumConsecutiveReadings ||
      nowMs - nfcCandidateSinceMs_ >= settings_.nfcStabilizationMs;
  if (!stabilized) {
    return;
  }

  const bool changed =
      state_.hasStableNfcUid != nfcCandidatePresent_ ||
      (nfcCandidatePresent_ &&
       std::strcmp(state_.stableNfcUid.data(), nfcCandidateUid_.data()) != 0);
  if (!changed) {
    state_.nfcAmbiguous = false;
    nfcCandidateAlternations_ = 0U;
    nfcAmbiguityWindowStartedAtMs_ = nowMs;
    return;
  }

  state_.hasStableNfcUid = nfcCandidatePresent_;
  state_.stableNfcUid = nfcCandidatePresent_
                            ? nfcCandidateUid_
                            : std::array<char, models::kScaleNfcUidBytes>{};
  state_.nfcAmbiguous = false;
  nfcCandidateAlternations_ = 0U;
  nfcAmbiguityWindowStartedAtMs_ = nowMs;
  ++state_.nfcEventRevision;
  if (state_.hasStableNfcUid) {
    KEEZER_LOG_INFO(kLogTag, "NFC stable: %s",
                    state_.stableNfcUid.data());
  } else {
    KEEZER_LOG_INFO(kLogTag, "NFC removed");
  }
}

void ScaleService::updateLinkStatus(const std::uint32_t nowMs) {
  if (!state_.hasLastUpdate) {
    setLinkStatus(models::ScaleLinkStatus::Offline, nowMs);
    return;
  }
  const std::uint32_t ageMs =
      nowMs - static_cast<std::uint32_t>(state_.lastUpdateMonotonicMs);
  if (ageMs < settings_.onlineTimeoutMs) {
    setLinkStatus(models::ScaleLinkStatus::Online, nowMs);
  } else if (ageMs < settings_.offlineTimeoutMs) {
    setLinkStatus(models::ScaleLinkStatus::Stale, nowMs);
  } else {
    setLinkStatus(models::ScaleLinkStatus::Offline, nowMs);
  }
}

void ScaleService::setLinkStatus(const models::ScaleLinkStatus status,
                                 const std::uint32_t nowMs) {
  if (state_.linkStatus == status) {
    return;
  }
  state_.linkStatus = status;
  ++revision_;
  const std::uint32_t ageMs =
      state_.hasLastUpdate
          ? nowMs - static_cast<std::uint32_t>(state_.lastUpdateMonotonicMs)
          : 0U;
  KEEZER_LOG_INFO(kLogTag, "Link state=%s age=%lu ms transport=%s",
                  models::scaleLinkStatusName(status),
                  static_cast<unsigned long>(ageMs),
                  transport_.isConnected() ? "UP" : "DOWN");
}

}  // namespace keezer::services
