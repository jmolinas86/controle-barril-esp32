#include "ui/screens/SettingsScreen.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "ui/Theme.h"
#include "ui/widgets/Card.h"

namespace keezer::ui::screens {
namespace {

constexpr std::array<const char*, 6U> kItems{
    "BARRIS", "BALANCA", "OTA", "REDE", "DISPLAY", "SISTEMA"};

lv_color_t scaleStatusColor(const models::ScaleLinkStatus status) {
  switch (status) {
    case models::ScaleLinkStatus::Online:
      return Theme::good();
    case models::ScaleLinkStatus::Stale:
      return Theme::warning();
    case models::ScaleLinkStatus::Offline:
      return Theme::danger();
  }
  return Theme::muted();
}

lv_color_t networkStatusColor(const app::NetworkViewData& network) {
  if (network.connected) return Theme::good();
  if (network.status == models::NetworkStatus::ConfigurationError) {
    return Theme::danger();
  }
  return Theme::warning();
}

void setLabelText(lv_obj_t* const label, const char* const text) {
  if (label != nullptr && text != nullptr &&
      std::strcmp(lv_label_get_text(label), text) != 0) {
    lv_label_set_text(label, text);
  }
}

}  // namespace

void SettingsScreen::create(lv_obj_t* const parent,
                            const app::ScaleViewData& scale,
                            const app::NetworkViewData& network,
                            const app::HistoryStorageViewData& history,
                            const ActionHandler archivesHandler,
                            const ActionHandler otaHandler,
                            const ActionHandler networkHandler,
                            const ActionHandler displayHandler,
                            void* const context) {
  scaleStatusLabel_ = nullptr;
  scaleWeightLabel_ = nullptr;
  scaleNfcLabel_ = nullptr;
  networkStatusLabel_ = nullptr;
  networkSignalLabel_ = nullptr;
  otaStatusLabel_ = nullptr;
  storageStatusLabel_ = nullptr;
  storagePendingLabel_ = nullptr;
  archivesHandler_ = archivesHandler;
  otaHandler_ = otaHandler;
  networkHandler_ = networkHandler;
  displayHandler_ = displayHandler;
  context_ = context;
  widgets::Card::addLabel(parent, "CONFIGURACOES", 8, 8, Theme::primary(),
                          &lv_font_montserrat_14);
  for (std::size_t index = 0U; index < kItems.size(); ++index) {
    const std::int32_t column = static_cast<std::int32_t>(index % 2U);
    const std::int32_t row = static_cast<std::int32_t>(index / 2U);
    lv_obj_t* const card = widgets::Card::create(
        parent, 6 + (column * 116), 36 + (row * 60), 110, 52);
    if (index == 0U) {
      widgets::Card::addLabel(card, "BARRIS", 8, 17, Theme::text(),
                              &lv_font_montserrat_12);
      widgets::Card::addLabel(card, LV_SYMBOL_RIGHT, 89, 17,
                              Theme::primary(), &lv_font_montserrat_12);
      lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_event_cb(card, handleArchives, LV_EVENT_CLICKED, this);
      continue;
    }
    if (index == 1U) {
      widgets::Card::addLabel(card, "BALANCA", 6, 5, Theme::primary(),
                              &lv_font_montserrat_10);
      scaleStatusLabel_ = widgets::Card::addLabel(
          card, "OFFLINE", 6, 19, Theme::danger(),
          &lv_font_montserrat_10);
      scaleWeightLabel_ = widgets::Card::addLabel(
          card, "--.--kg", 6, 34, Theme::text(), &lv_font_montserrat_10);
      scaleNfcLabel_ = widgets::Card::addLabel(
          card, "NFC --", 65, 36, Theme::muted(),
          &lv_font_montserrat_8);
      continue;
    }
    if (index == 2U) {
      widgets::Card::addLabel(card, "OTA", 6, 5, Theme::primary(),
                              &lv_font_montserrat_10);
      otaStatusLabel_ = widgets::Card::addLabel(
          card, "SEM REDE", 6, 25, Theme::warning(),
          &lv_font_montserrat_10);
      widgets::Card::addLabel(card, LV_SYMBOL_RIGHT, 91, 5, Theme::primary(),
                              &lv_font_montserrat_10);
      lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_event_cb(card, handleOta, LV_EVENT_CLICKED, this);
      continue;
    }
    if (index == 3U) {
      widgets::Card::addLabel(card, "REDE", 6, 5, Theme::primary(),
                              &lv_font_montserrat_10);
      networkStatusLabel_ = widgets::Card::addLabel(
          card, "CONECTANDO", 6, 23, Theme::warning(),
          &lv_font_montserrat_8);
      networkSignalLabel_ = widgets::Card::addLabel(
          card, "--- dBm", 6, 38, Theme::muted(),
          &lv_font_montserrat_8);
      widgets::Card::addLabel(card, LV_SYMBOL_RIGHT, 91, 5, Theme::primary(),
                              &lv_font_montserrat_10);
      lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_event_cb(card, handleNetwork, LV_EVENT_CLICKED, this);
      continue;
    }
    if (index == 5U) {
      widgets::Card::addLabel(card, "SISTEMA", 6, 5, Theme::primary(),
                              &lv_font_montserrat_10);
      storageStatusLabel_ = widgets::Card::addLabel(
          card, "SD --", 6, 23, Theme::warning(), &lv_font_montserrat_8);
      storagePendingLabel_ = widgets::Card::addLabel(
          card, "FILA 0", 6, 38, Theme::muted(), &lv_font_montserrat_8);
      continue;
    }
    if (index == 4U) {
      widgets::Card::addLabel(card, "DISPLAY", 8, 17, Theme::text(),
                              &lv_font_montserrat_12);
      widgets::Card::addLabel(card, LV_SYMBOL_RIGHT, 89, 17,
                              Theme::primary(), &lv_font_montserrat_12);
      lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_event_cb(card, handleDisplay, LV_EVENT_CLICKED, this);
      continue;
    }
    widgets::Card::addLabel(card, kItems[index], 8, 17, Theme::text(),
                            &lv_font_montserrat_12);
    widgets::Card::addLabel(card, LV_SYMBOL_RIGHT, 89, 17, Theme::primary(),
                            &lv_font_montserrat_12);
  }
  update(scale, network, history);
}

void SettingsScreen::update(const app::ScaleViewData& scale,
                            const app::NetworkViewData& network,
                            const app::HistoryStorageViewData& history) {
  setLabelText(scaleStatusLabel_, models::scaleLinkStatusName(scale.linkStatus));
  if (scaleStatusLabel_ != nullptr) {
    lv_obj_set_style_text_color(scaleStatusLabel_,
                                scaleStatusColor(scale.linkStatus),
                                LV_PART_MAIN);
  }

  char weight[20]{};
  if (scale.hasWeight) {
    const long grams = static_cast<long>(scale.weightGrams);
    std::snprintf(weight, sizeof(weight), "%ld.%02ldkg", grams / 1'000L,
                  (grams % 1'000L) / 10L);
  } else {
    std::snprintf(weight, sizeof(weight), "%s", "--.--kg");
  }
  setLabelText(scaleWeightLabel_, weight);
  setLabelText(scaleNfcLabel_, scale.hasNfcUid ? "NFC OK" : "NFC --");
  if (scaleNfcLabel_ != nullptr) {
    lv_obj_set_style_text_color(scaleNfcLabel_,
                                scale.hasNfcUid ? Theme::good()
                                                : Theme::muted(),
                                LV_PART_MAIN);
  }

  setLabelText(networkStatusLabel_, models::networkStatusName(network.status));
  if (networkStatusLabel_ != nullptr) {
    lv_obj_set_style_text_color(networkStatusLabel_,
                                networkStatusColor(network), LV_PART_MAIN);
  }
  char signal[16]{};
  if (network.hasRssi) {
    std::snprintf(signal, sizeof(signal), "%d dBm",
                  static_cast<int>(network.rssiDbm));
  } else {
    std::snprintf(signal, sizeof(signal), "%s", "--- dBm");
  }
  setLabelText(networkSignalLabel_, signal);
  setLabelText(otaStatusLabel_, network.connected ? "DISPONIVEL" : "SEM REDE");
  if (otaStatusLabel_ != nullptr) {
    lv_obj_set_style_text_color(otaStatusLabel_,
                                network.connected ? Theme::good()
                                                  : Theme::warning(),
                                LV_PART_MAIN);
  }

  const char* storageText = "SD ERRO";
  lv_color_t storageColor = Theme::danger();
  switch (history.status) {
    case app::HistoryStorageViewStatus::Ready:
      storageText = "SD OK";
      storageColor = Theme::good();
      break;
    case app::HistoryStorageViewStatus::Missing:
      storageText = "SD AUSENTE";
      storageColor = Theme::warning();
      break;
    case app::HistoryStorageViewStatus::Full:
      storageText = "SD CHEIO";
      storageColor = Theme::danger();
      break;
    case app::HistoryStorageViewStatus::Error:
      break;
  }
  setLabelText(storageStatusLabel_, storageText);
  if (storageStatusLabel_ != nullptr) {
    lv_obj_set_style_text_color(storageStatusLabel_, storageColor,
                                LV_PART_MAIN);
  }
  char pending[20]{};
  std::snprintf(pending, sizeof(pending), "FILA %u",
                static_cast<unsigned int>(history.pendingCount));
  setLabelText(storagePendingLabel_, pending);
  if (storagePendingLabel_ != nullptr) {
    lv_obj_set_style_text_color(
        storagePendingLabel_,
        history.pendingCount == 0U ? Theme::muted() : Theme::warning(),
        LV_PART_MAIN);
  }
}

void SettingsScreen::handleArchives(lv_event_t* const event) {
  auto* self = static_cast<SettingsScreen*>(lv_event_get_user_data(event));
  if (self != nullptr && self->archivesHandler_ != nullptr) {
    self->archivesHandler_(self->context_);
  }
}

void SettingsScreen::handleOta(lv_event_t* const event) {
  auto* self = static_cast<SettingsScreen*>(lv_event_get_user_data(event));
  if (self != nullptr && self->otaHandler_ != nullptr) {
    self->otaHandler_(self->context_);
  }
}

void SettingsScreen::handleNetwork(lv_event_t* const event) {
  auto* const self =
      static_cast<SettingsScreen*>(lv_event_get_user_data(event));
  if (self != nullptr && self->networkHandler_ != nullptr) {
    self->networkHandler_(self->context_);
  }
}

void SettingsScreen::handleDisplay(lv_event_t* const event) {
  auto* self = static_cast<SettingsScreen*>(lv_event_get_user_data(event));
  if (self != nullptr && self->displayHandler_ != nullptr) {
    self->displayHandler_(self->context_);
  }
}

}  // namespace keezer::ui::screens
