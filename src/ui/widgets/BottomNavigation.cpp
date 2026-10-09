#include "ui/widgets/BottomNavigation.h"

#include <cstddef>

#include "ui/Layout.h"
#include "ui/Theme.h"
#include "ui/assets/MiniBitmaps.h"

namespace keezer::ui::widgets {
namespace {

constexpr std::array<Route, 3U> kRoutes{
    Route::Home, Route::Temperature, Route::Settings};
constexpr std::array<const char*, 3U> kIcons{
    LV_SYMBOL_HOME, "", LV_SYMBOL_SETTINGS};
constexpr std::array<const char*, 3U> kLabels{
    "HOME", "KEEZER", "CONFIG."};

}  // namespace

bool BottomNavigation::create(lv_obj_t* const parent,
                              const RouteHandler handler,
                              void* const context) {
  routeHandler_ = handler;
  routeContext_ = context;

  lv_obj_t* const container = lv_obj_create(parent);
  lv_obj_set_pos(container, 0, layout::kNavigationY);
  lv_obj_set_size(container, layout::kScreenWidth,
                  layout::kNavigationHeight);
  Theme::applyNavContainer(container);
  lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

  for (std::size_t index = 0U; index < buttons_.size(); ++index) {
    lv_obj_t* const button = lv_button_create(container);
    buttons_[index] = button;
    lv_obj_set_pos(
        button,
        static_cast<std::int32_t>(index) * layout::kNavigationButtonWidth, 0);
    lv_obj_set_size(button, layout::kNavigationButtonWidth,
                    layout::kNavigationHeight);
    Theme::applyNavButton(button, index == 0U);
    lv_obj_add_event_cb(button, handleClick, LV_EVENT_CLICKED, this);

    lv_obj_t* icon = nullptr;
    if (index == 1U) {
      icon = lv_image_create(button);
      lv_image_set_src(icon, &assets::kFreezerNav);
      lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 2);
      lv_obj_set_style_image_recolor(icon, Theme::muted(), LV_PART_MAIN);
      lv_obj_set_style_image_recolor_opa(icon, LV_OPA_COVER, LV_PART_MAIN);
    } else {
      icon = lv_label_create(button);
      lv_label_set_text(icon, kIcons[index]);
      lv_obj_set_pos(icon, 0, 2);
      lv_obj_set_width(icon, layout::kNavigationButtonWidth);
      lv_obj_set_style_text_align(icon, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
      lv_obj_set_style_text_font(icon, &lv_font_montserrat_16, LV_PART_MAIN);
    }
    iconLabels_[index] = icon;

    lv_obj_t* const label = lv_label_create(button);
    textLabels_[index] = label;
    lv_label_set_text(label, kLabels[index]);
    lv_obj_set_pos(label, 0, 25);
    lv_obj_set_width(label, layout::kNavigationButtonWidth);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_8, LV_PART_MAIN);
  }
  return true;
}

void BottomNavigation::setActive(const Route route) {
  const Route highlighted =
      (route == Route::KegDetail || route == Route::KegEdit)
          ? Route::Home
          : ((route == Route::NetworkSettings ||
              route == Route::ArchivedKegs ||
              route == Route::ArchivedKegDetail ||
              route == Route::DisplaySettings)
                 ? Route::Settings
                 : route);
  for (std::size_t index = 0U; index < buttons_.size(); ++index) {
    if (buttons_[index] != nullptr) {
      const bool active = kRoutes[index] == highlighted;
      Theme::applyNavButton(buttons_[index], active);
      const lv_color_t color = active ? Theme::text() : Theme::muted();
      if (iconLabels_[index] != nullptr) {
        if (index == 1U) {
          lv_obj_set_style_image_recolor(iconLabels_[index], color,
                                         LV_PART_MAIN);
        } else {
          lv_obj_set_style_text_color(iconLabels_[index], color, LV_PART_MAIN);
        }
      }
      if (textLabels_[index] != nullptr) {
        lv_obj_set_style_text_color(textLabels_[index], color, LV_PART_MAIN);
      }
    }
  }
}

void BottomNavigation::handleClick(lv_event_t* const event) {
  auto* const self =
      static_cast<BottomNavigation*>(lv_event_get_user_data(event));
  if (self == nullptr || self->routeHandler_ == nullptr) {
    return;
  }

  lv_obj_t* const target = static_cast<lv_obj_t*>(lv_event_get_target(event));
  for (std::size_t index = 0U; index < self->buttons_.size(); ++index) {
    if (self->buttons_[index] == target) {
      self->routeHandler_(kRoutes[index], self->routeContext_);
      return;
    }
  }
}

}  // namespace keezer::ui::widgets
