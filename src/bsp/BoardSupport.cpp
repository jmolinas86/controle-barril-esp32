#include "bsp/BoardSupport.h"

#include <Arduino.h>
#include <Preferences.h>
#include <SD.h>

#include <algorithm>
#include <array>
#include <cstring>

#include "BuildConfig.h"
#include "bsp/BoardConfig.h"
#include "diagnostics/Logger.h"

namespace keezer::bsp {
namespace {

constexpr char kLogTag[] = "BSP";
constexpr char kTouchNamespace[] = "touch-cal";
constexpr char kTouchMagicKey[] = "magic";
constexpr char kTouchDataKey[] = "data";
constexpr char kSdSmokeTestPath[] = "/phase1.tmp";
constexpr char kSdSmokeTestContents[] = "keezer-phase-01\n";

}  // namespace

BoardSupport::BoardSupport() : sdSpi_(VSPI) {}

bool BoardSupport::begin() {
  beginSafeOutputs();

  if (!beginDisplay()) {
    KEEZER_LOG_ERROR(kLogTag, "Display initialization failed");
    return false;
  }

  const bool touchReady = beginTouchCalibration();
  const bool sdReady = beginSdCard();

  if (!touchReady) {
    KEEZER_LOG_WARN(kLogTag, "Touch calibration is unavailable");
  }
  if (!sdReady) {
    KEEZER_LOG_WARN(kLogTag, "microSD unavailable; continuing degraded");
  }

  return true;
}

void BoardSupport::beginSafeOutputs() {
  // Physical compressor control is deliberately disabled until the relay
  // polarity and the exact board revision have been verified.
  if constexpr (config::kEnableCompressorGpio) {
    const std::uint8_t offLevel =
        pins::kCompressorRelayActiveHigh ? LOW : HIGH;
    digitalWrite(pins::kCompressorRelayCandidate, offLevel);
    pinMode(pins::kCompressorRelayCandidate, OUTPUT);
    digitalWrite(pins::kCompressorRelayCandidate, offLevel);
    KEEZER_LOG_WARN(kLogTag, "Compressor GPIO enabled and forced OFF");
  } else {
    KEEZER_LOG_INFO(kLogTag, "Compressor GPIO disabled and kept logically OFF");
  }
}

bool BoardSupport::beginDisplay() {
  display_.init();
  display_.setRotation(0);
  // LVGL renders directly as RGB565_SWAPPED. Keeping LovyanGFX byte swapping
  // disabled lets the partial buffer be sent by DMA without a CPU conversion.
  display_.setSwapBytes(false);
  display_.setBrightness(config::kDisplayBrightness);
  display_.fillScreen(0x0000U);
  displayReady_ = display_.width() == config::kDisplayWidth &&
                  display_.height() == config::kDisplayHeight;

  KEEZER_LOG_INFO(kLogTag, "Display %ldx%ld portrait: %s",
                  static_cast<long>(display_.width()),
                  static_cast<long>(display_.height()),
                  displayReady_ ? "OK" : "unexpected geometry");
  scanlineSyncAvailable_ =
      displayReady_ && config::kDisplayScanlineSyncEnabled &&
      probeDisplayScanline();
  KEEZER_LOG_INFO(kLogTag, "Display scanline synchronization: %s",
                  scanlineSyncAvailable_ ? "AVAILABLE" : "FALLBACK");
  return displayReady_;
}

bool BoardSupport::beginTouchCalibration() {
  if (!displayReady_) {
    return false;
  }

  std::array<std::uint16_t, 8U> calibration{};
  calibrationLoaded_ =
      !config::kForceTouchCalibration && loadTouchCalibration(calibration);

  if (calibrationLoaded_) {
    display_.setTouchCalibrate(calibration.data());
    KEEZER_LOG_INFO(kLogTag, "Touch calibration loaded from NVS");
    return true;
  }

  KEEZER_LOG_INFO(kLogTag,
                  "Starting touch calibration; follow targets on display");
  display_.calibrateTouch(calibration.data(), 0xF81FU, 0x0000U, 15U);

  if (!saveTouchCalibration(calibration)) {
    KEEZER_LOG_WARN(kLogTag, "Could not persist touch calibration");
    return true;
  }

  calibrationLoaded_ = true;
  KEEZER_LOG_INFO(kLogTag, "Touch calibration saved to NVS");
  return true;
}

bool BoardSupport::beginSdCard() {
  sdStatus_ = {};
  sdSpi_.begin(pins::kSdSclk, pins::kSdMiso, pins::kSdMosi, pins::kSdCs);
  sdStatus_.mounted =
      SD.begin(pins::kSdCs, sdSpi_, config::kSdClockHz, "/sd", 5U, false);
  if (!sdStatus_.mounted) {
    return false;
  }

  const auto cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    sdStatus_.mounted = false;
    return false;
  }

  sdStatus_.totalBytes = SD.totalBytes();
  sdStatus_.usedBytes = SD.usedBytes();
  sdStatus_.smokeTestPassed =
      !config::kRunSdSmokeTest || runSdSmokeTest();

  KEEZER_LOG_INFO(kLogTag, "microSD mounted: %llu MB total, smoke=%s",
                  static_cast<unsigned long long>(sdStatus_.totalBytes /
                                                  (1024ULL * 1024ULL)),
                  sdStatus_.smokeTestPassed ? "OK" : "FAILED");
  return true;
}

bool BoardSupport::remountSdCard() {
  SD.end();
  const bool mounted = beginSdCard();
  KEEZER_LOG_INFO(kLogTag, "microSD recovery: %s",
                  mounted ? "MOUNTED" : "UNAVAILABLE");
  return mounted;
}

bool BoardSupport::runSdSmokeTest() {
  if (!sdStatus_.mounted) {
    return false;
  }

  if (SD.exists(kSdSmokeTestPath)) {
    SD.remove(kSdSmokeTestPath);
  }

  File output = SD.open(kSdSmokeTestPath, FILE_WRITE);
  if (!output) {
    return false;
  }
  const std::size_t expectedLength = std::strlen(kSdSmokeTestContents);
  const std::size_t written = output.write(
      reinterpret_cast<const std::uint8_t*>(kSdSmokeTestContents),
      expectedLength);
  output.flush();
  output.close();
  if (written != expectedLength) {
    SD.remove(kSdSmokeTestPath);
    return false;
  }

  File input = SD.open(kSdSmokeTestPath, FILE_READ);
  if (!input) {
    SD.remove(kSdSmokeTestPath);
    return false;
  }

  std::array<char, sizeof(kSdSmokeTestContents)> contents{};
  const std::size_t read = input.readBytes(contents.data(), expectedLength);
  input.close();
  const bool valid = read == expectedLength &&
                     std::memcmp(contents.data(), kSdSmokeTestContents,
                                 expectedLength) == 0;
  const bool removed = SD.remove(kSdSmokeTestPath);
  return valid && removed;
}

void BoardSupport::beginDisplayFrame(const std::int32_t firstLine) {
  if (!displayReady_) {
    return;
  }
  if (displayFrameOpen_) {
    endDisplayFrame();
  }

  if (scanlineSyncAvailable_) {
    if (waitForDisplayScanline(firstLine)) {
      scanlineSyncFailures_ = 0U;
    } else if (++scanlineSyncFailures_ >= 3U) {
      scanlineSyncAvailable_ = false;
      KEEZER_LOG_WARN(
          kLogTag,
          "Display scanline synchronization lost; using DMA fallback");
    }
  }

  // Keep an outer LovyanGFX transaction open. pushImageDMA() then queues the
  // transfer without waiting at the end of every LVGL strip.
  display_.startWrite();
  displayFrameOpen_ = true;
}

void BoardSupport::flushDisplay(const std::int32_t x, const std::int32_t y,
                                const std::int32_t width,
                                const std::int32_t height,
                                const std::uint16_t* const pixels) {
  if (!displayReady_ || pixels == nullptr || width <= 0 || height <= 0) {
    return;
  }
  if (!displayFrameOpen_) {
    beginDisplayFrame(y);
  }

  flushDisplayRaw(x, y, width, height, pixels);
}

void BoardSupport::endDisplayFrame() {
  if (!displayFrameOpen_) {
    return;
  }
  display_.waitDMA();
  display_.endWrite();
  displayFrameOpen_ = false;
}

void BoardSupport::waitDisplayTransfer() {
  if (displayFrameOpen_) {
    display_.waitDMA();
  }
}

bool BoardSupport::readTouch(std::uint16_t& x, std::uint16_t& y) {
  if (!displayReady_) {
    return false;
  }
  return display_.getTouch(&x, &y);
}

void BoardSupport::setDisplayBrightnessPercent(const std::uint8_t percent) {
  displayBrightnessPercent_ = std::max<std::uint8_t>(
      10U, std::min<std::uint8_t>(100U, percent));
  if (displayAwake_) {
    display_.setBrightness(static_cast<std::uint8_t>(
        (static_cast<std::uint16_t>(displayBrightnessPercent_) * 255U) /
        100U));
  }
}

void BoardSupport::setDisplayAwake(const bool awake) {
  if (displayAwake_ == awake) return;
  displayAwake_ = awake;
  display_.setBrightness(
      awake ? static_cast<std::uint8_t>(
                  (static_cast<std::uint16_t>(displayBrightnessPercent_) *
                   255U) /
                  100U)
            : 0U);
}

bool BoardSupport::displayAwake() const { return displayAwake_; }

const SdCardStatus& BoardSupport::sdCardStatus() const { return sdStatus_; }

bool BoardSupport::touchCalibrationLoaded() const {
  return calibrationLoaded_;
}

bool BoardSupport::probeDisplayScanline() {
  const std::int32_t first = display_.getScanLine();
  delay(2U);
  const std::int32_t second = display_.getScanLine();
  const auto valid = [](const std::int32_t line) {
    return line >= 0 && line < config::kDisplayHeight;
  };
  return valid(first) && valid(second) && first != second;
}

bool BoardSupport::waitForDisplayScanline(const std::int32_t firstLine) {
  const std::int32_t displayHeight = config::kDisplayHeight;
  const std::int32_t clampedFirstLine =
      std::max<std::int32_t>(0, std::min<std::int32_t>(
                                   firstLine, displayHeight - 1));
  const std::int32_t target =
      (clampedFirstLine + config::kDisplayScanlineGuardLines) % displayHeight;
  const std::uint32_t startedAtUs = micros();
  std::uint8_t invalidReads = 0U;

  while (micros() - startedAtUs < config::kDisplayScanlineSyncTimeoutUs) {
    const std::int32_t scanline = display_.getScanLine();
    if (scanline < 0 || scanline >= displayHeight) {
      if (++invalidReads >= 3U) {
        return false;
      }
    } else {
      invalidReads = 0U;
      const std::int32_t distance =
          (scanline + displayHeight - target) % displayHeight;
      if (distance <= config::kDisplayScanlineWindowLines) {
        return true;
      }
    }
    delayMicroseconds(100U);
  }
  return false;
}

void BoardSupport::flushDisplayRaw(const std::int32_t x,
                                   const std::int32_t y,
                                   const std::int32_t width,
                                   const std::int32_t height,
                                   const std::uint16_t* const pixels) {
  if (height > 0) {
    display_.pushImageDMA(x, y, width, height, pixels);
  }
}

bool BoardSupport::loadTouchCalibration(
    std::array<std::uint16_t, 8U>& calibration) {
  Preferences preferences;
  if (!preferences.begin(kTouchNamespace, true)) {
    return false;
  }

  const bool validMagic =
      preferences.getUInt(kTouchMagicKey, 0U) ==
      config::kTouchCalibrationMagic;
  const std::size_t storedLength = preferences.getBytesLength(kTouchDataKey);
  const bool validLength = storedLength == sizeof(calibration);
  const std::size_t loaded = validMagic && validLength
                                 ? preferences.getBytes(
                                       kTouchDataKey, calibration.data(),
                                       sizeof(calibration))
                                 : 0U;
  preferences.end();
  return loaded == sizeof(calibration);
}

bool BoardSupport::saveTouchCalibration(
    const std::array<std::uint16_t, 8U>& calibration) {
  Preferences preferences;
  if (!preferences.begin(kTouchNamespace, false)) {
    return false;
  }

  const bool magicSaved =
      preferences.putUInt(kTouchMagicKey,
                          config::kTouchCalibrationMagic) == sizeof(std::uint32_t);
  const bool dataSaved =
      preferences.putBytes(kTouchDataKey, calibration.data(),
                           sizeof(calibration)) == sizeof(calibration);
  preferences.end();
  return magicSaved && dataSaved;
}

}  // namespace keezer::bsp
