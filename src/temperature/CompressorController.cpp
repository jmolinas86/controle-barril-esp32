#include "temperature/CompressorController.h"

namespace keezer::temperature {

CompressorController::CompressorController(ICompressorOutput& output)
    : output_(output) {}

bool CompressorController::begin(const std::uint32_t nowMs,
                                 const CompressorSettings& settings) {
  if (settings.hysteresisCentiCelsius == 0U) {
    return false;
  }
  settings_ = settings;
  state_ = models::TemperatureControlState::Idle;
  demandCooling_ = false;
  outputFault_ = false;
  outputChangedAtMs_ = nowMs;
  offSinceMs_ = nowMs;
  onSinceMs_ = 0U;
  protectionRemainingMs_ = settings_.minimumOffTimeMs;
  started_ = output_.beginSafeOff() && !output_.isEnergized();
  if (!started_) {
    state_ = models::TemperatureControlState::Error;
    outputFault_ = true;
  }
  return started_;
}

void CompressorController::update(
    const std::uint32_t nowMs, const bool sensorHealthy,
    const std::int16_t temperatureCentiCelsius) {
  if (!started_) {
    return;
  }
  if (!sensorHealthy) {
    enterError(nowMs);
    return;
  }

  if (state_ == models::TemperatureControlState::Error) {
    state_ = models::TemperatureControlState::Idle;
  }

  const std::int32_t halfHysteresis =
      static_cast<std::int32_t>(settings_.hysteresisCentiCelsius) / 2;
  const std::int32_t upperThreshold =
      static_cast<std::int32_t>(settings_.setpointCentiCelsius) +
      halfHysteresis;
  const std::int32_t lowerThreshold =
      static_cast<std::int32_t>(settings_.setpointCentiCelsius) -
      halfHysteresis;
  if (temperatureCentiCelsius >= upperThreshold) {
    demandCooling_ = true;
  } else if (temperatureCentiCelsius <= lowerThreshold) {
    demandCooling_ = false;
  }

  switch (state_) {
    case models::TemperatureControlState::Idle:
      if (demandCooling_) {
        if (nowMs - offSinceMs_ >= settings_.minimumOffTimeMs) {
          turnOn(nowMs);
        } else {
          state_ = models::TemperatureControlState::Waiting;
        }
      }
      break;
    case models::TemperatureControlState::Waiting:
      if (!demandCooling_) {
        state_ = models::TemperatureControlState::Idle;
      } else if (nowMs - offSinceMs_ >= settings_.minimumOffTimeMs) {
        turnOn(nowMs);
      }
      break;
    case models::TemperatureControlState::Cooling:
      if (!demandCooling_ &&
          nowMs - onSinceMs_ >= settings_.minimumOnTimeMs) {
        turnOff(nowMs);
        state_ = models::TemperatureControlState::Idle;
      }
      break;
    case models::TemperatureControlState::Error:
      break;
  }
  updateProtection(nowMs);
}

bool CompressorController::setSetpoint(
    const std::int16_t setpointCentiCelsius) {
  settings_.setpointCentiCelsius = setpointCentiCelsius;
  return started_;
}

models::TemperatureControlState CompressorController::state() const {
  return state_;
}

bool CompressorController::compressorOn() const {
  return output_.isEnergized();
}

bool CompressorController::demandCooling() const { return demandCooling_; }

bool CompressorController::outputFault() const { return outputFault_; }

std::uint32_t CompressorController::changedAtMs() const {
  return outputChangedAtMs_;
}

std::uint32_t CompressorController::protectionRemainingMs() const {
  return protectionRemainingMs_;
}

std::int16_t CompressorController::setpointCentiCelsius() const {
  return settings_.setpointCentiCelsius;
}

void CompressorController::enterError(const std::uint32_t nowMs) {
  if (state_ == models::TemperatureControlState::Error) {
    demandCooling_ = false;
    protectionRemainingMs_ = 0U;
    return;
  }
  if (output_.isEnergized() && !turnOff(nowMs)) {
    outputFault_ = true;
  }
  offSinceMs_ = nowMs;
  demandCooling_ = false;
  protectionRemainingMs_ = 0U;
  state_ = models::TemperatureControlState::Error;
}

bool CompressorController::turnOn(const std::uint32_t nowMs) {
  if (!output_.setEnergized(true) || !output_.isEnergized()) {
    output_.setEnergized(false);
    outputFault_ = true;
    state_ = models::TemperatureControlState::Error;
    offSinceMs_ = nowMs;
    return false;
  }
  onSinceMs_ = nowMs;
  outputChangedAtMs_ = nowMs;
  state_ = models::TemperatureControlState::Cooling;
  return true;
}

bool CompressorController::turnOff(const std::uint32_t nowMs) {
  if (!output_.setEnergized(false) || output_.isEnergized()) {
    outputFault_ = true;
    state_ = models::TemperatureControlState::Error;
    return false;
  }
  offSinceMs_ = nowMs;
  outputChangedAtMs_ = nowMs;
  return true;
}

void CompressorController::updateProtection(const std::uint32_t nowMs) {
  protectionRemainingMs_ = 0U;
  if (state_ == models::TemperatureControlState::Waiting) {
    const std::uint32_t elapsed = nowMs - offSinceMs_;
    if (elapsed < settings_.minimumOffTimeMs) {
      protectionRemainingMs_ = settings_.minimumOffTimeMs - elapsed;
    }
  } else if (state_ == models::TemperatureControlState::Cooling &&
             !demandCooling_) {
    const std::uint32_t elapsed = nowMs - onSinceMs_;
    if (elapsed < settings_.minimumOnTimeMs) {
      protectionRemainingMs_ = settings_.minimumOnTimeMs - elapsed;
    }
  }
}

}  // namespace keezer::temperature
