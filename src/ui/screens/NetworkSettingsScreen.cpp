#include "ui/screens/NetworkSettingsScreen.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "ui/Layout.h"
#include "ui/Theme.h"
#include "ui/widgets/ActionButton.h"
#include "ui/widgets/Card.h"
#include "ui/widgets/ScrollContainer.h"

namespace keezer::ui::screens {
namespace {

constexpr char kPortCharacters[] = "0123456789";

const char* safeText(const char* const text) {
  return text == nullptr ? "" : text;
}

void setLabelText(lv_obj_t* const label, const char* const text) {
  if (label != nullptr && text != nullptr &&
      std::strcmp(lv_label_get_text(label), text) != 0) {
    lv_label_set_text(label, text);
  }
}

lv_color_t statusColor(const app::NetworkViewData& state) {
  if (state.connected) return Theme::good();
  if (state.status == models::NetworkStatus::ConfigurationError) {
    return Theme::danger();
  }
  return Theme::warning();
}

lv_obj_t* createButtonRow(lv_obj_t* const parent) {
  lv_obj_t* const row = lv_obj_create(parent);
  lv_obj_set_size(row, layout::kCardWidth, 38);
  lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_column(row, 6, LV_PART_MAIN);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
  return row;
}

}  // namespace

void NetworkSettingsScreen::create(
    lv_obj_t* const parent, const app::NetworkViewData& state,
    const ActionHandler saveHandler, const ActionHandler cancelHandler,
    void* const context) {
  saveHandler_ = saveHandler;
  cancelHandler_ = cancelHandler;
  context_ = context;
  scroll_ = widgets::ScrollContainer::create(parent);
  lv_obj_set_style_pad_row(scroll_, 3, LV_PART_MAIN);

  lv_obj_t* const summary =
      widgets::Card::create(scroll_, 0, 0, layout::kCardWidth, 66);
  widgets::Card::addLabel(summary, "STATUS REDE", 7, 6, Theme::primary(),
                          &lv_font_montserrat_10);
  connectionLabel_ = widgets::Card::addLabel(
      summary, "CONECTANDO", 82, 6, Theme::warning(),
      &lv_font_montserrat_10);
  ipLabel_ = widgets::Card::addLabel(summary, "IP: ---.---.---.---", 7, 26,
                                     Theme::text(), &lv_font_montserrat_10);
  rssiLabel_ = widgets::Card::addLabel(summary, "SINAL: ---", 7, 45,
                                       Theme::muted(),
                                       &lv_font_montserrat_10);

  resultLabel_ = lv_label_create(scroll_);
  lv_obj_set_width(resultLabel_, layout::kCardWidth);
  lv_label_set_text(resultLabel_, "ALTERAR A REDE REINICIA A CONEXAO");
  lv_obj_set_style_text_align(resultLabel_, LV_TEXT_ALIGN_CENTER,
                              LV_PART_MAIN);
  lv_obj_set_style_text_color(resultLabel_, Theme::warning(), LV_PART_MAIN);
  lv_obj_set_style_text_font(resultLabel_, &lv_font_montserrat_8,
                             LV_PART_MAIN);

  ssidField_ = createField(scroll_, "NOME DA REDE (SSID)",
                           state.ssid.data(),
                           models::kWifiSsidBytes - 1U, nullptr, this);
  passwordField_ = createField(scroll_, "NOVA SENHA (VAZIA = MANTER)", "",
                               models::kWifiPasswordBytes - 1U, nullptr, this);
  lv_textarea_set_password_mode(passwordField_, true);
  hostnameField_ = createField(scroll_, "NOME DO CONTROLADOR",
                               state.hostname.data(),
                               models::kNetworkHostnameBytes - 1U, nullptr,
                               this);
  scaleHostField_ = createField(scroll_, "IP OU HOST DA BALANCA",
                                state.scaleHost.data(),
                                models::kNetworkAddressBytes - 1U, nullptr,
                                this);
  char port[8]{};
  std::snprintf(port, sizeof(port), "%u",
                static_cast<unsigned int>(state.scalePort));
  scalePortField_ = createField(scroll_, "PORTA DA BALANCA", port, 5U,
                                kPortCharacters, this);

  lv_obj_t* const buttons = createButtonRow(scroll_);
  widgets::ActionButton::create(buttons, "CANCELAR", 108, 36, handleCancel,
                                this, false);
  widgets::ActionButton::create(buttons, LV_SYMBOL_SAVE " SALVAR", 108, 36,
                                handleSave, this, true);

  keyboard_ = lv_keyboard_create(parent);
  lv_obj_set_size(keyboard_, layout::kScreenWidth, 118);
  lv_obj_align(keyboard_, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_color(keyboard_, Theme::surfaceRaised(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(keyboard_, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_color(keyboard_, Theme::primary(), LV_PART_MAIN);
  lv_obj_set_style_border_width(keyboard_, 1, LV_PART_MAIN);
  lv_obj_set_style_radius(keyboard_, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(keyboard_, 2, LV_PART_MAIN);
  lv_obj_set_style_pad_row(keyboard_, 2, LV_PART_MAIN);
  lv_obj_set_style_pad_column(keyboard_, 2, LV_PART_MAIN);
  lv_obj_set_style_bg_color(keyboard_, Theme::surface(), LV_PART_ITEMS);
  lv_obj_set_style_bg_opa(keyboard_, LV_OPA_COVER, LV_PART_ITEMS);
  lv_obj_set_style_text_color(keyboard_, Theme::text(), LV_PART_ITEMS);
  lv_obj_set_style_text_font(keyboard_, &lv_font_montserrat_12, LV_PART_ITEMS);
  lv_obj_set_style_border_color(keyboard_, Theme::border(), LV_PART_ITEMS);
  lv_obj_set_style_border_width(keyboard_, 1, LV_PART_ITEMS);
  lv_obj_set_style_radius(keyboard_, 3, LV_PART_ITEMS);
  lv_obj_add_event_cb(keyboard_, handleKeyboard, LV_EVENT_READY, this);
  lv_obj_add_event_cb(keyboard_, handleKeyboard, LV_EVENT_CANCEL, this);
  lv_obj_add_flag(keyboard_, LV_OBJ_FLAG_HIDDEN);
  update(state);
  // The resistive pointer does not always emit the first focus transition on
  // every LVGL build. Open explicitly, as done by the KEG editor.
  openKeyboard(ssidField_);
}

void NetworkSettingsScreen::update(const app::NetworkViewData& state) {
  setLabelText(connectionLabel_, models::networkStatusName(state.status));
  if (connectionLabel_ != nullptr) {
    lv_obj_set_style_text_color(connectionLabel_, statusColor(state),
                                LV_PART_MAIN);
  }
  char text[48]{};
  std::snprintf(text, sizeof(text), "IP: %s",
                state.connected && state.ipAddress[0] != '\0'
                    ? state.ipAddress.data()
                    : "---.---.---.---");
  setLabelText(ipLabel_, text);
  if (state.hasRssi) {
    std::snprintf(text, sizeof(text), "SINAL: %d dBm",
                  static_cast<int>(state.rssiDbm));
  } else if (state.retryInMs > 0U) {
    std::snprintf(text, sizeof(text), "NOVA TENTATIVA: %lu s",
                  static_cast<unsigned long>((state.retryInMs + 999U) /
                                             1'000U));
  } else {
    std::snprintf(text, sizeof(text), "%s", "SINAL: ---");
  }
  setLabelText(rssiLabel_, text);
}

app::NetworkEditRequest NetworkSettingsScreen::request() const {
  const unsigned long parsedPort =
      std::strtoul(scalePortField_ == nullptr
                       ? "0"
                       : lv_textarea_get_text(scalePortField_),
                   nullptr, 10);
  return {
      ssidField_ == nullptr ? "" : lv_textarea_get_text(ssidField_),
      passwordField_ == nullptr ? "" : lv_textarea_get_text(passwordField_),
      hostnameField_ == nullptr ? ""
                                : lv_textarea_get_text(hostnameField_),
      scaleHostField_ == nullptr ? ""
                                 : lv_textarea_get_text(scaleHostField_),
      parsedPort <= 65'535UL ? static_cast<std::uint16_t>(parsedPort)
                             : static_cast<std::uint16_t>(0U),
  };
}

void NetworkSettingsScreen::showResult(const app::NetworkSaveResult result) {
  if (resultLabel_ == nullptr) return;
  const char* text = "CONFIGURACAO SALVA - RECONECTANDO";
  lv_color_t color = Theme::good();
  if (result == app::NetworkSaveResult::InvalidData) {
    text = "CONFIRA SSID, SENHA, HOST E PORTA";
    color = Theme::danger();
  } else if (result == app::NetworkSaveResult::StorageError) {
    text = "ERRO AO SALVAR NA MEMORIA";
    color = Theme::danger();
  } else if (result == app::NetworkSaveResult::ServiceError) {
    text = "ERRO AO REINICIAR A REDE";
    color = Theme::danger();
  }
  lv_label_set_text(resultLabel_, text);
  lv_obj_set_style_text_color(resultLabel_, color, LV_PART_MAIN);
  lv_obj_scroll_to_view(resultLabel_, LV_ANIM_ON);
}

lv_obj_t* NetworkSettingsScreen::createField(
    lv_obj_t* const parent, const char* const label, const char* const value,
    const std::uint32_t maximumLength, const char* const acceptedCharacters,
    NetworkSettingsScreen* const owner) {
  lv_obj_t* const row = lv_obj_create(parent);
  lv_obj_set_size(row, layout::kCardWidth, 48);
  lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t* const caption = lv_label_create(row);
  lv_label_set_text(caption, label);
  lv_obj_set_pos(caption, 4, 0);
  lv_obj_set_style_text_color(caption, Theme::primary(), LV_PART_MAIN);
  lv_obj_set_style_text_font(caption, &lv_font_montserrat_10, LV_PART_MAIN);
  lv_obj_t* const field = lv_textarea_create(row);
  lv_obj_set_pos(field, 0, 13);
  lv_obj_set_size(field, layout::kCardWidth, 34);
  lv_textarea_set_one_line(field, true);
  lv_textarea_set_max_length(field, maximumLength);
  lv_textarea_set_text(field, safeText(value));
  if (acceptedCharacters != nullptr) {
    lv_textarea_set_accepted_chars(field, acceptedCharacters);
  }
  lv_obj_set_style_bg_color(field, Theme::surfaceRaised(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(field, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_color(field, Theme::border(), LV_PART_MAIN);
  lv_obj_set_style_border_width(field, 1, LV_PART_MAIN);
  lv_obj_set_style_radius(field, 5, LV_PART_MAIN);
  lv_obj_set_style_pad_left(field, 7, LV_PART_MAIN);
  lv_obj_set_style_pad_top(field, 7, LV_PART_MAIN);
  lv_obj_set_style_text_color(field, Theme::text(), LV_PART_MAIN);
  lv_obj_set_style_text_font(field, &lv_font_montserrat_12, LV_PART_MAIN);
  lv_obj_add_event_cb(field, handleFieldFocused, LV_EVENT_FOCUSED, owner);
  lv_obj_add_event_cb(field, handleFieldFocused, LV_EVENT_CLICKED, owner);
  return field;
}

void NetworkSettingsScreen::handleFieldFocused(lv_event_t* const event) {
  auto* const self =
      static_cast<NetworkSettingsScreen*>(lv_event_get_user_data(event));
  if (self != nullptr) {
    self->openKeyboard(static_cast<lv_obj_t*>(lv_event_get_target(event)));
  }
}

void NetworkSettingsScreen::handleKeyboard(lv_event_t* const event) {
  auto* const self =
      static_cast<NetworkSettingsScreen*>(lv_event_get_user_data(event));
  if (self != nullptr) self->closeKeyboard();
}

void NetworkSettingsScreen::handleSave(lv_event_t* const event) {
  auto* const self =
      static_cast<NetworkSettingsScreen*>(lv_event_get_user_data(event));
  if (self != nullptr && self->saveHandler_ != nullptr) {
    self->closeKeyboard();
    self->saveHandler_(self->context_);
  }
}

void NetworkSettingsScreen::handleCancel(lv_event_t* const event) {
  auto* const self =
      static_cast<NetworkSettingsScreen*>(lv_event_get_user_data(event));
  if (self != nullptr && self->cancelHandler_ != nullptr) {
    self->closeKeyboard();
    self->cancelHandler_(self->context_);
  }
}

void NetworkSettingsScreen::openKeyboard(lv_obj_t* const field) {
  if (keyboard_ == nullptr || field == nullptr) return;
  lv_keyboard_set_mode(keyboard_,
                       field == scalePortField_ ? LV_KEYBOARD_MODE_NUMBER
                                                : LV_KEYBOARD_MODE_TEXT_LOWER);
  lv_keyboard_set_textarea(keyboard_, field);
  lv_obj_clear_flag(keyboard_, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_height(scroll_, 132);
  lv_obj_move_foreground(keyboard_);
  lv_obj_update_layout(scroll_);
  lv_obj_scroll_to_view_recursive(field, LV_ANIM_ON);
}

void NetworkSettingsScreen::closeKeyboard() {
  if (keyboard_ == nullptr) return;
  lv_keyboard_set_textarea(keyboard_, nullptr);
  lv_obj_add_flag(keyboard_, LV_OBJ_FLAG_HIDDEN);
  if (scroll_ != nullptr) lv_obj_set_height(scroll_, layout::kContentHeight);
}

}  // namespace keezer::ui::screens
