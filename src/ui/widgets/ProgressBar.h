#pragma once

#include <lvgl.h>

#include <cstdint>

namespace keezer::ui::widgets {

class ProgressBar final {
 public:
  static lv_obj_t* create(lv_obj_t* parent, std::int32_t width,
                          std::int32_t height, std::uint8_t percentage,
                          lv_color_t indicatorColor);
};

}  // namespace keezer::ui::widgets
