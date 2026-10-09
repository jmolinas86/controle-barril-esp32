#include <cassert>
#include <cstdio>

#include "temperature/CompressorController.h"

namespace {

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
  using keezer::temperature::CompressorController;
  using keezer::temperature::CompressorSettings;

  FakeCompressorOutput output;
  CompressorController controller(output);
  assert(controller.begin(0U, CompressorSettings{200, 100U, 1'000U, 500U}));
  assert(controller.state() == TemperatureControlState::Idle);
  assert(!controller.compressorOn());

  controller.update(0U, true, 270);
  assert(controller.state() == TemperatureControlState::Waiting);
  assert(controller.protectionRemainingMs() == 1'000U);
  controller.update(999U, true, 270);
  assert(!controller.compressorOn());
  controller.update(1'000U, true, 270);
  assert(controller.state() == TemperatureControlState::Cooling);
  assert(controller.compressorOn());

  // A critical sensor fault overrides minimum-on protection.
  controller.update(1'200U, false, 270);
  assert(controller.state() == TemperatureControlState::Error);
  assert(!controller.compressorOn());

  controller.update(1'300U, true, 270);
  assert(controller.state() == TemperatureControlState::Waiting);
  controller.update(2'199U, true, 270);
  assert(!controller.compressorOn());
  controller.update(2'200U, true, 270);
  assert(controller.compressorOn());

  // Low temperature requests stop, but minimum-on is still respected.
  controller.update(2'300U, true, 140);
  assert(controller.compressorOn());
  assert(controller.protectionRemainingMs() == 400U);
  controller.update(2'700U, true, 140);
  assert(controller.state() == TemperatureControlState::Idle);
  assert(!controller.compressorOn());

  // Changing setpoint does not bypass the minimum-off timer.
  assert(controller.setSetpoint(100));
  controller.update(2'800U, true, 270);
  assert(controller.state() == TemperatureControlState::Waiting);
  assert(!controller.compressorOn());

  std::puts("CompressorController smoke test: PASS");
  return 0;
}
