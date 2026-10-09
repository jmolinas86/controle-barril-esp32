#include "temperature/Ds18b20TemperatureSensor.h"

#include <Arduino.h>

#include <array>

namespace keezer::temperature {

Ds18b20TemperatureSensor::Ds18b20TemperatureSensor(
    const std::uint8_t dataPin)
    : dataPin_(dataPin) {}

bool Ds18b20TemperatureSensor::begin(const std::uint32_t nowMs) {
  sample_ = {};
  conversionRequestedAtMs_ = 0U;
  lastSampleAtMs_ = 0U;
  lastDiscoveryAtMs_ = nowMs - kDiscoveryRetryMs;
  sensorPresent_ = false;
  conversionPending_ = false;
  started_ = true;
  pinMode(dataPin_, INPUT_PULLUP);
  return requestConversion(nowMs);
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

bool Ds18b20TemperatureSensor::requestConversion(
    const std::uint32_t nowMs) {
  lastDiscoveryAtMs_ = nowMs;
  if (!busReset()) {
    sensorPresent_ = false;
    conversionPending_ = false;
    return false;
  }
  // A single externally powered DS18B20 is installed, so Skip ROM is safe.
  busWriteByte(0xCCU);
  busWriteByte(0x44U);
  pinMode(dataPin_, INPUT_PULLUP);
  sensorPresent_ = true;
  conversionRequestedAtMs_ = nowMs;
  conversionPending_ = true;
  return true;
}

models::TemperatureSampleQuality
Ds18b20TemperatureSensor::readTemperature(std::int16_t& centiCelsius) {
  if (!busReset()) return models::TemperatureSampleQuality::Disconnected;
  busWriteByte(0xCCU);
  busWriteByte(0xBEU);

  std::array<std::uint8_t, 9U> scratchpad{};
  for (std::uint8_t index = 0U; index < scratchpad.size(); ++index) {
    scratchpad[index] = busReadByte();
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
  if (crc8(scratchpad.data(), 8U) != scratchpad[8U]) {
    return models::TemperatureSampleQuality::ReadError;
  }
  const std::int16_t raw = static_cast<std::int16_t>(
      static_cast<std::uint16_t>(scratchpad[0U]) |
      (static_cast<std::uint16_t>(scratchpad[1U]) << 8U));
  const std::int32_t scaled = static_cast<std::int32_t>(raw) * 100;
  centiCelsius = static_cast<std::int16_t>(scaled / 16);
  return models::TemperatureSampleQuality::Valid;
}

bool Ds18b20TemperatureSensor::busReset() {
  // A healthy externally pulled-up 1-Wire bus must be idle HIGH. Checking it
  // before the reset pulse distinguishes a real presence pulse from a wiring
  // error or a data line shorted to ground.
  pinMode(dataPin_, INPUT_PULLUP);
  delayMicroseconds(5U);
  if (digitalRead(dataPin_) == LOW) {
    return false;
  }

  noInterrupts();
  pinMode(dataPin_, OUTPUT);
  digitalWrite(dataPin_, LOW);
  delayMicroseconds(480U);
  pinMode(dataPin_, INPUT_PULLUP);
  delayMicroseconds(70U);
  const bool present = digitalRead(dataPin_) == LOW;
  delayMicroseconds(410U);
  const bool released = digitalRead(dataPin_) == HIGH;
  interrupts();
  return present && released;
}

void Ds18b20TemperatureSensor::busWriteBit(const bool value) {
  noInterrupts();
  pinMode(dataPin_, OUTPUT);
  digitalWrite(dataPin_, LOW);
  if (value) {
    delayMicroseconds(6U);
    pinMode(dataPin_, INPUT_PULLUP);
    delayMicroseconds(64U);
  } else {
    delayMicroseconds(60U);
    pinMode(dataPin_, INPUT_PULLUP);
    delayMicroseconds(10U);
  }
  interrupts();
}

bool Ds18b20TemperatureSensor::busReadBit() {
  noInterrupts();
  pinMode(dataPin_, OUTPUT);
  digitalWrite(dataPin_, LOW);
  delayMicroseconds(3U);
  pinMode(dataPin_, INPUT_PULLUP);
  delayMicroseconds(10U);
  const bool value = digitalRead(dataPin_) == HIGH;
  delayMicroseconds(53U);
  interrupts();
  return value;
}

void Ds18b20TemperatureSensor::busWriteByte(std::uint8_t value) {
  for (std::uint8_t bit = 0U; bit < 8U; ++bit) {
    busWriteBit((value & 0x01U) != 0U);
    value >>= 1U;
  }
}

std::uint8_t Ds18b20TemperatureSensor::busReadByte() {
  std::uint8_t value = 0U;
  for (std::uint8_t bit = 0U; bit < 8U; ++bit) {
    if (busReadBit()) value |= static_cast<std::uint8_t>(1U << bit);
  }
  return value;
}

std::uint8_t Ds18b20TemperatureSensor::crc8(const std::uint8_t* const data,
                                            const std::uint8_t size) {
  std::uint8_t crc = 0U;
  for (std::uint8_t index = 0U; index < size; ++index) {
    std::uint8_t byte = data[index];
    for (std::uint8_t bit = 0U; bit < 8U; ++bit) {
      const bool mix = ((crc ^ byte) & 0x01U) != 0U;
      crc >>= 1U;
      if (mix) crc ^= 0x8CU;
      byte >>= 1U;
    }
  }
  return crc;
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
