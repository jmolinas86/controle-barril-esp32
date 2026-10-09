#pragma once

#include <cstdint>

namespace keezer::models {

enum class MeasurementValidity : std::uint8_t {
  Valid,
  BelowTare,
  AboveExpectedMaximum,
  InvalidWeight,
  InvalidKegConfiguration,
};

struct KegMeasurement final {
  std::int32_t filteredWeightGrams{0};
  std::uint32_t beerWeightGrams{0U};
  std::uint32_t unclampedVolumeMl{0U};
  std::uint32_t volumeMl{0U};
  std::uint16_t percentageBasisPoints{0U};
  MeasurementValidity validity{MeasurementValidity::InvalidWeight};
};

const char* measurementValidityName(MeasurementValidity validity);

}  // namespace keezer::models
