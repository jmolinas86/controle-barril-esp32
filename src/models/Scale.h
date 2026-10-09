#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace keezer::models {

inline constexpr std::uint8_t kScaleProtocolVersion = 1U;
inline constexpr char kMainScaleId[] = "SCALE_MAIN";
inline constexpr std::size_t kScaleIdBytes = 16U;
inline constexpr std::size_t kScaleNfcUidBytes = 21U;

enum class ScaleLinkStatus : std::uint8_t {
  Offline = 0U,
  Stale,
  Online,
};

const char* scaleLinkStatusName(ScaleLinkStatus status);

// Transport-neutral packet. A real HTTP, ESP-NOW or MQTT adapter will fill
// this same fixed-size structure before handing it to ScaleProtocol.
struct ScalePacket {
  std::uint8_t protocolVersion{kScaleProtocolVersion};
  std::uint32_t sequence{0U};
  std::array<char, kScaleIdBytes> scaleId{};
  bool hasNfcUid{false};
  std::array<char, kScaleNfcUidBytes> nfcUid{};
  std::int32_t weightGrams{0};
  bool stable{false};
  bool hasBatteryPercentage{false};
  std::uint8_t batteryPercentage{0U};
  bool hasRssiDbm{false};
  std::int8_t rssiDbm{0};
  std::uint64_t senderUptimeMs{0U};
};

// Immutable, normalized representation of an accepted packet.
struct ScaleReading {
  std::uint8_t protocolVersion{kScaleProtocolVersion};
  std::uint32_t sequence{0U};
  std::array<char, kScaleIdBytes> scaleId{};
  bool hasNfcUid{false};
  std::array<char, kScaleNfcUidBytes> nfcUid{};
  std::int32_t weightGrams{0};
  bool stable{false};
  bool hasBatteryPercentage{false};
  std::uint8_t batteryPercentage{0U};
  bool hasRssiDbm{false};
  std::int8_t rssiDbm{0};
  std::uint64_t senderUptimeMs{0U};
  std::uint64_t receivedAtMonotonicMs{0U};
};

struct ScaleState {
  ScaleLinkStatus linkStatus{ScaleLinkStatus::Offline};
  bool hasDetectedNfcUid{false};
  std::array<char, kScaleNfcUidBytes> detectedNfcUid{};
  bool hasStableNfcUid{false};
  std::array<char, kScaleNfcUidBytes> stableNfcUid{};
  bool nfcAmbiguous{false};
  std::uint32_t nfcEventRevision{0U};
  bool hasRawWeight{false};
  std::int32_t rawWeightGrams{0};
  bool hasFilteredWeight{false};
  std::int32_t filteredWeightGrams{0};
  std::uint8_t weightFilterSampleCount{0U};
  std::uint32_t filteredWeightUpdateCount{0U};
  std::uint32_t significantWeightChangeRevision{0U};
  std::int32_t significantWeightFromGrams{0};
  std::int32_t significantWeightToGrams{0};
  bool stable{false};
  bool hasBatteryPercentage{false};
  std::uint8_t batteryPercentage{0U};
  bool hasRssiDbm{false};
  std::int8_t rssiDbm{0};
  bool hasLastSequence{false};
  std::uint32_t lastSequence{0U};
  bool hasLastUpdate{false};
  std::uint64_t lastUpdateMonotonicMs{0U};
  std::uint32_t acceptedReadingCount{0U};
  std::uint32_t duplicateReadingCount{0U};
  std::uint32_t validationErrorCount{0U};
};

}  // namespace keezer::models
