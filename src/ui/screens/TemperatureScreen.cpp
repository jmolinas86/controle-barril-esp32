#include "ui/screens/TemperatureScreen.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "BuildConfig.h"
#include "ui/Layout.h"
#include "ui/Theme.h"
#include "ui/widgets/ActionButton.h"
#include "ui/widgets/Card.h"

namespace keezer::ui::screens {
namespace {

void setLabelText(lv_obj_t* const label, const char* const text) {
  if (label != nullptr && text != nullptr &&
      std::strcmp(lv_label_get_text(label), text) != 0) {
    lv_label_set_text(label, text);
  }
}

void setLabelColor(lv_obj_t* const label, const lv_color_t color) {
  if (label != nullptr) {
    lv_obj_set_style_text_color(label, color, LV_PART_MAIN);
  }
}

const char* controlText(const models::TemperatureControlState state) {
  switch (state) {
    case models::TemperatureControlState::Idle:
      return "PARADO";
    case models::TemperatureControlState::Waiting:
      return "AGUARDANDO";
    case models::TemperatureControlState::Cooling:
      return "COOLING";
    case models::TemperatureControlState::Error:
      return "ERRO";
  }
  return "ERRO";
}

lv_color_t controlColor(const models::TemperatureControlState state) {
  switch (state) {
    case models::TemperatureControlState::Idle:
      return Theme::muted();
    case models::TemperatureControlState::Waiting:
      return Theme::warning();
    case models::TemperatureControlState::Cooling:
      return Theme::good();
    case models::TemperatureControlState::Error:
      return Theme::danger();
  }
  return Theme::danger();
}

const char* sensorText(const models::TemperatureSampleStatus status) {
  switch (status) {
    case models::TemperatureSampleStatus::NoData:
      return "SEM LEITURA";
    case models::TemperatureSampleStatus::Valid:
      return "CONECTADO";
    case models::TemperatureSampleStatus::Stale:
      return "SEM COMUNICACAO";
    case models::TemperatureSampleStatus::Disconnected:
      return "DESCONECTADO";
    case models::TemperatureSampleStatus::OutOfRange:
      return "FORA DA FAIXA";
    case models::TemperatureSampleStatus::Error:
      return "ERRO DE LEITURA";
  }
  return "ERRO";
}

lv_color_t sensorColor(const models::TemperatureSampleStatus status) {
  switch (status) {
    case models::TemperatureSampleStatus::Valid:
      return Theme::good();
    case models::TemperatureSampleStatus::NoData:
    case models::TemperatureSampleStatus::Stale:
      return Theme::warning();
    case models::TemperatureSampleStatus::Disconnected:
    case models::TemperatureSampleStatus::OutOfRange:
    case models::TemperatureSampleStatus::Error:
      return Theme::danger();
  }
  return Theme::danger();
}

const char* faultText(const models::TemperatureFault fault) {
  switch (fault) {
    case models::TemperatureFault::None:
      return "LEITURA VALIDA";
    case models::TemperatureFault::NoSample:
      return "AGUARDANDO PRIMEIRA LEITURA";
    case models::TemperatureFault::Timeout:
      return "TIMEOUT DO SENSOR";
    case models::TemperatureFault::Disconnected:
      return "SENSOR AUSENTE";
    case models::TemperatureFault::OutOfRange:
      return "TEMPERATURA INVALIDA";
    case models::TemperatureFault::SensorError:
      return "FALHA NA LEITURA";
    case models::TemperatureFault::CompressorOutputError:
      return "FALHA NA SAIDA";
  }
  return "FALHA NA LEITURA";
}

}  // namespace

void TemperatureScreen::create(lv_obj_t* const parent,
                               const app::FreezerViewData& data,
                               const SetpointHandler setpointHandler,
                               void* const context) {
  setpointHandler_ = setpointHandler;
  setpointContext_ = context;
  dirty_ = false;
  savedSetpointCentiCelsius_ = static_cast<std::int16_t>(
      std::lround(static_cast<double>(data.setpointC) * 100.0));
  draftSetpointCentiCelsius_ = savedSetpointCentiCelsius_;

  lv_obj_t* const temperatureCard =
      widgets::Card::create(parent, 6, 4, layout::kCardWidth, 83);
  lv_obj_set_style_border_color(temperatureCard, Theme::primary(),
                                LV_PART_MAIN);
  lv_obj_t* const currentTitle = widgets::Card::addLabel(
      temperatureCard, "TEMP ATUAL", 0, 8, Theme::primary(),
      &lv_font_montserrat_12);
  lv_obj_set_width(currentTitle, 104);
  lv_obj_set_style_text_align(currentTitle, LV_TEXT_ALIGN_CENTER,
                              LV_PART_MAIN);
  temperatureValue_ = widgets::Card::addLabel(
      temperatureCard, "--.- C", 0, 31, Theme::text(),
      &lv_font_montserrat_28);
  lv_obj_set_width(temperatureValue_, 104);
  lv_obj_set_style_text_align(temperatureValue_, LV_TEXT_ALIGN_CENTER,
                              LV_PART_MAIN);

  widgets::Card::addLabel(temperatureCard, "SETPOINT", 139, 8,
                          Theme::primary(), &lv_font_montserrat_12);
  lv_obj_t* const decrease = widgets::ActionButton::create(
      temperatureCard, "-", 30, 31, handleDecrease, this, false);
  lv_obj_set_pos(decrease, 106, 41);
  setpointValue_ = widgets::Card::addLabel(
      temperatureCard, "2.0", 139, 46, Theme::text(),
      &lv_font_montserrat_20);
  lv_obj_set_width(setpointValue_, 48);
  lv_obj_set_style_text_align(setpointValue_, LV_TEXT_ALIGN_CENTER,
                              LV_PART_MAIN);
  lv_obj_t* const increase = widgets::ActionButton::create(
      temperatureCard, "+", 30, 31, handleIncrease, this, false);
  lv_obj_set_pos(increase, 190, 41);

  lv_obj_t* const saveButton = widgets::ActionButton::create(
      parent, LV_SYMBOL_SAVE "  SALVAR SETPOINT", layout::kCardWidth, 31,
      handleSave, this, true);
  lv_obj_set_pos(saveButton, 6, 92);

  lv_obj_t* const compressorCard =
      widgets::Card::create(parent, 6, 128, layout::kCardWidth, 55);
  widgets::Card::addLabel(compressorCard, "COMPRESSOR", 10, 6,
                          Theme::primary(), &lv_font_montserrat_12);
  compressorState_ = widgets::Card::addLabel(
      compressorCard, "---", 10, 27, Theme::muted(),
      &lv_font_montserrat_14);
  compressorPower_ = widgets::Card::addLabel(
      compressorCard, "OFF", 181, 6, Theme::muted(),
      &lv_font_montserrat_12);
  lv_obj_set_width(compressorPower_, 36);
  lv_obj_set_style_text_align(compressorPower_, LV_TEXT_ALIGN_RIGHT,
                              LV_PART_MAIN);
  compressorDetail_ = widgets::Card::addLabel(
      compressorCard, "SEM DEMANDA", 98, 29, Theme::muted(),
      &lv_font_montserrat_10);
  lv_obj_set_width(compressorDetail_, 119);
  lv_obj_set_style_text_align(compressorDetail_, LV_TEXT_ALIGN_RIGHT,
                              LV_PART_MAIN);

  lv_obj_t* const sensorCard =
      widgets::Card::create(parent, 6, 188, layout::kCardWidth, 57);
  widgets::Card::addLabel(sensorCard, "SENSOR", 10, 6, Theme::primary(),
                          &lv_font_montserrat_12);
  sensorState_ = widgets::Card::addLabel(
      sensorCard, "---", 72, 6, Theme::muted(),
      &lv_font_montserrat_12);
  sensorDetail_ = widgets::Card::addLabel(
      sensorCard, "---", 10, 34, Theme::muted(),
      &lv_font_montserrat_10);
  feedback_ = widgets::Card::addLabel(
      sensorCard, "", 116, 34, Theme::muted(), &lv_font_montserrat_10);
  lv_obj_set_width(feedback_, 101);
  lv_obj_set_style_text_align(feedback_, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);

  refreshDraft();
  update(data);
}

void TemperatureScreen::update(const app::FreezerViewData& data) {
  char value[32]{};
  if (data.hasTemperature) {
    std::snprintf(value, sizeof(value), "%.1f C",
                  static_cast<double>(data.temperatureC));
  } else {
    std::snprintf(value, sizeof(value), "--.- C");
  }
  setLabelText(temperatureValue_, value);

  const std::int16_t serviceSetpoint = static_cast<std::int16_t>(
      std::lround(static_cast<double>(data.setpointC) * 100.0));
  savedSetpointCentiCelsius_ = serviceSetpoint;
  if (!dirty_) {
    draftSetpointCentiCelsius_ = serviceSetpoint;
    refreshDraft();
  }

  setLabelText(compressorState_, controlText(data.controlState));
  setLabelColor(compressorState_, controlColor(data.controlState));
  setLabelText(compressorPower_, data.compressorOn ? "ON" : "OFF");
  setLabelColor(compressorPower_,
                data.compressorOn ? Theme::good() : Theme::muted());

  if (data.controlState == models::TemperatureControlState::Waiting &&
      data.protectionRemainingMs > 0U) {
    const unsigned long seconds = static_cast<unsigned long>(
        (data.protectionRemainingMs + 999U) / 1'000U);
    std::snprintf(value, sizeof(value), "PROTECAO: %lus", seconds);
  } else if (data.controlState ==
             models::TemperatureControlState::Cooling) {
    std::snprintf(value, sizeof(value), "COMANDO LOGICO ON");
  } else if (data.demandCooling) {
    std::snprintf(value, sizeof(value), "DEMANDA ATIVA");
  } else {
    std::snprintf(value, sizeof(value), "SEM DEMANDA");
  }
  setLabelText(compressorDetail_, value);
  setLabelColor(compressorDetail_,
                data.controlState == models::TemperatureControlState::Error
                    ? Theme::danger()
                    : (data.controlState ==
                               models::TemperatureControlState::Waiting
                           ? Theme::warning()
                           : Theme::muted()));

  setLabelText(sensorState_, sensorText(data.sampleStatus));
  setLabelColor(sensorState_, sensorColor(data.sampleStatus));
  setLabelText(sensorDetail_, faultText(data.temperatureFault));
  setLabelColor(sensorDetail_,
                data.temperatureFault == models::TemperatureFault::None
                    ? Theme::muted()
                    : Theme::danger());
}

void TemperatureScreen::handleDecrease(lv_event_t* const event) {
  auto* const self =
      static_cast<TemperatureScreen*>(lv_event_get_user_data(event));
  if (self != nullptr) {
    self->changeDraft(-config::kTemperatureSetpointStepCentiCelsius);
  }
}

void TemperatureScreen::handleIncrease(lv_event_t* const event) {
  auto* const self =
      static_cast<TemperatureScreen*>(lv_event_get_user_data(event));
  if (self != nullptr) {
    self->changeDraft(config::kTemperatureSetpointStepCentiCelsius);
  }
}

void TemperatureScreen::handleSave(lv_event_t* const event) {
  auto* const self =
      static_cast<TemperatureScreen*>(lv_event_get_user_data(event));
  if (self == nullptr) {
    return;
  }
  if (!self->dirty_) {
    self->showFeedback("SEM ALTERACAO", Theme::muted());
    return;
  }
  if (self->setpointHandler_ == nullptr) {
    self->showFeedback("SERVICO INDISP.", Theme::danger());
    return;
  }
  const app::TemperatureSetpointResult result = self->setpointHandler_(
      self->draftSetpointCentiCelsius_, self->setpointContext_);
  switch (result) {
    case app::TemperatureSetpointResult::Ok:
      self->savedSetpointCentiCelsius_ =
          self->draftSetpointCentiCelsius_;
      self->dirty_ = false;
      self->showFeedback("SETPOINT SALVO", Theme::good());
      break;
    case app::TemperatureSetpointResult::OutOfRange:
      self->showFeedback("FORA DO LIMITE", Theme::danger());
      break;
    case app::TemperatureSetpointResult::StorageError:
      self->showFeedback("ERRO AO SALVAR", Theme::danger());
      break;
    case app::TemperatureSetpointResult::ServiceError:
      self->showFeedback("CONTROLE INDISP.", Theme::danger());
      break;
  }
}

void TemperatureScreen::changeDraft(const std::int16_t deltaCentiCelsius) {
  const std::int32_t candidate =
      static_cast<std::int32_t>(draftSetpointCentiCelsius_) +
      deltaCentiCelsius;
  if (candidate < config::kTemperatureMinimumSetpointCentiCelsius ||
      candidate > config::kTemperatureMaximumSetpointCentiCelsius) {
    showFeedback("LIMITE ATINGIDO", Theme::warning());
    return;
  }
  draftSetpointCentiCelsius_ = static_cast<std::int16_t>(candidate);
  dirty_ = draftSetpointCentiCelsius_ != savedSetpointCentiCelsius_;
  refreshDraft();
  showFeedback(dirty_ ? "ALTERACAO PENDENTE" : "", Theme::warning());
}

void TemperatureScreen::refreshDraft() {
  char value[16]{};
  const std::int32_t absoluteValue =
      draftSetpointCentiCelsius_ < 0
          ? -static_cast<std::int32_t>(draftSetpointCentiCelsius_)
          : draftSetpointCentiCelsius_;
  std::snprintf(value, sizeof(value), "%s%ld.%01ld",
                draftSetpointCentiCelsius_ < 0 ? "-" : "",
                static_cast<long>(absoluteValue / 100),
                static_cast<long>((absoluteValue % 100) / 10));
  setLabelText(setpointValue_, value);
  setLabelColor(setpointValue_, dirty_ ? Theme::warning() : Theme::text());
}

void TemperatureScreen::showFeedback(const char* const text,
                                     const lv_color_t color) {
  setLabelText(feedback_, text == nullptr ? "" : text);
  setLabelColor(feedback_, color);
}

}  // namespace keezer::ui::screens
