#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include "bsp/BoardConfig.h"

namespace keezer::bsp {

class CydDisplay final : public lgfx::LGFX_Device {
 public:
  CydDisplay();

 private:
  lgfx::Panel_ST7789 panel_;
  lgfx::Bus_SPI bus_;
  lgfx::Light_PWM light_;
  lgfx::Touch_XPT2046 touch_;
};

}  // namespace keezer::bsp
