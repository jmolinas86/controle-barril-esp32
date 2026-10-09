#pragma once

#include <cstdint>

namespace keezer::bsp::pins {

// Confirmed ESP32-2432S028 dual-USB revision (USB-C + micro-USB), with
// ST7789 display and XPT2046 resistive touch.
inline constexpr std::int8_t kTftMiso = 12;
inline constexpr std::int8_t kTftMosi = 13;
inline constexpr std::int8_t kTftSclk = 14;
inline constexpr std::int8_t kTftCs = 15;
inline constexpr std::int8_t kTftDc = 2;
inline constexpr std::int8_t kTftReset = -1;
inline constexpr std::int8_t kTftBacklight = 21;

inline constexpr std::int8_t kTouchMiso = 39;
inline constexpr std::int8_t kTouchMosi = 32;
inline constexpr std::int8_t kTouchSclk = 25;
inline constexpr std::int8_t kTouchCs = 33;
inline constexpr std::int8_t kTouchIrq = 36;

inline constexpr std::int8_t kSdMiso = 19;
inline constexpr std::int8_t kSdMosi = 23;
inline constexpr std::int8_t kSdSclk = 18;
inline constexpr std::int8_t kSdCs = 5;

inline constexpr std::int8_t kRgbRed = 4;
inline constexpr std::int8_t kRgbGreen = 16;
inline constexpr std::int8_t kRgbBlue = 17;

inline constexpr std::int8_t kTemperatureDataCandidate = 22;
inline constexpr std::int8_t kCompressorRelayCandidate = 27;

// Confirmed relay module: energized with HIGH, safe/off with LOW.
inline constexpr bool kCompressorRelayActiveHigh = true;

}  // namespace keezer::bsp::pins
