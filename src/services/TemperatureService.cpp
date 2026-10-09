#include "services/TemperatureService.h"

#include "diagnostics/Logger.h"

namespace keezer::services {
namespace {

constexpr char kLogTag[] = "TEMP";

}  // namespace

TemperatureService::TemperatureService(
    temperature::ITemperatureSensor& sensor,
    temperature::ICompressorOutput& output)
    : sensor_(sensor), controller_(output) {}

bool TemperatureService::begin(const std::uint32_t nowMs,
                               const TemperatureSettings& settings) {
  if (settings.hysteresisCentiCelsius == 0U ||
      settings.sensorTimeoutMs == 0U ||
      settings.minimumValidCentiCelsius >=
          settings.maximumValidCentiCelsius ||
      settings.setpointCentiCelsius <= settings.minimumValidCentiCelsius ||
      settings.setpointCentiCelsius >= settings.maximumValidCentiCelsius ||
      settings.recoveryValidSamples == 0U) {
    KEEZER_LOG_ERROR(kLogTag, "Invalid temperature settings");
    return false;
  }

  state_ = {};
  state_.setpointCentiCelsius = settings.setpointCentiCelsius;
  state_.revision = 1U;
  hasSeenSequence_ = false;
  lastSeenSequence_ = 0U;
  beganAtMs_ = nowMs;
  sensorTimeoutMs_ = settings.sensorTimeoutMs;
  minimumValidCentiCelsius_ = settings.minimumValidCentiCelsius;
  maximumValidCentiCelsius_ = settings.maximumValidCentiCelsius;
  recoveryValidSamples_ = settings.recoveryValidSamples;
  consecutiveRecoverySamples_ = 0U;
  validSampleLogCounter_ = 0U;
  faultLatched_ = false;

  const temperature::CompressorSettings compressorSettings{
      settings.setpointCentiCelsius,
      settings.hysteresisCentiCelsius,
      settings.minimumOffTimeMs,
      settings.minimumOnTimeMs,
  };
  const bool outputReady = controller_.begin(nowMs, compressorSettings);
  const bool sensorReady = sensor_.begin(nowMs);
  started_ = outputReady;
  if (!outputReady) {
    setFault(models::TemperatureFault::CompressorOutputError,
             models::TemperatureSampleStatus::Error);
    KEEZER_LOG_ERROR(kLogTag,
                     "Temperature control unavailable: output unsafe");
    return false;
  }
  if (!sensorReady) {
    setFault(models::TemperatureFault::Disconnected,
             models::TemperatureSampleStatus::Disconnected);
  }

  KEEZER_LOG_INFO(
      kLogTag,
      "Temperature service started: sensor=%s output=GPIO setpoint=%d.%02dC hysteresis=%u.%02uC minOff=%lus minOn=%lus timeout=%lus",
      sensorReady ? "READY" : "FAILED",
      static_cast<int>(settings.setpointCentiCelsius / 100),
      static_cast<int>(settings.setpointCentiCelsius >= 0
                           ? settings.setpointCentiCelsius % 100
                           : -(settings.setpointCentiCelsius % 100)),
      static_cast<unsigned int>(settings.hysteresisCentiCelsius / 100U),
      static_cast<unsigned int>(settings.hysteresisCentiCelsius % 100U),
      static_cast<unsigned long>(settings.minimumOffTimeMs / 1'000U),
      static_cast<unsigned long>(settings.minimumOnTimeMs / 1'000U),
      static_cast<unsigned long>(settings.sensorTimeoutMs / 1'000U));
  update(nowMs);
  return true;
}

void TemperatureService::update(const std::uint32_t nowMs) {
  if (!started_) {
    return;
  }
  const models::TemperatureControlState previousControlState =
      state_.controlState;
  const bool previousCompressorOn = state_.compressorOn;
  const models::TemperatureFault previousFault = state_.fault;
  const models::TemperatureSampleStatus previousSampleStatus =
      state_.sampleStatus;
  const std::int16_t previousTemperature = state_.currentCentiCelsius;

  sensor_.update(nowMs);
  models::TemperatureSample sample{};
  const bool hasSample = sensor_.latestSample(sample);
  const bool newSample =
      hasSample && (!hasSeenSequence_ || sample.sequence != lastSeenSequence_);
  if (newSample) {
    hasSeenSequence_ = true;
    lastSeenSequence_ = sample.sequence;
    state_.lastSampleMonotonicMs = sample.sampledAtMonotonicMs;
    processNewSample(sample);
  }

  const std::uint32_t sampleAgeMs =
      hasSeenSequence_ ? nowMs - state_.lastSampleMonotonicMs
                           : nowMs - beganAtMs_;
  if ((!hasSeenSequence_ || sampleAgeMs >= sensorTimeoutMs_) &&
      state_.fault != models::TemperatureFault::CompressorOutputError) {
    setFault(hasSeenSequence_ ? models::TemperatureFault::Timeout
                                  : models::TemperatureFault::NoSample,
             hasSeenSequence_ ? models::TemperatureSampleStatus::Stale
                                  : models::TemperatureSampleStatus::NoData);
  }

  const bool sensorHealthy =
      !faultLatched_ && state_.sampleStatus ==
                            models::TemperatureSampleStatus::Valid &&
      state_.hasCurrentTemperature;
  controller_.update(nowMs, sensorHealthy, state_.currentCentiCelsius);
  if (controller_.outputFault() &&
      state_.fault != models::TemperatureFault::CompressorOutputError) {
    setFault(models::TemperatureFault::CompressorOutputError,
             models::TemperatureSampleStatus::Error);
    controller_.update(nowMs, false, state_.currentCentiCelsius);
  }
  publishControllerState(nowMs);
  logChanges(previousControlState, previousCompressorOn, previousFault);

  if (newSample || previousControlState != state_.controlState ||
      previousCompressorOn != state_.compressorOn ||
      previousFault != state_.fault ||
      previousSampleStatus != state_.sampleStatus ||
      previousTemperature != state_.currentCentiCelsius) {
    ++state_.revision;
  }
}

bool TemperatureService::setSetpointCentiCelsius(
    const std::int16_t setpointCentiCelsius) {
  if (!started_ ||
      setpointCentiCelsius <= minimumValidCentiCelsius_ ||
      setpointCentiCelsius >= maximumValidCentiCelsius_ ||
      !controller_.setSetpoint(setpointCentiCelsius)) {
    return false;
  }
  state_.setpointCentiCelsius = setpointCentiCelsius;
  ++state_.revision;
  KEEZER_LOG_INFO(kLogTag, "Setpoint changed: %d.%02dC",
                  static_cast<int>(setpointCentiCelsius / 100),
                  static_cast<int>(setpointCentiCelsius >= 0
                                       ? setpointCentiCelsius % 100
                                       : -(setpointCentiCelsius % 100)));
  return true;
}

const models::TemperatureState& TemperatureService::state() const {
  return state_;
}

std::uint32_t TemperatureService::revision() const { return state_.revision; }

void TemperatureService::processNewSample(
    const models::TemperatureSample& sample) {
  models::TemperatureFault sampleFault = models::TemperatureFault::None;
  models::TemperatureSampleStatus sampleStatus =
      models::TemperatureSampleStatus::Valid;
  if (sample.quality == models::TemperatureSampleQuality::Disconnected) {
    sampleFault = models::TemperatureFault::Disconnected;
    sampleStatus = models::TemperatureSampleStatus::Disconnected;
  } else if (sample.quality == models::TemperatureSampleQuality::ReadError) {
    sampleFault = models::TemperatureFault::SensorError;
    sampleStatus = models::TemperatureSampleStatus::Error;
  } else if (sample.centiCelsius < minimumValidCentiCelsius_ ||
             sample.centiCelsius > maximumValidCentiCelsius_) {
    sampleFault = models::TemperatureFault::OutOfRange;
    sampleStatus = models::TemperatureSampleStatus::OutOfRange;
  }

  state_.sampleStatus = sampleStatus;
  if (sampleFault != models::TemperatureFault::None) {
    setFault(sampleFault, sampleStatus);
    return;
  }

  state_.hasCurrentTemperature = true;
  state_.currentCentiCelsius = sample.centiCelsius;
  if (faultLatched_) {
    if (consecutiveRecoverySamples_ < 255U) {
      ++consecutiveRecoverySamples_;
    }
    if (consecutiveRecoverySamples_ >= recoveryValidSamples_ &&
        state_.fault != models::TemperatureFault::CompressorOutputError) {
      clearFault();
    }
  }

  if (validSampleLogCounter_ < 5U) {
    ++validSampleLogCounter_;
  }
  if (validSampleLogCounter_ == 1U || validSampleLogCounter_ == 5U) {
    if (validSampleLogCounter_ == 5U) {
      validSampleLogCounter_ = 0U;
    }
    KEEZER_LOG_INFO(
        kLogTag,
        "TEMPERATURE_UPDATED value=%d.%02dC sample=%s control=%s compressor=%s",
        static_cast<int>(sample.centiCelsius / 100),
        static_cast<int>(sample.centiCelsius >= 0
                             ? sample.centiCelsius % 100
                             : -(sample.centiCelsius % 100)),
        models::temperatureSampleStatusName(state_.sampleStatus),
        models::temperatureControlStateName(state_.controlState),
        state_.compressorOn ? "ON" : "OFF");
  }
}

void TemperatureService::setFault(
    const models::TemperatureFault fault,
    const models::TemperatureSampleStatus status) {
  state_.fault = fault;
  state_.sampleStatus = status;
  consecutiveRecoverySamples_ = 0U;
  faultLatched_ = true;
}

void TemperatureService::clearFault() {
  state_.fault = models::TemperatureFault::None;
  consecutiveRecoverySamples_ = 0U;
  faultLatched_ = false;
}

void TemperatureService::publishControllerState(const std::uint32_t nowMs) {
  (void)nowMs;
  const models::TemperatureControlState controlState = controller_.state();
  const bool compressorOn = controller_.compressorOn();
  state_.controlState = controlState;
  state_.compressorOn = compressorOn;
  state_.demandCooling = controller_.demandCooling();
  state_.compressorChangedAtMs = controller_.changedAtMs();
  state_.protectionRemainingMs = controller_.protectionRemainingMs();
  state_.setpointCentiCelsius = controller_.setpointCentiCelsius();
}

void TemperatureService::logChanges(
    const models::TemperatureControlState previousControlState,
    const bool previousCompressorOn,
    const models::TemperatureFault previousFault) {
  if (previousFault != state_.fault) {
    if (state_.fault == models::TemperatureFault::None) {
      KEEZER_LOG_INFO(kLogTag,
                      "Temperature sensor recovered after valid samples");
    } else {
      KEEZER_LOG_WARN(kLogTag, "TEMPERATURE_SENSOR_ERROR fault=%s output=OFF",
                      models::temperatureFaultName(state_.fault));
    }
  }
  if (!previousCompressorOn && state_.compressorOn) {
    KEEZER_LOG_INFO(kLogTag, "COOLING_STARTED temperature=%d.%02dC",
                    static_cast<int>(state_.currentCentiCelsius / 100),
                    static_cast<int>(state_.currentCentiCelsius >= 0
                                         ? state_.currentCentiCelsius % 100
                                         : -(state_.currentCentiCelsius % 100)));
  } else if (previousCompressorOn && !state_.compressorOn) {
    KEEZER_LOG_INFO(kLogTag, "COOLING_STOPPED state=%s fault=%s",
                    models::temperatureControlStateName(state_.controlState),
                    models::temperatureFaultName(state_.fault));
  } else if (previousControlState != state_.controlState) {
    KEEZER_LOG_INFO(
        kLogTag, "Temperature control state=%s protection=%lus",
        models::temperatureControlStateName(state_.controlState),
        static_cast<unsigned long>(state_.protectionRemainingMs / 1'000U));
  }
}

}  // namespace keezer::services
