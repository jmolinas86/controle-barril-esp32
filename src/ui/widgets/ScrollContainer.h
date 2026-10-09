#pragma once

#include <lvgl.h>

namespace keezer::ui::widgets {

class ScrollContainer final {
 public:
  static lv_obj_t* create(lv_obj_t* parent);
};

}  // namespace keezer::ui::widgets
