#include "domain/KegCalculator.h"

#include <algorithm>

namespace keezer::domain {

namespace {

constexpr std::int32_t kMaximumPhysicalWeightGrams = 100'000;
constexpr std::uint32_t kMinimumCapacityMl = 1'000U;
constexpr std::uint32_t kMaximumCapacityMl = 100'000U;
constexpr std::uint16_t kMinimumDensityGramsPerLiter = 900U;
constexpr std::uint16_t kMaximumDensityGramsPerLiter = 1'300U;
constexpr std::uint32_t kOverfillToleranceGrams = 500U;

}  // namespace

bool KegCalculator::validateWeight(const std::int32_t filteredWeightGrams) {
  return filteredWeightGrams >= 0 &&
         filteredWeightGrams <= kMaximumPhysicalWeightGrams;
}

bool KegCalculator::validateConfiguration(const models::Keg& keg) {
  return validateConfiguration(keg.capacityMl, keg.tareGrams,
                               keg.densityGramsPerLiter);
}

bool KegCalculator::validateConfiguration(
    const std::uint32_t capacityMl, const std::int32_t tareGrams,
    const std::uint16_t densityGramsPerLiter) {
  return tareGrams >= 0 && tareGrams < kMaximumPhysicalWeightGrams &&
         capacityMl >= kMinimumCapacityMl &&
         capacityMl <= kMaximumCapacityMl &&
         densityGramsPerLiter >= kMinimumDensityGramsPerLiter &&
         densityGramsPerLiter <= kMaximumDensityGramsPerLiter;
}

std::uint32_t KegCalculator::calculateBeerWeight(
    const std::int32_t filteredWeightGrams, const std::int32_t tareGrams) {
  return filteredWeightGrams <= tareGrams
             ? 0U
             : static_cast<std::uint32_t>(filteredWeightGrams - tareGrams);
}

std::uint32_t KegCalculator::calculateVolume(
    const std::uint32_t beerWeightGrams,
    const std::uint16_t densityGramsPerLiter,
    const std::uint32_t capacityMl) {
  if (densityGramsPerLiter == 0U || capacityMl == 0U) {
    return 0U;
  }
  const std::uint64_t calculatedVolume =
      (static_cast<std::uint64_t>(beerWeightGrams) * 1'000ULL) /
      densityGramsPerLiter;
  return static_cast<std::uint32_t>(
      std::min<std::uint64_t>(calculatedVolume, capacityMl));
}

std::uint16_t KegCalculator::calculatePercentage(
    const std::uint32_t volumeMl, const std::uint32_t capacityMl) {
  if (capacityMl == 0U) {
    return 0U;
  }
  const std::uint64_t boundedVolume =
      std::min<std::uint64_t>(volumeMl, capacityMl);
  return static_cast<std::uint16_t>(
      (boundedVolume * 10'000ULL) / capacityMl);
}

models::KegMeasurement KegCalculator::calculate(
    const models::Keg& keg, const std::int32_t filteredWeightGrams) {
  models::KegMeasurement result{};
  result.filteredWeightGrams = filteredWeightGrams;
  if (!validateWeight(filteredWeightGrams)) {
    result.validity = models::MeasurementValidity::InvalidWeight;
    return result;
  }
  if (!validateConfiguration(keg)) {
    result.validity =
        models::MeasurementValidity::InvalidKegConfiguration;
    return result;
  }

  result.beerWeightGrams = calculateBeerWeight(filteredWeightGrams,
                                                keg.tareGrams);
  const std::uint64_t calculatedVolume =
      (static_cast<std::uint64_t>(result.beerWeightGrams) * 1'000ULL) /
      keg.densityGramsPerLiter;
  result.unclampedVolumeMl = static_cast<std::uint32_t>(calculatedVolume);
  result.volumeMl = calculateVolume(result.beerWeightGrams,
                                    keg.densityGramsPerLiter,
                                    keg.capacityMl);
  result.percentageBasisPoints =
      calculatePercentage(result.volumeMl, keg.capacityMl);

  if (filteredWeightGrams <= keg.tareGrams) {
    result.validity = models::MeasurementValidity::BelowTare;
    return result;
  }

  const std::uint64_t expectedBeerWeight =
      (static_cast<std::uint64_t>(keg.capacityMl) *
       keg.densityGramsPerLiter) /
      1'000ULL;
  const std::uint64_t expectedMaximum =
      static_cast<std::uint64_t>(keg.tareGrams) + expectedBeerWeight +
      kOverfillToleranceGrams;
  result.validity =
      static_cast<std::uint64_t>(filteredWeightGrams) > expectedMaximum
          ? models::MeasurementValidity::AboveExpectedMaximum
          : models::MeasurementValidity::Valid;
  return result;
}

}  // namespace keezer::domain
