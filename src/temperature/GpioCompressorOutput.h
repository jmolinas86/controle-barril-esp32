#pragma once

#include <cstdint>

#include "temperature/ICompressorOutput.h"

namespace keezer::temperature {

class GpioCompressorOutput final : public ICompressorOutput {
 public:
  GpioCompressorOutput(std::uint8_t pin, bool activeHigh);

  bool beginSafeOff() override;
  bool setEnergized(bool energized) override;
  bool isEnergized() const override;

 private:
  std::uint8_t levelFor(bool energized) const;

  std::uint8_t pin_;
  bool activeHigh_;
  bool energized_{false};
  bool started_{false};
};

}  // namespace keezer::temperature
