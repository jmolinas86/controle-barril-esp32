#include "temperature/GpioCompressorOutput.h"

#include <Arduino.h>

namespace keezer::temperature {

GpioCompressorOutput::GpioCompressorOutput(const std::uint8_t pin,
                                           const bool activeHigh)
    : pin_(pin), activeHigh_(activeHigh) {}

bool GpioCompressorOutput::beginSafeOff() {
  const std::uint8_t offLevel = levelFor(false);
  digitalWrite(pin_, offLevel);
  pinMode(pin_, OUTPUT);
  digitalWrite(pin_, offLevel);
  energized_ = false;
  started_ = digitalRead(pin_) == offLevel;
  return started_;
}

bool GpioCompressorOutput::setEnergized(const bool energized) {
  if (!started_) return false;
  const std::uint8_t requestedLevel = levelFor(energized);
  digitalWrite(pin_, requestedLevel);
  const bool levelConfirmed = digitalRead(pin_) == requestedLevel;
  energized_ = levelConfirmed && energized;
  return levelConfirmed;
}

bool GpioCompressorOutput::isEnergized() const { return energized_; }

std::uint8_t GpioCompressorOutput::levelFor(const bool energized) const {
  return (energized == activeHigh_) ? HIGH : LOW;
}

}  // namespace keezer::temperature
