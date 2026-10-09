#include "drivers/ScaleHardware.h"

#include <Arduino.h>
#include <Wire.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "BoardConfig.h"
#include "config.h"

namespace balanca::drivers {
namespace {

std::int32_t absoluteValue(const std::int32_t value) {
  return value < 0 ? -value : value;
}

bool validCalibrationFactor(const float value) {
  return std::isfinite(value) && std::fabs(value) >= 1.0F &&
         std::fabs(value) <= 10000000.0F;
}

}  // namespace

ScaleHardware::ScaleHardware()
    : nfc_(board::kPn532Irq, board::kPn532Reset, &Wire),
      display_(board::kOledWidth, board::kOledHeight, &Wire, -1) {}

void ScaleHardware::begin(const storage::DeviceSettings& settings) {
  Wire.begin(board::kI2cSda, board::kI2cScl);
  Wire.setClock(100000U);

  if (i2cDevicePresent(board::kOledAddress)) {
    oledHealthy_ = display_.begin(SSD1306_SWITCHCAPVCC, board::kOledAddress);
    if (oledHealthy_) {
      display_.clearDisplay();
      display_.setTextColor(SSD1306_WHITE);
      display_.setTextSize(1);
      display_.setCursor(0, 0);
      display_.println(F("Control de Barril"));
      display_.println(F(DEVICE_NAME));
      display_.display();
    }
  }
  Serial.println(oledHealthy_ ? F("[OLED] OK")
                              : F("[OLED] NOT FOUND (optional)"));

  scale_.begin(board::kHx711Data, board::kHx711Clock);
  calibrationFactor_ = settings.calibrationFactor;
  scale_.set_scale(calibrationFactor_);
  scale_.set_offset(settings.offset);
  hx711Healthy_ = scale_.wait_ready_timeout(1000U);
  Serial.println(hx711Healthy_ ? F("[HX711] OK")
                               : F("[HX711] NOT READY"));

  if (i2cDevicePresent(0x24U)) {
    nfc_.begin();
    const std::uint32_t version = nfc_.getFirmwareVersion();
    if (version != 0U) {
      nfc_.SAMConfig();
      pn532Healthy_ = true;
      Serial.printf("[PN532] OK - firmware %u.%u\n",
                    static_cast<unsigned>((version >> 16U) & 0xFFU),
                    static_cast<unsigned>((version >> 8U) & 0xFFU));
    }
  }
  if (!pn532Healthy_) Serial.println(F("[PN532] NOT FOUND"));
}

void ScaleHardware::update(const std::uint32_t nowMs) {
  updateWeight(nowMs);
  updateNfc(nowMs);
}

void ScaleHardware::updateWeight(const std::uint32_t nowMs) {
  if (!scale_.is_ready()) {
    if (snapshot_.sequence != 0U &&
        nowMs - snapshot_.lastWeightAtMs >= HX711_STALE_TIMEOUT_MS) {
      if (hx711Healthy_) Serial.println(F("[HX711] STALE"));
      hx711Healthy_ = false;
      snapshot_.weightValid = false;
      snapshot_.stable = false;
    }
    return;
  }

  const float rawSample = static_cast<float>(scale_.get_value(1U));
  const float weightKg = rawSample / calibrationFactor_;
  if (!std::isfinite(weightKg) || weightKg < MIN_VALID_WEIGHT_KG ||
      weightKg > MAX_ABSOLUTE_WEIGHT_KG) {
    snapshot_.weightValid = false;
    snapshot_.stable = false;
    return;
  }

  if (!hx711Healthy_) Serial.println(F("[HX711] OK - data resumed"));
  hx711Healthy_ = true;
  const std::int32_t sampleGrams =
      static_cast<std::int32_t>(std::lround(weightKg * 1000.0F));
  rawSamples_[sampleIndex_] = rawSample;
  weightSamples_[sampleIndex_] = sampleGrams;
  sampleIndex_ = (sampleIndex_ + 1U) % HX711_READ_SAMPLES;
  if (sampleCount_ < HX711_READ_SAMPLES) ++sampleCount_;

  std::int64_t total = 0;
  for (std::uint8_t index = 0U; index < sampleCount_; ++index) {
    total += weightSamples_[index];
  }
  std::int32_t measured = static_cast<std::int32_t>(total / sampleCount_);
  const std::int32_t zeroThreshold = static_cast<std::int32_t>(
      std::lround(ZERO_TRACK_THRESHOLD_KG * 1000.0F));
  if (absoluteValue(measured) <= zeroThreshold || measured < 0) measured = 0;
  const std::int32_t deadband =
      static_cast<std::int32_t>(std::lround(WEIGHT_DEADBAND * 1000.0F));
  if (!snapshot_.weightValid ||
      absoluteValue(measured - snapshot_.weightGrams) >= deadband) {
    snapshot_.weightGrams = measured;
  }
  snapshot_.weightValid = true;
  snapshot_.stable = weightWindowStable();
  snapshot_.lastWeightAtMs = nowMs;
  ++snapshot_.sequence;
}

void ScaleHardware::updateNfc(const std::uint32_t nowMs) {
  if (!pn532Healthy_ || nowMs - lastNfcPollAtMs_ < NFC_INTERVAL_MS) return;
  lastNfcPollAtMs_ = nowMs;
  std::uint8_t uid[7]{};
  std::uint8_t uidLength = 0U;
  if (nfc_.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 1U)) {
    char detected[model::kNfcUidBytes]{};
    std::size_t position = 0U;
    for (std::uint8_t index = 0U;
         index < uidLength && position + 2U < sizeof(detected); ++index) {
      std::snprintf(detected + position, sizeof(detected) - position, "%02X",
                    uid[index]);
      position += 2U;
    }
    lastTagSeenAtMs_ = nowMs;
    if (!snapshot_.hasNfcUid ||
        std::strcmp(snapshot_.nfcUid, detected) != 0) {
      snapshot_.hasNfcUid = true;
      std::snprintf(snapshot_.nfcUid, sizeof(snapshot_.nfcUid), "%s", detected);
      Serial.printf("[NFC] UID: %s\n", snapshot_.nfcUid);
    }
  } else if (snapshot_.hasNfcUid &&
             nowMs - lastTagSeenAtMs_ >= NFC_REMOVE_TIMEOUT_MS) {
    snapshot_.hasNfcUid = false;
    snapshot_.nfcUid[0] = '\0';
    Serial.println(F("[NFC] NONE"));
  }
}

bool ScaleHardware::tare(storage::DeviceSettings& settings) {
  if (!scale_.wait_ready_timeout(250U)) return false;
  scale_.tare(HX711_READ_SAMPLES * 2U);
  settings.offset = static_cast<std::int32_t>(scale_.get_offset());
  clearWeightWindow();
  Serial.printf("[CAL] Tare offset=%ld\n", static_cast<long>(settings.offset));
  return true;
}

bool ScaleHardware::calibrate(const std::int32_t referenceGrams,
                              storage::DeviceSettings& settings) {
  if (referenceGrams <= 0 || sampleCount_ < HX711_READ_SAMPLES ||
      !weightWindowStable()) {
    return false;
  }
  float rawTotal = 0.0F;
  for (std::uint8_t index = 0U; index < sampleCount_; ++index) {
    rawTotal += rawSamples_[index];
  }
  const float rawMean = rawTotal / sampleCount_;
  const float newFactor = rawMean / (static_cast<float>(referenceGrams) / 1000.0F);
  if (!validCalibrationFactor(newFactor)) return false;
  calibrationFactor_ = newFactor;
  settings.calibrationFactor = newFactor;
  scale_.set_scale(newFactor);
  clearWeightWindow();
  Serial.printf("[CAL] Reference=%ldg factor=%.4f\n",
                static_cast<long>(referenceGrams), newFactor);
  return true;
}

void ScaleHardware::updateDisplay(const std::uint32_t nowMs,
                                  const bool wifiConnected,
                                  const bool provisioning,
                                  const std::int32_t rssiDbm,
                                  const IPAddress& ip) {
  if (!oledHealthy_ || nowMs - lastDisplayAtMs_ < DISPLAY_INTERVAL_MS) return;
  lastDisplayAtMs_ = nowMs;
  display_.clearDisplay();
  display_.setTextColor(SSD1306_WHITE);
  display_.setTextSize(1);
  display_.setCursor(0, 0);
  display_.printf("BALANCA %s\n", DEVICE_NAME);
  display_.printf("NFC: %.12s\n", snapshot_.hasNfcUid ? snapshot_.nfcUid : "NONE");
  if (wifiConnected) {
    display_.printf("WiFi: %ld dBm\n", static_cast<long>(rssiDbm));
  } else {
    display_.printf("WiFi: %s\n", provisioning ? "CONFIG" : "OFF");
  }
  display_.printf("IP: %u.%u.%u.%u\n", ip[0], ip[1], ip[2], ip[3]);
  display_.printf("HTTP:%s HX:%s\n", wifiConnected || provisioning ? "OK" : "OFF",
                  hx711Healthy_ ? "OK" : "ERR");
  display_.setTextSize(2);
  display_.setCursor(0, 48);
  if (snapshot_.weightValid) {
    display_.printf("%ld.%02ldkg", static_cast<long>(snapshot_.weightGrams / 1000),
                    static_cast<long>((snapshot_.weightGrams % 1000) / 10));
  } else {
    display_.print(F("--.--kg"));
  }
  display_.display();
}

void ScaleHardware::clearWeightWindow() {
  sampleCount_ = 0U;
  sampleIndex_ = 0U;
  snapshot_.weightValid = false;
  snapshot_.stable = false;
}

bool ScaleHardware::weightWindowStable() const {
  if (sampleCount_ < HX711_READ_SAMPLES) return false;
  std::int32_t minimum = weightSamples_[0];
  std::int32_t maximum = weightSamples_[0];
  for (std::uint8_t index = 1U; index < sampleCount_; ++index) {
    minimum = std::min(minimum, weightSamples_[index]);
    maximum = std::max(maximum, weightSamples_[index]);
  }
  return maximum - minimum <= static_cast<std::int32_t>(
                                  std::lround(CALIBRATION_STABILITY_KG * 1000.0F));
}

bool ScaleHardware::i2cDevicePresent(const std::uint8_t address) const {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0U;
}

const model::ScaleSnapshot& ScaleHardware::snapshot() const { return snapshot_; }
bool ScaleHardware::hx711Healthy() const { return hx711Healthy_; }
bool ScaleHardware::pn532Healthy() const { return pn532Healthy_; }
bool ScaleHardware::oledHealthy() const { return oledHealthy_; }
float ScaleHardware::calibrationFactor() const { return calibrationFactor_; }

}  // namespace balanca::drivers
