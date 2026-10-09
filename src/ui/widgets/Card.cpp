#include "ui/widgets/Card.h"

#include "ui/Theme.h"

namespace keezer::ui::widgets {

lv_obj_t* Card::create(lv_obj_t* const parent, const std::int32_t x,
                       const std::int32_t y, const std::int32_t width,
                       const std::int32_t height) {
  lv_obj_t* const card = lv_obj_create(parent);
  lv_obj_set_pos(card, x, y);
  lv_obj_set_size(card, width, height);
  Theme::applyCard(card);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  return card;
}

lv_obj_t* Card::addLabel(lv_obj_t* const parent, const char* const text,
                         const std::int32_t x, const std::int32_t y,
                         const lv_color_t color,
                         const lv_font_t* const font) {
  lv_obj_t* const label = lv_label_create(parent);
  lv_label_set_text(label, text);
  lv_obj_set_pos(label, x, y);
  lv_obj_set_style_text_color(label, color, LV_PART_MAIN);
  lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
  return label;
}

}  // namespace keezer::ui::widgets

