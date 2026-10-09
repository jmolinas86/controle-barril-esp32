#include <cassert>
#include <cstdio>

#include "scale/WeightFilter.h"

int main() {
  using keezer::scale::WeightFilter;
  using keezer::scale::WeightFilterSettings;

  WeightFilter invalid;
  assert(!invalid.begin(WeightFilterSettings{4U, 20, 2'000}));
  assert(!invalid.begin(WeightFilterSettings{5U, -1, 2'000}));

  WeightFilter filter;
  assert(filter.begin(WeightFilterSettings{5U, 20, 2'000}));

  auto result = filter.addSample(10'000);
  assert(result.hasOutput && result.outputChanged);
  assert(result.outputGrams == 10'000);
  assert(!result.significantChange);

  result = filter.addSample(9'992);
  assert(!result.outputChanged && result.outputGrams == 10'000);
  result = filter.addSample(10'008);
  assert(!result.outputChanged && result.outputGrams == 10'000);
  result = filter.addSample(9'995);
  assert(!result.outputChanged && result.outputGrams == 10'000);

  // A single 1.5 kg outlier is rejected by the median.
  result = filter.addSample(11'500);
  assert(!result.outputChanged && result.outputGrams == 10'000);
  result = filter.addSample(10'006);
  assert(!result.outputChanged && result.outputGrams == 10'000);

  // Three coherent low samples move the median and emit one significant
  // change; one isolated sample cannot do so.
  result = filter.addSample(5'000);
  assert(!result.outputChanged);
  result = filter.addSample(4'990);
  assert(!result.outputChanged);
  result = filter.addSample(5'010);
  assert(result.outputChanged && result.significantChange);
  assert(result.previousOutputGrams == 10'000);
  assert(result.outputGrams == 5'010);
  assert(result.deltaGrams == -4'990);

  WeightFilter deadband;
  assert(deadband.begin(WeightFilterSettings{1U, 20, 2'000}));
  deadband.addSample(10'000);
  result = deadband.addSample(9'981);
  assert(!result.outputChanged && result.outputGrams == 10'000);
  result = deadband.addSample(9'980);
  assert(result.outputChanged && result.outputGrams == 9'980);

  WeightFilter threshold;
  assert(threshold.begin(WeightFilterSettings{1U, 0, 2'000}));
  threshold.addSample(10'000);
  result = threshold.addSample(8'000);
  assert(result.outputChanged && !result.significantChange);
  result = threshold.addSample(5'999);
  assert(result.outputChanged && result.significantChange);

  std::puts("WeightFilter smoke test: PASS");
  return 0;
}
