#include "scale/WeightFilter.h"

#include <cstdint>

namespace keezer::scale {
namespace {

std::int64_t absoluteDifference(const std::int32_t lhs,
                                const std::int32_t rhs) {
  const std::int64_t difference =
      static_cast<std::int64_t>(lhs) - static_cast<std::int64_t>(rhs);
  return difference < 0 ? -difference : difference;
}

}  // namespace

bool WeightFilter::begin(const WeightFilterSettings& settings) {
  if (settings.windowSize == 0U ||
      settings.windowSize > kMaximumWindowSize ||
      (settings.windowSize % 2U) == 0U || settings.deadbandGrams < 0 ||
      settings.significantChangeGrams <= 0) {
    configured_ = false;
    return false;
  }
  settings_ = settings;
  configured_ = true;
  reset();
  return true;
}

void WeightFilter::reset() {
  samples_ = {};
  sampleCount_ = 0U;
  nextSampleIndex_ = 0U;
  hasOutput_ = false;
  outputGrams_ = 0;
}

WeightFilterResult WeightFilter::addSample(const std::int32_t weightGrams) {
  WeightFilterResult result{};
  if (!configured_) {
    return result;
  }

  samples_[nextSampleIndex_] = weightGrams;
  nextSampleIndex_ = static_cast<std::uint8_t>(
      (nextSampleIndex_ + 1U) % settings_.windowSize);
  if (sampleCount_ < settings_.windowSize) {
    ++sampleCount_;
  }

  const std::int32_t candidate = median();
  if (!hasOutput_) {
    hasOutput_ = true;
    outputGrams_ = candidate;
    result.hasOutput = true;
    result.outputChanged = true;
    result.outputGrams = outputGrams_;
    result.previousOutputGrams = outputGrams_;
    return result;
  }

  result.hasOutput = true;
  result.previousOutputGrams = outputGrams_;
  const std::int64_t difference =
      absoluteDifference(candidate, outputGrams_);
  if (difference < settings_.deadbandGrams) {
    result.outputGrams = outputGrams_;
    return result;
  }

  const std::int32_t previousOutput = outputGrams_;
  outputGrams_ = candidate;
  result.outputChanged = true;
  result.outputGrams = outputGrams_;
  result.deltaGrams = outputGrams_ - previousOutput;
  result.significantChange =
      difference > settings_.significantChangeGrams;
  return result;
}

bool WeightFilter::hasOutput() const { return hasOutput_; }

std::int32_t WeightFilter::outputGrams() const { return outputGrams_; }

std::uint8_t WeightFilter::sampleCount() const { return sampleCount_; }

std::int32_t WeightFilter::median() const {
  std::array<std::int32_t, kMaximumWindowSize> sorted{};
  for (std::uint8_t index = 0U; index < sampleCount_; ++index) {
    sorted[index] = samples_[index];
  }

  // The window never exceeds nine entries, so insertion sort is compact and
  // deterministic without allocating memory.
  for (std::uint8_t index = 1U; index < sampleCount_; ++index) {
    const std::int32_t value = sorted[index];
    std::uint8_t destination = index;
    while (destination > 0U && sorted[destination - 1U] > value) {
      sorted[destination] = sorted[destination - 1U];
      --destination;
    }
    sorted[destination] = value;
  }
  return sorted[sampleCount_ / 2U];
}

}  // namespace keezer::scale
