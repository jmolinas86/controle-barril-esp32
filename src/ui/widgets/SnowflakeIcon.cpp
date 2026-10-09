#include "ui/widgets/SnowflakeIcon.h"

namespace keezer::ui::widgets {
namespace {

lv_obj_t* addSegment(lv_obj_t* const parent, const std::int32_t x,
                     const std::int32_t y, const std::int32_t width,
                     const std::int32_t height, const lv_color_t color,
                     const std::int32_t angle = 0) {
  lv_obj_t* const segment = lv_obj_create(parent);
  lv_obj_set_pos(segment, x, y);
  lv_obj_set_size(segment, width, height);
  lv_obj_set_style_bg_color(segment, color, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(segment, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(segment, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(segment, 1, LV_PART_MAIN);
  lv_obj_set_style_pad_all(segment, 0, LV_PART_MAIN);
  lv_obj_set_style_transform_pivot_x(segment, width / 2, LV_PART_MAIN);
  lv_obj_set_style_transform_pivot_y(segment, height / 2, LV_PART_MAIN);
  lv_obj_set_style_transform_rotation(segment, angle, LV_PART_MAIN);
  lv_obj_clear_flag(segment, LV_OBJ_FLAG_SCROLLABLE);
  return segment;
}

void addBranchedArm(lv_obj_t* const parent, const std::int32_t size,
                    const lv_color_t color, const std::int32_t angle) {
  lv_obj_t* const group = lv_obj_create(parent);
  lv_obj_set_pos(group, 0, 0);
  lv_obj_set_size(group, size, size);
  lv_obj_set_style_bg_opa(group, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(group, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(group, 0, LV_PART_MAIN);
  lv_obj_set_style_transform_pivot_x(group, size / 2, LV_PART_MAIN);
  lv_obj_set_style_transform_pivot_y(group, size / 2, LV_PART_MAIN);
  lv_obj_set_style_transform_rotation(group, angle, LV_PART_MAIN);
  lv_obj_clear_flag(group, LV_OBJ_FLAG_SCROLLABLE);

  const std::int32_t center = size / 2;
  addSegment(group, center - 1, 1, 2, size - 2, color);
  if (size < 17) {
    return;
  }

  addSegment(group, center - 4, 3, 2, 6, color, -450);
  addSegment(group, center + 2, 3, 2, 6, color, 450);
  addSegment(group, center - 4, size - 9, 2, 6, color, 450);
  addSegment(group, center + 2, size - 9, 2, 6, color, -450);
}

}  // namespace

lv_obj_t* SnowflakeIcon::create(lv_obj_t* const parent,
                                const std::int32_t size,
                                const lv_color_t color) {
  lv_obj_t* const icon = lv_obj_create(parent);
  lv_obj_set_size(icon, size, size);
  lv_obj_set_style_bg_opa(icon, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(icon, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(icon, 0, LV_PART_MAIN);
  lv_obj_clear_flag(icon, LV_OBJ_FLAG_SCROLLABLE);
  addBranchedArm(icon, size, color, 0);
  addBranchedArm(icon, size, color, 600);
  addBranchedArm(icon, size, color, 1200);
  return icon;
}

}  // namespace keezer::ui::widgets
