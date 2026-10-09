#pragma once

#include <lvgl.h>

#include <cstdint>

namespace keezer::ui::widgets {

class Card final {
 public:
  static lv_obj_t* create(lv_obj_t* parent, std::int32_t x, std::int32_t y,
                          std::int32_t width, std::int32_t height);
  static lv_obj_t* addLabel(lv_obj_t* parent, const char* text,
                            std::int32_t x, std::int32_t y,
                            lv_color_t color,
                            const lv_font_t* font = &lv_font_montserrat_14);
};

}  // namespace keezer::ui::widgets

