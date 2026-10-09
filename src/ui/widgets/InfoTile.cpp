#include "ui/widgets/InfoTile.h"

#include "ui/Theme.h"
#include "ui/widgets/Card.h"

namespace keezer::ui::widgets {

lv_obj_t* InfoTile::create(lv_obj_t* const parent, const char* const label,
                           const char* const value, const std::int32_t width,
                           const std::int32_t height,
                           const lv_color_t valueColor,
                           const lv_image_dsc_t* const icon,
                           lv_obj_t** const valueLabelOut) {
  lv_obj_t* const tile = lv_obj_create(parent);
  lv_obj_set_size(tile, width, height);
  Theme::applyInfoTile(tile);
  lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);

  const std::int32_t textX = icon == nullptr ? 7 : 34;
  const std::int32_t titleY = icon == nullptr ? 6 : 3;
  const std::int32_t valueY = icon == nullptr ? 25 : 21;
  if (icon != nullptr) {
    lv_obj_t* const iconObject = lv_image_create(tile);
    lv_image_set_src(iconObject, icon);
    lv_obj_align(iconObject, LV_ALIGN_LEFT_MID, 3, 0);
  }

  lv_obj_t* const titleLabel =
      Card::addLabel(tile, label, textX, titleY, Theme::primary(),
                     &lv_font_montserrat_12);
  lv_obj_set_width(titleLabel, width - textX - 3);
  lv_label_set_long_mode(titleLabel, LV_LABEL_LONG_CLIP);
  if (icon != nullptr) {
    lv_obj_set_style_text_letter_space(titleLabel, -1, LV_PART_MAIN);
  }
  lv_obj_t* const valueLabel = Card::addLabel(
      tile, value, textX, valueY, valueColor, &lv_font_montserrat_14);
  lv_obj_set_width(valueLabel, width - textX - 3);
  lv_label_set_long_mode(valueLabel, LV_LABEL_LONG_CLIP);
  if (icon != nullptr) {
    lv_obj_set_style_text_letter_space(valueLabel, -1, LV_PART_MAIN);
  }
  if (valueLabelOut != nullptr) {
    *valueLabelOut = valueLabel;
  }
  return tile;
}

}  // namespace keezer::ui::widgets
