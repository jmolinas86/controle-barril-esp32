#pragma once

#include <array>
#include <cstdint>

#include "models/Keg.h"
#include "models/KegMeasurement.h"

namespace keezer::models {

inline constexpr std::uint8_t kHistoryFormatVersion = 1U;

enum class HistorySource : std::uint8_t {
  Nfc = 0U,
  Manual = 1U,
  AutomaticTracking = 2U,
};

struct HistoryRecord final {
  std::array<char, kKegIdBytes> kegId{};
  std::uint32_t sequence{0U};
  std::uint32_t bootId{0U};
  std::int64_t utcSeconds{-1};
  std::uint32_t monotonicMs{0U};
  std::int32_t rawWeightGrams{0};
  std::int32_t filteredWeightGrams{0};
  std::uint32_t volumeMl{0U};
  std::uint16_t percentageBasisPoints{0U};
  HistorySource source{HistorySource::Nfc};
  MeasurementValidity validity{MeasurementValidity::InvalidWeight};
  std::uint8_t flags{0U};
};

}  // namespace keezer::models
