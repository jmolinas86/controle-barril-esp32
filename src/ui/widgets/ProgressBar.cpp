#include "ui/widgets/ProgressBar.h"

#include "ui/Theme.h"

namespace keezer::ui::widgets {

lv_obj_t* ProgressBar::create(lv_obj_t* const parent,
                              const std::int32_t width,
                              const std::int32_t height,
                              const std::uint8_t percentage,
                              const lv_color_t indicatorColor) {
  lv_obj_t* const progress = lv_bar_create(parent);
  lv_obj_set_size(progress, width, height);
  lv_bar_set_range(progress, 0, 100);
  lv_bar_set_value(progress, percentage, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(progress, Theme::surfaceRaised(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(progress, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(progress, indicatorColor, LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(progress, LV_OPA_COVER, LV_PART_INDICATOR);
  lv_obj_set_style_radius(progress, height / 2, LV_PART_MAIN);
  lv_obj_set_style_radius(progress, height / 2, LV_PART_INDICATOR);
  return progress;
}

}  // namespace keezer::ui::widgets
