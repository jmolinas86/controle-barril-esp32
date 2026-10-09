#pragma once

#include <array>
#include <cstdint>

namespace keezer::scale {

struct WeightFilterSettings {
  std::uint8_t windowSize{5U};
  std::int32_t deadbandGrams{20};
  std::int32_t significantChangeGrams{2'000};
};

struct WeightFilterResult {
  bool hasOutput{false};
  bool outputChanged{false};
  std::int32_t outputGrams{0};
  bool significantChange{false};
  std::int32_t previousOutputGrams{0};
  std::int32_t deltaGrams{0};
};

// Fixed-memory median filter followed by an output deadband. Only samples
// explicitly supplied by ScaleService enter the window.
class WeightFilter final {
 public:
  static constexpr std::uint8_t kMaximumWindowSize = 9U;

  bool begin(const WeightFilterSettings& settings = {});
  void reset();
  WeightFilterResult addSample(std::int32_t weightGrams);

  bool hasOutput() const;
  std::int32_t outputGrams() const;
  std::uint8_t sampleCount() const;

 private:
  std::int32_t median() const;

  WeightFilterSettings settings_{};
  std::array<std::int32_t, kMaximumWindowSize> samples_{};
  std::uint8_t sampleCount_{0U};
  std::uint8_t nextSampleIndex_{0U};
  bool hasOutput_{false};
  std::int32_t outputGrams_{0};
  bool configured_{false};
};

}  // namespace keezer::scale
