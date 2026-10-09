#include "temperature/DemoTemperatureSensor.h"

#include "BuildConfig.h"

namespace keezer::temperature {

bool DemoTemperatureSensor::begin(const std::uint32_t nowMs) {
  sample_ = {};
  startedAtMs_ = nowMs;
  lastSampleAtMs_ = nowMs;
  prepareSample(nowMs);
  return true;
}

void DemoTemperatureSensor::update(const std::uint32_t nowMs) {
  if (nowMs - lastSampleAtMs_ < config::kDemoTemperatureSamplePeriodMs) {
    return;
  }
  prepareSample(nowMs);
}

bool DemoTemperatureSensor::latestSample(
    models::TemperatureSample& sample) const {
  if (sample_.sequence == 0U) {
    return false;
  }
  sample = sample_;
  return true;
}

void DemoTemperatureSensor::prepareSample(const std::uint32_t nowMs) {
  const std::uint32_t phaseMs =
      (nowMs - startedAtMs_) % config::kDemoTemperatureCycleMs;
  sample_.sequence += 1U;
  sample_.sampledAtMonotonicMs = nowMs;
  if (phaseMs >= 13'000U && phaseMs < 17'000U) {
    sample_.quality = models::TemperatureSampleQuality::ReadError;
    sample_.centiCelsius = 0;
  } else {
    sample_.quality = models::TemperatureSampleQuality::Valid;
    sample_.centiCelsius =
        phaseMs >= 28'000U && phaseMs < 45'000U ? 140 : 270;
  }
  lastSampleAtMs_ = nowMs;
}

}  // namespace keezer::temperature
