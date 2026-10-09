#include "scale/ScaleHttpJson.h"

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace keezer::scale {
namespace {

const char* skipWhitespace(const char* cursor) {
  while (cursor != nullptr &&
         (*cursor == ' ' || *cursor == '\t' || *cursor == '\r' ||
          *cursor == '\n')) {
    ++cursor;
  }
  return cursor;
}

bool isValueTerminator(const char* cursor) {
  cursor = skipWhitespace(cursor);
  return cursor != nullptr && (*cursor == ',' || *cursor == '}');
}

const char* valueFor(const char* const json, const char* const key) {
  if (json == nullptr || key == nullptr) return nullptr;
  const char* cursor = std::strstr(json, key);
  if (cursor == nullptr) return nullptr;
  cursor = std::strchr(cursor + std::strlen(key), ':');
  if (cursor == nullptr) return nullptr;
  ++cursor;
  return skipWhitespace(cursor);
}

bool parseUnsigned(const char* const json, const char* const key,
                   std::uint64_t& output) {
  const char* value = valueFor(json, key);
  if (value == nullptr || *value < '0' || *value > '9') return false;
  errno = 0;
  char* end = nullptr;
  const unsigned long long parsed = std::strtoull(value, &end, 10);
  if (errno != 0 || end == value || !isValueTerminator(end)) return false;
  output = static_cast<std::uint64_t>(parsed);
  return true;
}

bool parseSigned(const char* const json, const char* const key,
                 std::int64_t& output) {
  const char* value = valueFor(json, key);
  if (value == nullptr || (*value != '-' && (*value < '0' || *value > '9'))) {
    return false;
  }
  errno = 0;
  char* end = nullptr;
  const long long parsed = std::strtoll(value, &end, 10);
  if (errno != 0 || end == value || !isValueTerminator(end)) return false;
  output = static_cast<std::int64_t>(parsed);
  return true;
}

bool parseBoolean(const char* const json, const char* const key, bool& output) {
  const char* value = valueFor(json, key);
  if (value == nullptr) return false;
  if (std::strncmp(value, "true", 4U) == 0 &&
      isValueTerminator(value + 4U)) {
    output = true;
    return true;
  }
  if (std::strncmp(value, "false", 5U) == 0 &&
      isValueTerminator(value + 5U)) {
    output = false;
    return true;
  }
  return false;
}

template <std::size_t Size>
bool parseString(const char* const json, const char* const key,
                 std::array<char, Size>& output) {
  output = {};
  const char* value = valueFor(json, key);
  if (value == nullptr || *value != '\"') return false;
  ++value;
  std::size_t written = 0U;
  while (*value != '\0' && *value != '\"') {
    if (*value == '\\') {
      ++value;
      if (*value == '\0') return false;
    }
    if (written + 1U >= output.size()) {
      output = {};
      return false;
    }
    output[written++] = *value++;
  }
  if (*value != '\"' || !isValueTerminator(value + 1U)) {
    output = {};
    return false;
  }
  output[written] = '\0';
  return true;
}

}  // namespace

bool ScaleHttpJson::parseReading(const char* const json,
                                 models::ScalePacket& packet) {
  packet = {};
  std::uint64_t unsignedValue = 0U;
  std::int64_t signedValue = 0;
  if (!parseUnsigned(json, "\"protocol_version\"", unsignedValue) ||
      unsignedValue > std::numeric_limits<std::uint8_t>::max()) {
    return false;
  }
  packet.protocolVersion = static_cast<std::uint8_t>(unsignedValue);
  if (!parseUnsigned(json, "\"sequence\"", unsignedValue) ||
      unsignedValue > std::numeric_limits<std::uint32_t>::max()) {
    return false;
  }
  packet.sequence = static_cast<std::uint32_t>(unsignedValue);
  if (!parseString(json, "\"scale_id\"", packet.scaleId)) return false;
  if (!parseSigned(json, "\"weight_grams\"", signedValue) ||
      signedValue < std::numeric_limits<std::int32_t>::min() ||
      signedValue > std::numeric_limits<std::int32_t>::max()) {
    return false;
  }
  packet.weightGrams = static_cast<std::int32_t>(signedValue);
  if (!parseBoolean(json, "\"stable\"", packet.stable)) return false;

  const char* nfc = valueFor(json, "\"nfc_uid\"");
  if (nfc == nullptr) return false;
  if (std::strncmp(nfc, "null", 4U) == 0 &&
      isValueTerminator(nfc + 4U)) {
    packet.hasNfcUid = false;
  } else {
    packet.hasNfcUid = true;
    if (!parseString(json, "\"nfc_uid\"", packet.nfcUid)) return false;
  }

  if (!parseSigned(json, "\"rssi_dbm\"", signedValue) ||
      signedValue < std::numeric_limits<std::int8_t>::min() ||
      signedValue > std::numeric_limits<std::int8_t>::max()) {
    return false;
  }
  packet.hasRssiDbm = true;
  packet.rssiDbm = static_cast<std::int8_t>(signedValue);
  if (!parseUnsigned(json, "\"uptime_ms\"", unsignedValue)) return false;
  packet.senderUptimeMs = unsignedValue;
  packet.hasBatteryPercentage = false;
  return true;
}

}  // namespace keezer::scale
