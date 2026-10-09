#pragma once

#include <cstdint>

#include "models/Keg.h"
#include "models/KegMeasurement.h"

namespace keezer::domain {

class KegCalculator final {
 public:
  static bool validateWeight(std::int32_t filteredWeightGrams);
  static bool validateConfiguration(const models::Keg& keg);
  static bool validateConfiguration(std::uint32_t capacityMl,
                                    std::int32_t tareGrams,
                                    std::uint16_t densityGramsPerLiter);
  static std::uint32_t calculateBeerWeight(
      std::int32_t filteredWeightGrams, std::int32_t tareGrams);
  static std::uint32_t calculateVolume(
      std::uint32_t beerWeightGrams,
      std::uint16_t densityGramsPerLiter,
      std::uint32_t capacityMl);
  static std::uint16_t calculatePercentage(std::uint32_t volumeMl,
                                           std::uint32_t capacityMl);
  static models::KegMeasurement calculate(const models::Keg& keg,
                                           std::int32_t filteredWeightGrams);
};

}  // namespace keezer::domain
