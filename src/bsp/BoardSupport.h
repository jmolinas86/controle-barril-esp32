#pragma once

#include <SPI.h>

#include <array>
#include <cstdint>

#include "bsp/CydDisplay.h"

namespace keezer::bsp {

struct SdCardStatus {
  bool mounted{false};
  bool smokeTestPassed{false};
  std::uint64_t totalBytes{0U};
  std::uint64_t usedBytes{0U};
};

class BoardSupport final {
 public:
  BoardSupport();

  bool begin();
  void beginSafeOutputs();
  bool beginDisplay();
  bool beginTouchCalibration();
  bool beginSdCard();
  bool remountSdCard();
  bool runSdSmokeTest();

  void beginDisplayFrame(std::int32_t firstLine);
  void flushDisplay(std::int32_t x, std::int32_t y, std::int32_t width,
                    std::int32_t height, const std::uint16_t* pixels);
  void waitDisplayTransfer();
  void endDisplayFrame();
  bool readTouch(std::uint16_t& x, std::uint16_t& y);
  void setDisplayBrightnessPercent(std::uint8_t percent);
  void setDisplayAwake(bool awake);
  bool displayAwake() const;
  const SdCardStatus& sdCardStatus() const;
  bool touchCalibrationLoaded() const;

 private:
  bool probeDisplayScanline();
  bool waitForDisplayScanline(std::int32_t firstLine);
  void flushDisplayRaw(std::int32_t x, std::int32_t y, std::int32_t width,
                       std::int32_t height, const std::uint16_t* pixels);
  bool loadTouchCalibration(std::array<std::uint16_t, 8U>& calibration);
  bool saveTouchCalibration(
      const std::array<std::uint16_t, 8U>& calibration);

  CydDisplay display_;
  SPIClass sdSpi_;
  SdCardStatus sdStatus_;
  bool displayReady_{false};
  bool displayFrameOpen_{false};
  bool scanlineSyncAvailable_{false};
  std::uint8_t scanlineSyncFailures_{0U};
  bool calibrationLoaded_{false};
  std::uint8_t displayBrightnessPercent_{70U};
  bool displayAwake_{true};
};

}  // namespace keezer::bsp
