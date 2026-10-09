#include "scale/ScaleProtocol.h"

#include <cstddef>
#include <cstring>

namespace keezer::scale {
namespace {

bool isSeparator(const char value) {
  return value == ':' || value == '-' || value == ' ' || value == '\t';
}

char normalizedHex(const char value) {
  if (value >= '0' && value <= '9') {
    return value;
  }
  if (value >= 'a' && value <= 'f') {
    return static_cast<char>(value - 'a' + 'A');
  }
  if (value >= 'A' && value <= 'F') {
    return value;
  }
  return '\0';
}

bool normalizeNfcUid(
    const std::array<char, models::kScaleNfcUidBytes>& source,
    std::array<char, models::kScaleNfcUidBytes>& destination) {
  destination = {};
  std::size_t written = 0U;
  for (const char value : source) {
    if (value == '\0') {
      break;
    }
    if (isSeparator(value)) {
      continue;
    }
    const char digit = normalizedHex(value);
    if (digit == '\0' || written + 1U >= destination.size()) {
      destination = {};
      return false;
    }
    destination[written++] = digit;
  }
  if (written < 2U || (written % 2U) != 0U) {
    destination = {};
    return false;
  }
  destination[written] = '\0';
  return true;
}

}  // namespace

ScaleProtocolError ScaleProtocol::validate(
    const models::ScalePacket& packet,
    const std::uint64_t receivedAtMonotonicMs,
    const std::int32_t maximumWeightGrams, models::ScaleReading& reading) {
  reading = {};
  if (packet.protocolVersion != models::kScaleProtocolVersion) {
    return ScaleProtocolError::UnsupportedVersion;
  }
  if (std::strncmp(packet.scaleId.data(), models::kMainScaleId,
                   packet.scaleId.size()) != 0) {
    return ScaleProtocolError::InvalidScaleId;
  }
  if (packet.weightGrams < 0 || packet.weightGrams > maximumWeightGrams) {
    return ScaleProtocolError::WeightOutOfRange;
  }
  if (packet.hasBatteryPercentage && packet.batteryPercentage > 100U) {
    return ScaleProtocolError::BatteryOutOfRange;
  }
  if (packet.hasRssiDbm && packet.rssiDbm > 0) {
    return ScaleProtocolError::RssiOutOfRange;
  }

  reading.protocolVersion = packet.protocolVersion;
  reading.sequence = packet.sequence;
  reading.scaleId = packet.scaleId;
  reading.hasNfcUid = packet.hasNfcUid;
  if (packet.hasNfcUid && !normalizeNfcUid(packet.nfcUid, reading.nfcUid)) {
    reading = {};
    return ScaleProtocolError::InvalidNfcUid;
  }
  reading.weightGrams = packet.weightGrams;
  reading.stable = packet.stable;
  reading.hasBatteryPercentage = packet.hasBatteryPercentage;
  reading.batteryPercentage = packet.batteryPercentage;
  reading.hasRssiDbm = packet.hasRssiDbm;
  reading.rssiDbm = packet.rssiDbm;
  reading.senderUptimeMs = packet.senderUptimeMs;
  reading.receivedAtMonotonicMs = receivedAtMonotonicMs;
  return ScaleProtocolError::None;
}

const char* scaleProtocolErrorName(const ScaleProtocolError error) {
  switch (error) {
    case ScaleProtocolError::None:
      return "NONE";
    case ScaleProtocolError::UnsupportedVersion:
      return "UNSUPPORTED_VERSION";
    case ScaleProtocolError::InvalidScaleId:
      return "INVALID_SCALE_ID";
    case ScaleProtocolError::InvalidNfcUid:
      return "INVALID_NFC_UID";
    case ScaleProtocolError::WeightOutOfRange:
      return "WEIGHT_OUT_OF_RANGE";
    case ScaleProtocolError::BatteryOutOfRange:
      return "BATTERY_OUT_OF_RANGE";
    case ScaleProtocolError::RssiOutOfRange:
      return "RSSI_OUT_OF_RANGE";
  }
  return "UNKNOWN";
}

}  // namespace keezer::scale
