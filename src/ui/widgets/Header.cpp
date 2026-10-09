#include "ui/widgets/Header.h"

#include <cstdio>

#include "ui/Layout.h"
#include "ui/Theme.h"
#include "ui/assets/MiniBitmaps.h"

namespace keezer::ui::widgets {

bool Header::create(lv_obj_t* const parent) {
  lv_obj_t* const container = lv_obj_create(parent);
  lv_obj_set_pos(container, 0, 0);
  lv_obj_set_size(container, layout::kScreenWidth, layout::kHeaderHeight);
  Theme::applyHeader(container);
  lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

  // Keep the header height unchanged, but move its visual divider 5 px up.
  lv_obj_t* const divider = lv_obj_create(container);
  lv_obj_set_pos(divider, 0, layout::kHeaderHeight - 6);
  lv_obj_set_size(divider, layout::kScreenWidth, 1);
  lv_obj_set_style_bg_color(divider, Theme::primary(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(divider, LV_OPA_70, LV_PART_MAIN);
  lv_obj_set_style_border_width(divider, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(divider, 0, LV_PART_MAIN);
  lv_obj_clear_flag(divider, LV_OBJ_FLAG_SCROLLABLE);

  backButton_ = lv_button_create(container);
  lv_obj_set_pos(backButton_, 0, 0);
  lv_obj_set_size(backButton_, 31, 30);
  lv_obj_set_style_bg_opa(backButton_, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(backButton_, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(backButton_, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(backButton_, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(backButton_, 0, LV_PART_MAIN);
  lv_obj_add_event_cb(backButton_, handleBack, LV_EVENT_CLICKED, this);

  lv_obj_t* const backIcon = lv_label_create(backButton_);
  lv_label_set_text(backIcon, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_color(backIcon, Theme::text(), LV_PART_MAIN);
  lv_obj_set_style_text_font(backIcon, &lv_font_montserrat_20, LV_PART_MAIN);
  lv_obj_center(backIcon);

  titleLabel_ = lv_label_create(container);
  lv_obj_set_pos(titleLabel_, 29, 8);
  lv_obj_set_width(titleLabel_, 165);
  lv_label_set_long_mode(titleLabel_, LV_LABEL_LONG_CLIP);
  lv_obj_set_style_text_color(titleLabel_, Theme::text(), LV_PART_MAIN);
  lv_obj_set_style_text_font(titleLabel_, &lv_font_montserrat_12,
                             LV_PART_MAIN);

  connectivityLabel_ = lv_label_create(container);
  lv_label_set_text(connectivityLabel_, LV_SYMBOL_WIFI " 12:30");
  lv_obj_set_pos(connectivityLabel_, 177, 8);
  lv_obj_set_width(connectivityLabel_, 58);
  lv_obj_set_style_text_align(connectivityLabel_, LV_TEXT_ALIGN_RIGHT,
                              LV_PART_MAIN);
  lv_obj_set_style_text_color(connectivityLabel_, Theme::warning(),
                              LV_PART_MAIN);
  lv_obj_set_style_text_font(connectivityLabel_, &lv_font_montserrat_12,
                             LV_PART_MAIN);

  homeIcon_ = lv_image_create(container);
  lv_image_set_src(homeIcon_, &keezer::ui::assets::kHeaderSnowflake);
  lv_obj_set_pos(homeIcon_, 7, 6);

  showTitle("KEEZER", false);
  return titleLabel_ != nullptr && backButton_ != nullptr;
}

void Header::setNetworkState(const app::NetworkViewData& network) {
  if (connectivityLabel_ == nullptr) return;
  lv_obj_set_style_text_color(
      connectivityLabel_,
      network.connected
          ? Theme::text()
          : (network.status == models::NetworkStatus::ConfigurationError
                 ? Theme::danger()
                 : Theme::warning()),
      LV_PART_MAIN);
}

void Header::setBackHandler(const BackHandler handler, void* const context) {
  backHandler_ = handler;
  backContext_ = context;
}

void Header::showHome(const std::size_t kegCount) {
  (void)kegCount;
  showTitle("KEEZER CONTROL", false);
  if (homeIcon_ != nullptr) {
    lv_obj_remove_flag(homeIcon_, LV_OBJ_FLAG_HIDDEN);
  }
}

void Header::showTitle(const char* const title, const bool showBack) {
  if (titleLabel_ != nullptr) {
    lv_label_set_text(titleLabel_, title);
    lv_obj_set_x(titleLabel_, showBack ? 33 : 29);
    lv_obj_set_width(titleLabel_, showBack ? 139 : 143);
  }
  if (backButton_ != nullptr) {
    if (showBack) {
      lv_obj_remove_flag(backButton_, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(backButton_, LV_OBJ_FLAG_HIDDEN);
    }
  }
  if (homeIcon_ != nullptr) {
    lv_obj_add_flag(homeIcon_, LV_OBJ_FLAG_HIDDEN);
  }
}

void Header::handleBack(lv_event_t* const event) {
  auto* const self = static_cast<Header*>(lv_event_get_user_data(event));
  if (self != nullptr && self->backHandler_ != nullptr) {
    self->backHandler_(self->backContext_);
  }
}

}  // namespace keezer::ui::widgets
