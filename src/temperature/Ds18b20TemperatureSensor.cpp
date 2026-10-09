#include "temperature/Ds18b20TemperatureSensor.h"

#include <Arduino.h>

#include <array>

#include "diagnostics/Logger.h"

namespace keezer::temperature {
namespace {

constexpr char kLogTag[] = "TEMP_SENSOR";
constexpr std::uint8_t kDs18b20FamilyCode = 0x28U;

}  // namespace

Ds18b20TemperatureSensor::Ds18b20TemperatureSensor(
    const std::uint8_t dataPin)
    : dataPin_(dataPin), bus_(dataPin) {}

bool Ds18b20TemperatureSensor::begin(const std::uint32_t nowMs) {
  sample_ = {};
  conversionRequestedAtMs_ = 0U;
  lastSampleAtMs_ = 0U;
  lastDiscoveryAtMs_ = nowMs - kDiscoveryRetryMs;
  sensorPresent_ = false;
  addressValid_ = false;
  conversionPending_ = false;
  started_ = true;
  const bool ready = requestConversion(nowMs);
  if (!ready) {
    KEEZER_LOG_WARN(kLogTag,
                    "DS18B20_NOT_FOUND gpio=%u expected_family=0x28",
                    static_cast<unsigned int>(dataPin_));
  }
  return ready;
}

void Ds18b20TemperatureSensor::update(const std::uint32_t nowMs) {
  if (!started_) return;

  if (!sensorPresent_) {
    if (nowMs - lastDiscoveryAtMs_ < kDiscoveryRetryMs) return;
    if (!requestConversion(nowMs)) {
      publish(nowMs, 0, models::TemperatureSampleQuality::Disconnected);
    }
    return;
  }

  if (!conversionPending_) {
    requestConversion(nowMs);
    return;
  }
  if (nowMs - conversionRequestedAtMs_ < kConversionTimeMs ||
      (lastSampleAtMs_ != 0U &&
       nowMs - lastSampleAtMs_ < kSamplePeriodMs)) {
    return;
  }

  std::int16_t centiCelsius = 0;
  const models::TemperatureSampleQuality quality =
      readTemperature(centiCelsius);
  conversionPending_ = false;
  if (quality == models::TemperatureSampleQuality::Disconnected) {
    sensorPresent_ = false;
    publish(nowMs, 0, quality);
    return;
  }
  publish(nowMs, centiCelsius, quality);
  requestConversion(nowMs);
}

bool Ds18b20TemperatureSensor::latestSample(
    models::TemperatureSample& sample) const {
  if (sample_.sequence == 0U) return false;
  sample = sample_;
  return true;
}

bool Ds18b20TemperatureSensor::discoverSensor() {
  bus_.reset_search();
  std::array<std::uint8_t, 8U> candidate{};
  while (bus_.search(candidate.data())) {
    if (OneWire::crc8(candidate.data(), 7U) != candidate[7U] ||
        candidate[0U] != kDs18b20FamilyCode) {
      continue;
    }
    address_ = candidate;
    addressValid_ = true;
    KEEZER_LOG_INFO(
        kLogTag,
        "DS18B20_FOUND gpio=%u rom=%02X%02X%02X%02X%02X%02X%02X%02X",
        static_cast<unsigned int>(dataPin_), address_[0U], address_[1U],
        address_[2U], address_[3U], address_[4U], address_[5U], address_[6U],
        address_[7U]);
    return true;
  }
  bus_.reset_search();
  addressValid_ = false;
  return false;
}

bool Ds18b20TemperatureSensor::requestConversion(
    const std::uint32_t nowMs) {
  lastDiscoveryAtMs_ = nowMs;
  if (!addressValid_ && !discoverSensor()) {
    sensorPresent_ = false;
    conversionPending_ = false;
    return false;
  }
  if (!bus_.reset()) {
    sensorPresent_ = false;
    addressValid_ = false;
    conversionPending_ = false;
    return false;
  }
  bus_.select(address_.data());
  bus_.write(0x44U, 0U);
  sensorPresent_ = true;
  conversionRequestedAtMs_ = nowMs;
  conversionPending_ = true;
  return true;
}

models::TemperatureSampleQuality
Ds18b20TemperatureSensor::readTemperature(std::int16_t& centiCelsius) {
  if (!addressValid_ || !bus_.reset()) {
    addressValid_ = false;
    return models::TemperatureSampleQuality::Disconnected;
  }
  bus_.select(address_.data());
  bus_.write(0xBEU);

  std::array<std::uint8_t, 9U> scratchpad{};
  for (std::uint8_t index = 0U; index < scratchpad.size(); ++index) {
    scratchpad[index] = bus_.read();
  }
  bool allZero = true;
  bool allOnes = true;
  for (const std::uint8_t value : scratchpad) {
    allZero = allZero && value == 0x00U;
    allOnes = allOnes && value == 0xFFU;
  }
  // A line held LOW produces nine zero bytes and even passes the Dallas CRC
  // by coincidence. Neither an all-zero nor an all-one scratchpad is a valid
  // DS18B20 response, so never publish it as a real 0.00 C measurement.
  if (allZero || allOnes) {
    return models::TemperatureSampleQuality::ReadError;
  }
  if (OneWire::crc8(scratchpad.data(), 8U) != scratchpad[8U]) {
    return models::TemperatureSampleQuality::ReadError;
  }
  const std::int16_t raw = static_cast<std::int16_t>(
      static_cast<std::uint16_t>(scratchpad[0U]) |
      (static_cast<std::uint16_t>(scratchpad[1U]) << 8U));
  const std::int32_t scaled = static_cast<std::int32_t>(raw) * 100;
  centiCelsius = static_cast<std::int16_t>(scaled / 16);
  return models::TemperatureSampleQuality::Valid;
}

void Ds18b20TemperatureSensor::publish(
    const std::uint32_t nowMs, const std::int16_t centiCelsius,
    const models::TemperatureSampleQuality quality) {
  ++sample_.sequence;
  sample_.sampledAtMonotonicMs = nowMs;
  sample_.centiCelsius = centiCelsius;
  sample_.quality = quality;
  lastSampleAtMs_ = nowMs;
}

}  // namespace keezer::temperature
