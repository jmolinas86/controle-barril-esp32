#pragma once

#include <cstdint>

#include "models/Scale.h"
#include "scale/IScaleTransport.h"
#include "scale/WeightFilter.h"

namespace keezer::services {

struct ScaleSettings {
  std::uint32_t onlineTimeoutMs{30'000U};
  std::uint32_t offlineTimeoutMs{300'000U};
  std::int32_t maximumWeightGrams{100'000};
  std::uint8_t nfcMinimumConsecutiveReadings{3U};
  std::uint32_t nfcStabilizationMs{1'500U};
  std::uint32_t nfcAmbiguityWindowMs{1'500U};
  std::uint8_t weightFilterWindowSize{5U};
  std::int32_t weightDeadbandGrams{20};
  std::int32_t significantWeightChangeGrams{2'000};
};

class ScaleService final {
 public:
  explicit ScaleService(scale::IScaleTransport& transport);

  bool begin(std::uint32_t nowMs, const ScaleSettings& settings = {});
  void update(std::uint32_t nowMs);
  const models::ScaleState& state() const;
  const models::ScaleReading* latestStableReading() const;
  std::uint32_t revision() const;

 private:
  void consumePacket(const models::ScalePacket& packet, std::uint32_t nowMs);
  void updateNfcDebounce(const models::ScaleReading& reading,
                         std::uint32_t nowMs);
  void updateLinkStatus(std::uint32_t nowMs);
  void setLinkStatus(models::ScaleLinkStatus status, std::uint32_t nowMs);

  scale::IScaleTransport& transport_;
  scale::WeightFilter weightFilter_;
  ScaleSettings settings_{};
  models::ScaleState state_{};
  models::ScaleReading latestStableReading_{};
  bool hasLatestStableReading_{false};
  bool hasSenderUptime_{false};
  std::uint64_t lastSenderUptimeMs_{0U};
  bool nfcCandidateInitialized_{false};
  bool nfcCandidatePresent_{false};
  std::array<char, models::kScaleNfcUidBytes> nfcCandidateUid_{};
  std::uint8_t nfcCandidateCount_{0U};
  std::uint32_t nfcCandidateSinceMs_{0U};
  std::uint8_t nfcCandidateAlternations_{0U};
  std::uint32_t nfcAmbiguityWindowStartedAtMs_{0U};
  std::uint32_t revision_{0U};
  bool started_{false};
};

}  // namespace keezer::services
