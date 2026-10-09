#include "ui/widgets/KegCard.h"

#include <cstdio>
#include <cstring>

#include "ui/Layout.h"
#include "ui/Theme.h"
#include "ui/TimeFormat.h"
#include "ui/assets/MiniBitmaps.h"
#include "ui/widgets/ActionButton.h"
#include "ui/widgets/Card.h"
#include "ui/widgets/KegIcon.h"
#include "ui/widgets/ProgressBar.h"

namespace keezer::ui::widgets {
namespace {

lv_color_t levelColor(const app::KegViewData& data) {
  if (data.percentage >= 50U) {
    return Theme::good();
  }
  if (data.percentage >= 15U) {
    return Theme::warning();
  }
  return Theme::danger();
}

void addDivider(lv_obj_t* const parent, const std::int32_t x,
                const std::int32_t y, const std::int32_t height) {
  lv_obj_t* const divider = lv_obj_create(parent);
  lv_obj_set_pos(divider, x, y);
  lv_obj_set_size(divider, 1, height);
  lv_obj_set_style_bg_color(divider, Theme::muted(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(divider, LV_OPA_50, LV_PART_MAIN);
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

}  // namespace

lv_obj_t* KegCard::create(lv_obj_t* const parent,
                          const app::KegViewData& data,
                          const std::size_t index,
                          const DetailsHandler handler, void* const context) {
  index_ = index;
  detailsHandler_ = handler;
  detailsContext_ = context;

  lv_obj_t* const card = Card::create(parent, 0, 0, layout::kCardWidth, 86);
  numberLabel_ = Card::addLabel(card, "KEG --", 6, 5, Theme::text(),
                                &lv_font_montserrat_10);
  addDivider(card, 53, 4, 15);

  nameLabel_ = Card::addLabel(card, "---", 59, 5, Theme::text(),
                              &lv_font_montserrat_12);
  lv_obj_set_width(nameLabel_, 94);
  lv_label_set_long_mode(nameLabel_, LV_LABEL_LONG_CLIP);

  addDivider(card, 53, 23, 57);
  lv_obj_t* const kegIcon = KegIcon::create(card, 36, 51);
  lv_obj_set_pos(kegIcon, 8, 27);

  Card::addLabel(card, "VOLUME ATUAL", 59, 24, Theme::primary(),
                 &lv_font_montserrat_10);
  volumeLabel_ = Card::addLabel(card, "--.- L", 59, 35, Theme::text(),
                                &lv_font_montserrat_20);

  progress_ = ProgressBar::create(card, 69, 7, 0U, Theme::muted());
  lv_obj_set_pos(progress_, 59, 59);
  percentageLabel_ = Card::addLabel(card, "--%", 131, 55, Theme::muted(),
                                    &lv_font_montserrat_10);

  syncLabel_ = Card::addLabel(card, "SYNC: --/-- --:--", 59, 70,
                              Theme::muted(), &lv_font_montserrat_10);

  lv_obj_t* const statusTitle =
      Card::addLabel(card, "STATUS", 162, 14, Theme::primary(),
                     &lv_font_montserrat_10);
  lv_obj_set_width(statusTitle, 60);
  lv_obj_set_style_text_align(statusTitle, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  statusLabel_ = Card::addLabel(card, "---", 154, 30, Theme::muted(),
                                &lv_font_montserrat_8);
  lv_obj_set_width(statusLabel_, 76);
  lv_label_set_long_mode(statusLabel_, LV_LABEL_LONG_CLIP);
  lv_obj_set_style_text_align(statusLabel_, LV_TEXT_ALIGN_CENTER,
                              LV_PART_MAIN);
  lv_obj_set_style_text_letter_space(statusLabel_, 0, LV_PART_MAIN);

  lv_obj_t* const details = ActionButton::create(
      card, "", 60, 30, handleDetails, this, false);
  lv_obj_set_pos(details, 162, 44);
  lv_obj_set_style_bg_color(details, Theme::surfaceRaised(), LV_PART_MAIN);
  lv_obj_set_style_bg_grad_dir(details, LV_GRAD_DIR_NONE, LV_PART_MAIN);
  lv_obj_set_style_border_color(details, Theme::primary(), LV_PART_MAIN);
  lv_obj_set_style_border_width(details, 1, LV_PART_MAIN);
  lv_obj_set_style_radius(details, 5, LV_PART_MAIN);
  lv_obj_t* const detailsIcon = lv_image_create(details);
  lv_image_set_src(detailsIcon, &assets::kDetailsList);
  lv_obj_center(detailsIcon);
  update(data);
  return card;
}

void KegCard::update(const app::KegViewData& data) {
  if (numberLabel_ == nullptr || nameLabel_ == nullptr ||
      volumeLabel_ == nullptr || progress_ == nullptr ||
      percentageLabel_ == nullptr || syncLabel_ == nullptr ||
      statusLabel_ == nullptr) {
    return;
  }

  char text[32]{};
  std::snprintf(text, sizeof(text), "KEG %02u",
                static_cast<unsigned int>(data.number));
  setLabelText(numberLabel_, text);
  setLabelText(nameLabel_, data.name == nullptr ? "---" : data.name);

  std::snprintf(text, sizeof(text), "%.1f L",
                static_cast<double>(data.volumeL));
  setLabelText(volumeLabel_, text);

  const lv_color_t indicator = levelColor(data);
  lv_bar_set_value(progress_, data.percentage, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(progress_, indicator, LV_PART_INDICATOR);
  std::snprintf(text, sizeof(text), "%u%%",
                static_cast<unsigned int>(data.percentage));
  setLabelText(percentageLabel_, text);
  lv_obj_set_style_text_color(percentageLabel_, indicator, LV_PART_MAIN);

  char syncText[15]{};
  formatLocalDateTime(data.lastSyncUtc, false, syncText, sizeof(syncText));
  std::snprintf(text, sizeof(text), "SYNC: %s", syncText);
  setLabelText(syncLabel_, text);

  const char* const status =
      data.onScale ? app::trackingStatusText(data.trackingStatus) : "LIDO";
  setLabelText(statusLabel_, status);
  const bool warning =
      data.trackingStatus == app::TrackingViewStatus::Paused;
  const bool danger =
      data.trackingStatus == app::TrackingViewStatus::ChangePending;
  lv_obj_set_style_text_color(
      statusLabel_, danger ? Theme::danger()
                           : (warning ? Theme::warning()
                                      : (data.onScale ? Theme::good()
                                                      : Theme::warning())),
      LV_PART_MAIN);
}

void KegCard::handleDetails(lv_event_t* const event) {
  auto* const self = static_cast<KegCard*>(lv_event_get_user_data(event));
  if (self != nullptr && self->detailsHandler_ != nullptr) {
    self->detailsHandler_(self->index_, self->detailsContext_);
  }
}

}  // namespace keezer::ui::widgets
