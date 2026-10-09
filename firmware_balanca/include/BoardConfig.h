#pragma once

#include <Arduino.h>

namespace balanca::board {

inline constexpr uint8_t kI2cSda = D2;
inline constexpr uint8_t kI2cScl = D1;
inline constexpr uint8_t kHx711Clock = D5;
inline constexpr uint8_t kHx711Data = D6;
inline constexpr uint8_t kPn532Irq = D7;
inline constexpr uint8_t kPn532Reset = D0;
inline constexpr uint8_t kOledAddress = 0x3C;
inline constexpr uint8_t kOledWidth = 128;
inline constexpr uint8_t kOledHeight = 64;

}  // namespace balanca::board
