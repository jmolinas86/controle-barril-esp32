#pragma once

#include <lvgl.h>

#include <cstdint>

namespace keezer::ui::widgets {

class SnowflakeIcon final {
 public:
  static lv_obj_t* create(lv_obj_t* parent, std::int32_t size,
                          lv_color_t color);
};

}  // namespace keezer::ui::widgets
