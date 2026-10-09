#pragma once

#include <lvgl.h>

#include <cstdint>

namespace keezer::ui::widgets {

class InfoTile final {
 public:
  static lv_obj_t* create(lv_obj_t* parent, const char* label,
                          const char* value, std::int32_t width,
                          std::int32_t height,
                          lv_color_t valueColor,
                          const lv_image_dsc_t* icon = nullptr,
                          lv_obj_t** valueLabelOut = nullptr);
};

}  // namespace keezer::ui::widgets
