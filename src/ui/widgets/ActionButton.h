#pragma once

#include <lvgl.h>

#include <cstdint>

namespace keezer::ui::widgets {

class ActionButton final {
 public:
  static lv_obj_t* create(lv_obj_t* parent, const char* text,
                          std::int32_t width, std::int32_t height,
                          lv_event_cb_t handler, void* context,
                          bool filled = true);
};

}  // namespace keezer::ui::widgets
