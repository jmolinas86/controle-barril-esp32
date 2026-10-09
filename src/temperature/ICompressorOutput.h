#pragma once

namespace keezer::temperature {

class ICompressorOutput {
 public:
  virtual ~ICompressorOutput() = default;
  virtual bool beginSafeOff() = 0;
  virtual bool setEnergized(bool energized) = 0;
  virtual bool isEnergized() const = 0;
};

}  // namespace keezer::temperature
