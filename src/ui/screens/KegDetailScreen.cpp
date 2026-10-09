#include "ui/screens/KegDetailScreen.h"

#include <array>
#include <cstdio>
#include <cstring>

#include "ui/Layout.h"
#include "ui/Theme.h"
#include "ui/TimeFormat.h"
#include "ui/assets/MiniBitmaps.h"
#include "ui/widgets/Card.h"
#include "ui/widgets/ActionButton.h"
#include "ui/widgets/InfoTile.h"
#include "ui/widgets/KegIcon.h"
#include "ui/widgets/ProgressBar.h"
#include "ui/widgets/ScrollContainer.h"

namespace keezer::ui::screens {
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

lv_obj_t* createTileRow(lv_obj_t* const parent) {
  lv_obj_t* const row = lv_obj_create(parent);
  lv_obj_set_size(row, layout::kCardWidth, 44);
  lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_column(row, 6, LV_PART_MAIN);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
  return row;
}

lv_obj_t* createActionRow(lv_obj_t* const parent) {
  lv_obj_t* const row = lv_obj_create(parent);
  lv_obj_set_size(row, layout::kCardWidth, 36);
  lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_column(row, 6, LV_PART_MAIN);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
  return row;
}

void setLabelText(lv_obj_t* const label, const char* const text) {
  if (label != nullptr && text != nullptr &&
      std::strcmp(lv_label_get_text(label), text) != 0) {
    lv_label_set_text(label, text);
  }
}

lv_color_t statusColor(const app::KegViewData& data) {
  if (data.trackingStatus == app::TrackingViewStatus::ChangePending) {
    return Theme::danger();
  }
  if (data.trackingStatus == app::TrackingViewStatus::Paused) {
    return Theme::warning();
  }
  switch (data.status) {
    case models::KegStatus::ActiveOnScale:
    case models::KegStatus::Available:
      return Theme::good();
    case models::KegStatus::Stored:
    case models::KegStatus::Empty:
    case models::KegStatus::Cleaning:
      return Theme::warning();
    case models::KegStatus::Finished:
    case models::KegStatus::Archived:
      return Theme::danger();
  }
  return Theme::muted();
}

const char* trackingButtonText(const app::KegViewData& data) {
  if (!data.onScale) return "PESAR";
  switch (data.trackingStatus) {
    case app::TrackingViewStatus::Acquiring:
      return "ESTABILIZANDO";
    case app::TrackingViewStatus::Monitoring:
      return "MONITORANDO";
    case app::TrackingViewStatus::Paused:
      return "PAUSADO - PESAR";
    case app::TrackingViewStatus::ChangePending:
      return "CONFIRMAR KEG";
    case app::TrackingViewStatus::Idle:
      return "PESAR";
  }
  return "PESAR";
}

}  // namespace

void KegDetailScreen::create(lv_obj_t* const parent,
                             const app::KegViewData& data,
                             const app::ManualWeighingViewData& weighing,
                             const ActionHandler weighHandler,
                             const ActionHandler removeHandler,
                             const ActionHandler editHandler,
                             const ActionHandler nfcHandler,
                             const ActionHandler finishHandler,
                             const ActionHandler archiveHandler,
                             void* const context) {
  weighHandler_ = weighHandler;
  removeHandler_ = removeHandler;
  editHandler_ = editHandler;
  nfcHandler_ = nfcHandler;
  finishHandler_ = finishHandler;
  archiveHandler_ = archiveHandler;
  actionContext_ = context;
  lv_obj_t* const scroll = widgets::ScrollContainer::create(parent);
  lv_obj_set_style_pad_row(scroll, 2, LV_PART_MAIN);
  const lv_color_t indicator = levelColor(data);

  lv_obj_t* const hero =
      widgets::Card::create(scroll, 0, 0, layout::kCardWidth, 96);
  lv_obj_set_style_border_color(hero, Theme::primary(), LV_PART_MAIN);
  lv_obj_t* const kegIcon = widgets::KegIcon::create(hero, 56, 76);
  lv_obj_set_pos(kegIcon, 13, 10);

  char value[40]{};
  std::snprintf(value, sizeof(value), "%.1f L",
                static_cast<double>(data.volumeL));
  heroVolume_ = widgets::Card::addLabel(hero, value, 78, 14, Theme::text(),
                                        &lv_font_montserrat_24);
  std::snprintf(value, sizeof(value), "%u%% DA CAPACIDADE",
                static_cast<unsigned int>(data.percentage));
  heroCapacity_ = widgets::Card::addLabel(
      hero, value, 78, 49, indicator, &lv_font_montserrat_12);
  heroProgress_ = widgets::ProgressBar::create(
      hero, 132, 8, data.percentage, indicator);
  lv_obj_set_pos(heroProgress_, 78, 74);

  lv_obj_t* row = createActionRow(scroll);
  weighButton_ = widgets::ActionButton::create(
      row, "PESAR", 108, 34, handleWeigh, this, true);
  weighButtonLabel_ = lv_obj_get_child(weighButton_, 0);
  removeButton_ = widgets::ActionButton::create(
      row, "RETIRAR", 108, 34, handleRemove, this, false);
  lv_obj_set_style_text_color(removeButton_, Theme::danger(), LV_PART_MAIN);

  std::array<char, 40U> tare{};
  std::array<char, 40U> density{};
  std::array<char, 40U> volume{};
  std::array<char, 40U> weight{};
  std::array<char, 40U> capacity{};
  std::array<char, 18U> lastSync{};
  std::snprintf(tare.data(), tare.size(), "%.2f kg",
                static_cast<double>(data.tareKg));
  std::snprintf(density.data(), density.size(), "%.3f kg/L",
                static_cast<double>(data.densityKgPerL));
  std::snprintf(volume.data(), volume.size(), "%.1f L",
                static_cast<double>(data.volumeL));
  std::snprintf(weight.data(), weight.size(), "%.2f kg",
                static_cast<double>(data.weightKg));
  std::snprintf(capacity.data(), capacity.size(), "%.1f L",
                static_cast<double>(data.capacityL));
  formatLocalDateTime(data.lastSyncUtc, true, lastSync.data(),
                      lastSync.size());

  row = createTileRow(scroll);
  widgets::InfoTile::create(row, "TARA", tare.data(), 111, 44, Theme::text(),
                            &assets::kMeasurementTare, &tareValue_);
  widgets::InfoTile::create(row, "DENSIDADE", density.data(), 111, 44,
                            Theme::text(), &assets::kMeasurementDensity,
                            &densityValue_);

  row = createTileRow(scroll);
  widgets::InfoTile::create(row, "VOLUME ATUAL", volume.data(), 111, 44,
                            Theme::text(), &assets::kMeasurementVolume,
                            &volumeValue_);
  widgets::InfoTile::create(row, "PESO", weight.data(), 111, 44,
                            Theme::text(), &assets::kMeasurementWeight,
                            &weightValue_);

  row = createTileRow(scroll);
  widgets::InfoTile::create(row, "ULTIMO SYNC", lastSync.data(), 111, 44,
                            Theme::text(), nullptr, &syncValue_);
  widgets::InfoTile::create(row, "CAPACIDADE", capacity.data(), 111, 44,
                            Theme::text(), nullptr, &capacityValue_);

  row = createTileRow(scroll);
  widgets::InfoTile::create(row, "NFC UID", data.nfcUid, 111, 44,
                            Theme::text(), nullptr, &nfcValue_);
  widgets::InfoTile::create(row, "STATUS", app::kegStatusText(data.status),
                            111, 44, statusColor(data), nullptr, &statusValue_);

  chart_.create(scroll, data);

  row = createActionRow(scroll);
  widgets::ActionButton::create(row, LV_SYMBOL_EDIT " EDITAR", 108, 34,
                                handleEdit, this, true);
  widgets::ActionButton::create(row, "ASSOC. NFC", 108, 34, handleNfc, this,
                                true);
  row = createActionRow(scroll);
  widgets::ActionButton::create(row, "FINALIZAR", 108, 34, handleFinish,
                                this, true);
  lv_obj_t* const archive = widgets::ActionButton::create(
      row, "ARQUIVAR", 108, 34, handleArchive, this, true);
  lv_obj_set_style_text_color(archive, Theme::danger(), LV_PART_MAIN);
  update(data, weighing);
}

void KegDetailScreen::update(
    const app::KegViewData& data,
    const app::ManualWeighingViewData& weighing) {
  if (heroVolume_ == nullptr || heroCapacity_ == nullptr ||
      heroProgress_ == nullptr) {
    return;
  }

  char value[40]{};
  std::snprintf(value, sizeof(value), "%.1f L",
                static_cast<double>(data.volumeL));
  setLabelText(heroVolume_, value);
  setLabelText(volumeValue_, value);

  const lv_color_t indicator = levelColor(data);
  std::snprintf(value, sizeof(value), "%u%% DA CAPACIDADE",
                static_cast<unsigned int>(data.percentage));
  setLabelText(heroCapacity_, value);
  lv_obj_set_style_text_color(heroCapacity_, indicator, LV_PART_MAIN);
  lv_bar_set_value(heroProgress_, data.percentage, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(heroProgress_, indicator, LV_PART_INDICATOR);

  std::snprintf(value, sizeof(value), "%.2f kg",
                static_cast<double>(data.tareKg));
  setLabelText(tareValue_, value);
  std::snprintf(value, sizeof(value), "%.3f kg/L",
                static_cast<double>(data.densityKgPerL));
  setLabelText(densityValue_, value);
  std::snprintf(value, sizeof(value), "%.2f kg",
                static_cast<double>(data.weightKg));
  setLabelText(weightValue_, value);
  std::snprintf(value, sizeof(value), "%.1f L",
                static_cast<double>(data.capacityL));
  setLabelText(capacityValue_, value);
  formatLocalDateTime(data.lastSyncUtc, true, value, sizeof(value));
  setLabelText(syncValue_, value);
  setLabelText(nfcValue_, data.nfcUid == nullptr ? "---" : data.nfcUid);
  setLabelText(statusValue_,
               data.onScale ? app::trackingStatusText(data.trackingStatus)
                            : app::kegStatusText(data.status));
  if (statusValue_ != nullptr) {
    lv_obj_set_style_text_color(statusValue_, statusColor(data), LV_PART_MAIN);
  }
  const bool thisKeg = data.id != nullptr && weighing.kegId[0] != '\0' &&
                       std::strcmp(data.id, weighing.kegId.data()) == 0;
  const char* weighText = trackingButtonText(data);
  if (thisKeg) {
    switch (weighing.status) {
      case app::ManualWeighingStatus::WaitingForReading:
        weighText = "AGUARDANDO PESO...";
        break;
      case app::ManualWeighingStatus::ReadyToConfirm:
        weighText = "PESO PRONTO";
        break;
      case app::ManualWeighingStatus::Completed:
        weighText = data.trackingStatus == app::TrackingViewStatus::Monitoring
                        ? "MONITORANDO"
                        : "PESAGEM SALVA";
        break;
      case app::ManualWeighingStatus::TimedOut:
        weighText = "SEM LEITURA - PESAR";
        break;
      case app::ManualWeighingStatus::Error:
        weighText = "ERRO - PESAR";
        break;
      case app::ManualWeighingStatus::Idle:
        break;
    }
  }
  setLabelText(weighButtonLabel_, weighText);
  if (removeButton_ != nullptr) {
    if (data.onScale) {
      lv_obj_clear_state(removeButton_, LV_STATE_DISABLED);
    } else {
      lv_obj_add_state(removeButton_, LV_STATE_DISABLED);
    }
  }
  chart_.update(data);
}

void KegDetailScreen::handleWeigh(lv_event_t* const event) {
  auto* const self =
      static_cast<KegDetailScreen*>(lv_event_get_user_data(event));
  if (self != nullptr && self->weighHandler_ != nullptr) {
    self->weighHandler_(self->actionContext_);
  }
}

void KegDetailScreen::handleRemove(lv_event_t* const event) {
  auto* const self =
      static_cast<KegDetailScreen*>(lv_event_get_user_data(event));
  if (self != nullptr && self->removeHandler_ != nullptr) {
    self->removeHandler_(self->actionContext_);
  }
}

void KegDetailScreen::handleEdit(lv_event_t* const event) {
  auto* const self =
      static_cast<KegDetailScreen*>(lv_event_get_user_data(event));
  if (self != nullptr && self->editHandler_ != nullptr) {
    self->editHandler_(self->actionContext_);
  }
}

void KegDetailScreen::handleNfc(lv_event_t* const event) {
  auto* const self =
      static_cast<KegDetailScreen*>(lv_event_get_user_data(event));
  if (self != nullptr && self->nfcHandler_ != nullptr) {
    self->nfcHandler_(self->actionContext_);
  }
}

void KegDetailScreen::handleFinish(lv_event_t* const event) {
  auto* const self =
      static_cast<KegDetailScreen*>(lv_event_get_user_data(event));
  if (self != nullptr && self->finishHandler_ != nullptr) {
    self->finishHandler_(self->actionContext_);
  }
}

void KegDetailScreen::handleArchive(lv_event_t* const event) {
  auto* const self =
      static_cast<KegDetailScreen*>(lv_event_get_user_data(event));
  if (self != nullptr && self->archiveHandler_ != nullptr) {
    self->archiveHandler_(self->actionContext_);
  }
}

}  // namespace keezer::ui::screens
