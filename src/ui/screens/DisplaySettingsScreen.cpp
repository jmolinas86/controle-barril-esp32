#include "ui/screens/DisplaySettingsScreen.h"

#include <array>
#include <cstdio>

#include "ui/Theme.h"
#include "ui/widgets/ActionButton.h"
#include "ui/widgets/Card.h"

namespace keezer::ui::screens {
namespace {

constexpr std::array<std::uint16_t, 5U> kTimeouts{0U, 30U, 60U, 120U, 300U};

lv_obj_t* valueButton(lv_obj_t* parent, const char* text, std::int32_t x,
                      std::int32_t y, lv_event_cb_t callback, void* context) {
  lv_obj_t* button = widgets::ActionButton::create(
      parent, text, 42, 38, callback, context, false);
  lv_obj_set_pos(button, x, y);
  return button;
}

}  // namespace

void DisplaySettingsScreen::create(lv_obj_t* const parent,
                                   const models::DisplaySettings& settings,
                                   const SaveHandler saveHandler,
                                   void* const context) {
  brightness_ = settings.brightnessPercent;
  timeoutIndex_ = 0U;
  for (std::size_t index = 0U; index < kTimeouts.size(); ++index) {
    if (kTimeouts[index] == settings.timeoutSeconds) {
      timeoutIndex_ = static_cast<std::uint8_t>(index);
      break;
    }
  }
  saveHandler_ = saveHandler;
  context_ = context;

  lv_obj_t* brightnessCard = widgets::Card::create(parent, 6, 8, 228, 72);
  widgets::Card::addLabel(brightnessCard, "BRILHO", 9, 7, Theme::primary(),
                          &lv_font_montserrat_12);
  valueButton(brightnessCard, "-", 9, 27, handleBrightnessDown, this);
  valueButton(brightnessCard, "+", 176, 27, handleBrightnessUp, this);
  brightnessLabel_ = widgets::Card::addLabel(
      brightnessCard, "70%", 52, 34, Theme::text(), &lv_font_montserrat_16);
  lv_obj_set_width(brightnessLabel_, 124);
  lv_obj_set_style_text_align(brightnessLabel_, LV_TEXT_ALIGN_CENTER,
                              LV_PART_MAIN);

  lv_obj_t* timeoutCard = widgets::Card::create(parent, 6, 86, 228, 72);
  widgets::Card::addLabel(timeoutCard, "APAGAR APOS", 9, 7,
                          Theme::primary(), &lv_font_montserrat_12);
  valueButton(timeoutCard, "-", 9, 27, handleTimeoutDown, this);
  valueButton(timeoutCard, "+", 176, 27, handleTimeoutUp, this);
  timeoutLabel_ = widgets::Card::addLabel(
      timeoutCard, "1 MIN", 52, 34, Theme::text(), &lv_font_montserrat_14);
  lv_obj_set_width(timeoutLabel_, 124);
  lv_obj_set_style_text_align(timeoutLabel_, LV_TEXT_ALIGN_CENTER,
                              LV_PART_MAIN);

  lv_obj_t* save = widgets::ActionButton::create(
      parent, LV_SYMBOL_SAVE "  SALVAR DISPLAY", 228, 38, handleSave, this,
      true);
  lv_obj_set_pos(save, 6, 165);
  resultLabel_ = widgets::Card::addLabel(
      parent, "O PRIMEIRO TOQUE APENAS ACORDA", 6, 210, Theme::muted(),
      &lv_font_montserrat_10);
  lv_obj_set_width(resultLabel_, 228);
  lv_obj_set_style_text_align(resultLabel_, LV_TEXT_ALIGN_CENTER,
                              LV_PART_MAIN);
  refreshValues();
}

void DisplaySettingsScreen::showResult(const app::DisplaySaveResult result) {
  if (resultLabel_ == nullptr) return;
  if (result == app::DisplaySaveResult::Ok) {
    lv_label_set_text(resultLabel_, "CONFIGURACAO SALVA");
    lv_obj_set_style_text_color(resultLabel_, Theme::good(), LV_PART_MAIN);
  } else {
    lv_label_set_text(resultLabel_, "ERRO AO SALVAR");
    lv_obj_set_style_text_color(resultLabel_, Theme::danger(), LV_PART_MAIN);
  }
}

void DisplaySettingsScreen::refreshValues() {
  char brightness[12]{};
  std::snprintf(brightness, sizeof(brightness), "%u%%",
                static_cast<unsigned int>(brightness_));
  lv_label_set_text(brightnessLabel_, brightness);
  const std::uint16_t seconds = kTimeouts[timeoutIndex_];
  const char* text = "SEMPRE LIGADO";
  if (seconds == 30U) text = "30 S";
  else if (seconds == 60U) text = "1 MIN";
  else if (seconds == 120U) text = "2 MIN";
  else if (seconds == 300U) text = "5 MIN";
  lv_label_set_text(timeoutLabel_, text);
}

void DisplaySettingsScreen::handleBrightnessDown(lv_event_t* event) {
  auto* self = static_cast<DisplaySettingsScreen*>(lv_event_get_user_data(event));
  if (self == nullptr) return;
  self->brightness_ = self->brightness_ <= 20U ? 10U : self->brightness_ - 10U;
  self->refreshValues();
}

void DisplaySettingsScreen::handleBrightnessUp(lv_event_t* event) {
  auto* self = static_cast<DisplaySettingsScreen*>(lv_event_get_user_data(event));
  if (self == nullptr) return;
  self->brightness_ = self->brightness_ >= 90U ? 100U : self->brightness_ + 10U;
  self->refreshValues();
}

void DisplaySettingsScreen::handleTimeoutDown(lv_event_t* event) {
  auto* self = static_cast<DisplaySettingsScreen*>(lv_event_get_user_data(event));
  if (self == nullptr) return;
  if (self->timeoutIndex_ > 0U) --self->timeoutIndex_;
  self->refreshValues();
}

void DisplaySettingsScreen::handleTimeoutUp(lv_event_t* event) {
  auto* self = static_cast<DisplaySettingsScreen*>(lv_event_get_user_data(event));
  if (self == nullptr) return;
  if (self->timeoutIndex_ + 1U < kTimeouts.size()) ++self->timeoutIndex_;
  self->refreshValues();
}

void DisplaySettingsScreen::handleSave(lv_event_t* event) {
  auto* self = static_cast<DisplaySettingsScreen*>(lv_event_get_user_data(event));
  if (self == nullptr || self->saveHandler_ == nullptr) return;
  self->showResult(self->saveHandler_(self->brightness_,
                                     kTimeouts[self->timeoutIndex_],
                                     self->context_));
}

}  // namespace keezer::ui::screens
