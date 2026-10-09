#pragma once

#include "temperature/ICompressorOutput.h"

namespace keezer::temperature {

// Stores the logical command for simulation and tests without touching GPIO.
class NullCompressorOutput final : public ICompressorOutput {
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

}  // namespace keezer::temperature
