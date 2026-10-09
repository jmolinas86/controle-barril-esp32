#include "ui/widgets/FreezerCard.h"

#include <cstdio>
#include <cstring>

#include "ui/Layout.h"
#include "ui/Theme.h"
#include "ui/assets/MiniBitmaps.h"
#include "ui/widgets/Card.h"

namespace keezer::ui::widgets {
namespace {

void addDivider(lv_obj_t* const parent, const std::int32_t x) {
  lv_obj_t* const divider = lv_obj_create(parent);
  lv_obj_set_pos(divider, x, 8);
  lv_obj_set_size(divider, 1, 47);
  lv_obj_set_style_bg_color(divider, Theme::muted(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(divider, LV_OPA_60, LV_PART_MAIN);
  lv_obj_set_style_border_width(divider, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(divider, 0, LV_PART_MAIN);
  lv_obj_clear_flag(divider, LV_OBJ_FLAG_SCROLLABLE);
}

void setLabelText(lv_obj_t* const label, const char* const text) {
  if (label != nullptr && text != nullptr &&
      std::strcmp(lv_label_get_text(label), text) != 0) {
    lv_label_set_text(label, text);
  }
}

lv_color_t compressorColor(const app::FreezerViewData& data) {
  if (data.compressorState != nullptr &&
      std::strcmp(data.compressorState, "ERROR") == 0) {
    return Theme::danger();
  }
  if (data.compressorOn) {
    return Theme::good();
  }
  if (data.compressorState != nullptr &&
      std::strcmp(data.compressorState, "WAITING") == 0) {
    return Theme::warning();
  }
  return Theme::muted();
}

}  // namespace

lv_obj_t* FreezerCard::create(lv_obj_t* const parent,
                              const app::FreezerViewData& data) {
  lv_obj_t* const card = Card::create(parent, 0, 0, layout::kCardWidth, 64);
  lv_obj_set_style_border_color(card, Theme::primary(), LV_PART_MAIN);

  lv_obj_t* const thermometer = lv_image_create(card);
  lv_image_set_src(thermometer,
                   &keezer::ui::assets::kKeezerTemperature);
  lv_obj_set_pos(thermometer, 5, 5);

  Card::addLabel(card, "KEEZER", 37, 5, Theme::primary(),
                 &lv_font_montserrat_10);

  temperatureValue_ = Card::addLabel(card, "--.-", 42, 22, Theme::text(),
                                     &lv_font_montserrat_24);
  Card::addLabel(card, "°C", 83, 37, Theme::text(),
                 &lv_font_montserrat_10);

  addDivider(card, 100);
  lv_obj_t* const setpointTitle =
      Card::addLabel(card, "SET POINT", 105, 5, Theme::primary(),
                     &lv_font_montserrat_10);
  lv_obj_set_width(setpointTitle, 54);
  lv_obj_set_style_text_align(setpointTitle, LV_TEXT_ALIGN_CENTER,
                              LV_PART_MAIN);
  setpointValue_ = Card::addLabel(card, "--.-", 111, 23, Theme::text(),
                                  &lv_font_montserrat_20);
  Card::addLabel(card, "°C", 143, 35, Theme::text(),
                 &lv_font_montserrat_10);

  addDivider(card, 162);
  lv_obj_t* const statusTitle =
      Card::addLabel(card, "STATUS", 160, 3, Theme::primary(),
                     &lv_font_montserrat_10);
  lv_obj_set_width(statusTitle, 68);
  lv_obj_set_style_text_align(statusTitle, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_t* const compressorTitle =
      Card::addLabel(card, "COMPRESS", 160, 15, Theme::primary(),
                     &lv_font_montserrat_10);
  lv_obj_set_width(compressorTitle, 68);
  lv_obj_set_style_text_align(compressorTitle, LV_TEXT_ALIGN_CENTER,
                              LV_PART_MAIN);
  stateLabel_ = Card::addLabel(card, "---", 160, 28, Theme::good(),
                               &lv_font_montserrat_10);
  lv_obj_set_width(stateLabel_, 68);
  lv_obj_set_style_text_align(stateLabel_, LV_TEXT_ALIGN_CENTER,
                              LV_PART_MAIN);

  stateIcon_ = lv_image_create(card);
  lv_image_set_src(stateIcon_, &assets::kHeaderSnowflake);
  lv_obj_set_pos(stateIcon_, 185, 42);
  update(data);
  return card;
}

void FreezerCard::update(const app::FreezerViewData& data) {
  if (temperatureValue_ == nullptr || setpointValue_ == nullptr ||
      stateLabel_ == nullptr || stateIcon_ == nullptr) {
    return;
  }

  char value[20]{};
  std::snprintf(value, sizeof(value), "%.1f",
                static_cast<double>(data.temperatureC));
  setLabelText(temperatureValue_, value);
  std::snprintf(value, sizeof(value), "%.1f",
                static_cast<double>(data.setpointC));
  setLabelText(setpointValue_, value);

  setLabelText(stateLabel_,
               data.compressorState == nullptr ? "---" : data.compressorState);
  lv_obj_set_style_text_color(stateLabel_, compressorColor(data), LV_PART_MAIN);

  const bool showCoolingIcon = data.compressorOn;
  if (showCoolingIcon) {
    lv_obj_remove_flag(stateIcon_, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(stateIcon_, LV_OBJ_FLAG_HIDDEN);
  }
}

}  // namespace keezer::ui::widgets
