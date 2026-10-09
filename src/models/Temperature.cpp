#include "models/Temperature.h"

namespace keezer::models {

const char* temperatureSampleStatusName(const TemperatureSampleStatus status) {
  switch (status) {
    case TemperatureSampleStatus::NoData:
      return "NO_DATA";
    case TemperatureSampleStatus::Valid:
      return "VALID";
    case TemperatureSampleStatus::Stale:
      return "STALE";
    case TemperatureSampleStatus::Disconnected:
      return "DISCONNECTED";
    case TemperatureSampleStatus::OutOfRange:
      return "OUT_OF_RANGE";
    case TemperatureSampleStatus::Error:
      return "ERROR";
  }
  return "ERROR";
}

const char* temperatureControlStateName(
    const TemperatureControlState state) {
  switch (state) {
    case TemperatureControlState::Idle:
      return "IDLE";
    case TemperatureControlState::Waiting:
      return "WAITING";
    case TemperatureControlState::Cooling:
      return "COOLING";
    case TemperatureControlState::Error:
      return "ERROR";
  }
  return "ERROR";
}

const char* temperatureFaultName(const TemperatureFault fault) {
  switch (fault) {
    case TemperatureFault::None:
      return "NONE";
    case TemperatureFault::NoSample:
      return "NO_SAMPLE";
    case TemperatureFault::Timeout:
      return "TIMEOUT";
    case TemperatureFault::Disconnected:
      return "DISCONNECTED";
    case TemperatureFault::OutOfRange:
      return "OUT_OF_RANGE";
    case TemperatureFault::SensorError:
      return "SENSOR_ERROR";
    case TemperatureFault::CompressorOutputError:
      return "COMPRESSOR_OUTPUT_ERROR";
  }
  return "SENSOR_ERROR";
}

}  // namespace keezer::models
