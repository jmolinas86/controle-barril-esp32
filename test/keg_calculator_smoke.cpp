#include <cassert>
#include <cstdio>

#include "domain/KegCalculator.h"

namespace {

keezer::models::Keg configuredKeg() {
  keezer::models::Keg keg{};
  keg.capacityMl = 20'000U;
  keg.tareGrams = 4'350;
  keg.densityGramsPerLiter = 1'000U;
  return keg;
}

}  // namespace

int main() {
  using keezer::domain::KegCalculator;
  using keezer::models::MeasurementValidity;

  assert(KegCalculator::validateWeight(0));
  assert(KegCalculator::validateWeight(100'000));
  assert(!KegCalculator::validateWeight(-1));
  assert(!KegCalculator::validateWeight(100'001));

  const auto keg = configuredKeg();
  assert(KegCalculator::validateConfiguration(keg));
  assert(KegCalculator::calculateBeerWeight(18'550, 4'350) == 14'200U);
  assert(KegCalculator::calculateBeerWeight(4'000, 4'350) == 0U);
  assert(KegCalculator::calculateVolume(14'200U, 1'000U, 20'000U) ==
         14'200U);
  assert(KegCalculator::calculateVolume(14'200U, 1'010U, 20'000U) ==
         14'059U);
  assert(KegCalculator::calculateVolume(25'000U, 1'000U, 20'000U) ==
         20'000U);
  assert(KegCalculator::calculatePercentage(14'200U, 20'000U) == 7'100U);
  assert(KegCalculator::calculatePercentage(6'300U, 20'000U) == 3'150U);
  assert(KegCalculator::calculatePercentage(25'000U, 20'000U) == 10'000U);
  assert(KegCalculator::calculatePercentage(1'000U, 0U) == 0U);

  auto measurement = KegCalculator::calculate(keg, 18'550);
  assert(measurement.validity == MeasurementValidity::Valid);
  assert(measurement.filteredWeightGrams == 18'550);
  assert(measurement.beerWeightGrams == 14'200U);
  assert(measurement.unclampedVolumeMl == 14'200U);
  assert(measurement.volumeMl == 14'200U);
  assert(measurement.percentageBasisPoints == 7'100U);

  measurement = KegCalculator::calculate(keg, 4'350);
  assert(measurement.validity == MeasurementValidity::BelowTare);
  assert(measurement.beerWeightGrams == 0U);
  assert(measurement.volumeMl == 0U);
  assert(measurement.percentageBasisPoints == 0U);

  measurement = KegCalculator::calculate(keg, -1);
  assert(measurement.validity == MeasurementValidity::InvalidWeight);
  measurement = KegCalculator::calculate(keg, 100'001);
  assert(measurement.validity == MeasurementValidity::InvalidWeight);

  measurement = KegCalculator::calculate(keg, 24'851);
  assert(measurement.validity ==
         MeasurementValidity::AboveExpectedMaximum);
  assert(measurement.unclampedVolumeMl == 20'501U);
  assert(measurement.volumeMl == 20'000U);
  assert(measurement.percentageBasisPoints == 10'000U);

  auto invalidKeg = keg;
  invalidKeg.densityGramsPerLiter = 899U;
  measurement = KegCalculator::calculate(invalidKeg, 18'550);
  assert(measurement.validity ==
         MeasurementValidity::InvalidKegConfiguration);
  invalidKeg = keg;
  invalidKeg.capacityMl = 0U;
  measurement = KegCalculator::calculate(invalidKeg, 18'550);
  assert(measurement.validity ==
         MeasurementValidity::InvalidKegConfiguration);

  std::puts("KegCalculator smoke test: PASS");
  return 0;
}
