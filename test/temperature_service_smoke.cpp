#include <cassert>
#include <cstdio>

#include "diagnostics/Logger.h"
#include "services/TemperatureService.h"

namespace keezer::diagnostics {
LogLevel Logger::currentLevel_ = LogLevel::Error;
void Logger::begin(std::uint32_t) {}
void Logger::setLevel(const LogLevel level) { currentLevel_ = level; }
LogLevel Logger::level() { return currentLevel_; }
void Logger::log(LogLevel, const char*, const char*, ...) {}
const char* Logger::levelName(LogLevel) { return "TEST"; }
}  // namespace keezer::diagnostics

namespace {

class FakeTemperatureSensor final
    : public keezer::temperature::ITemperatureSensor {
 public:
  bool begin(std::uint32_t) override { return true; }
  void update(std::uint32_t) override {}
  bool latestSample(keezer::models::TemperatureSample& sample) const override {
    if (!hasSample_) {
      return false;
    }
    sample = sample_;
    return true;
  }
  void push(const std::uint32_t sequence, const std::uint32_t atMs,
            const std::int16_t centiCelsius,
            const keezer::models::TemperatureSampleQuality quality =
                keezer::models::TemperatureSampleQuality::Valid) {
    sample_.sequence = sequence;
    sample_.sampledAtMonotonicMs = atMs;
    sample_.centiCelsius = centiCelsius;
    sample_.quality = quality;
    hasSample_ = true;
  }

 private:
  keezer::models::TemperatureSample sample_{};
  bool hasSample_{false};
};

class FakeCompressorOutput final
    : public keezer::temperature::ICompressorOutput {
 public:
  bool beginSafeOff() override {
    energized_ = false;
    return true;
  }
  bool setEnergized(const bool energized) override {
    energized_ = energized;
    return true;
  }
  bool isEnergized() const override { return energized_; }

 private:
  bool energized_{false};
};

}  // namespace

int main() {
  using keezer::models::TemperatureControlState;
  using keezer::models::TemperatureFault;
  using keezer::models::TemperatureSampleStatus;
  using keezer::services::TemperatureService;
  using keezer::services::TemperatureSettings;

  FakeTemperatureSensor sensor;
  FakeCompressorOutput output;
  TemperatureService service(sensor, output);
  const TemperatureSettings settings{200, 100U, 1'000U, 500U, 1'000U,
                                     -2'000, 5'000, 3U};
  assert(service.begin(0U, settings));
  assert(service.state().fault == TemperatureFault::NoSample);
  assert(service.state().controlState == TemperatureControlState::Error);

  sensor.push(1U, 100U, 270);
  service.update(100U);
  sensor.push(2U, 200U, 270);
  service.update(200U);
  assert(service.state().fault == TemperatureFault::NoSample);
  sensor.push(3U, 300U, 270);
  service.update(300U);
  assert(service.state().fault == TemperatureFault::None);
  assert(service.state().sampleStatus == TemperatureSampleStatus::Valid);
  assert(service.state().controlState == TemperatureControlState::Waiting);

  // No new sample for the configured interval becomes a critical timeout.
  service.update(1'300U);
  assert(service.state().fault == TemperatureFault::Timeout);
  assert(service.state().sampleStatus == TemperatureSampleStatus::Stale);
  assert(!service.state().compressorOn);

  sensor.push(4U, 1'400U, 6'000);
  service.update(1'400U);
  assert(service.state().fault == TemperatureFault::OutOfRange);
  assert(service.state().sampleStatus == TemperatureSampleStatus::OutOfRange);

  sensor.push(5U, 1'450U, 0,
              keezer::models::TemperatureSampleQuality::Disconnected);
  service.update(1'450U);
  assert(service.state().fault == TemperatureFault::Disconnected);
  assert(!service.state().compressorOn);

  sensor.push(6U, 1'500U, 270);
  service.update(1'500U);
  sensor.push(7U, 1'600U, 270);
  service.update(1'600U);
  sensor.push(8U, 1'700U, 270);
  service.update(1'700U);
  assert(service.state().fault == TemperatureFault::None);
  assert(service.setSetpointCentiCelsius(250));
  assert(service.state().setpointCentiCelsius == 250);

  std::puts("TemperatureService smoke test: PASS");
  return 0;
}
