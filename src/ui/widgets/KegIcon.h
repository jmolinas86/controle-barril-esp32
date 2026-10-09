#pragma once

#include <lvgl.h>

#include <cstdint>

namespace keezer::ui::widgets {

class KegIcon final {
 public:
  static lv_obj_t* create(lv_obj_t* parent, std::int32_t width,
                          std::int32_t height);
};

}  // namespace keezer::ui::widgets
