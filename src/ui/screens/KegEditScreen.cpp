#include "ui/screens/KegEditScreen.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "diagnostics/Logger.h"
#include "ui/Layout.h"
#include "ui/Theme.h"
#include "ui/widgets/ActionButton.h"
#include "ui/widgets/ScrollContainer.h"

namespace keezer::ui::screens {
namespace {

constexpr char kLogTag[] = "KEG_EDIT";
constexpr char kNumericCharacters[] = "0123456789.,";
constexpr char kNfcCharacters[] = "0123456789ABCDEFabcdef:- ";
constexpr char kDateCharacters[] = "0123456789/";

const char* safeText(const char* const value) {
  return value == nullptr ? "" : value;
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

void KegEditScreen::create(lv_obj_t* const parent,
                           const app::KegViewData* const keg,
                           const char* const suggestedId,
                           const ActionHandler saveHandler,
                           const ActionHandler cancelHandler,
                           void* const context, const bool focusNfc,
                           const char* const prefilledNfc) {
  editing_ = keg != nullptr;
  std::snprintf(originalId_.data(), originalId_.size(), "%s",
                editing_ ? safeText(keg->id) : "");
  saveHandler_ = saveHandler;
  cancelHandler_ = cancelHandler;
  context_ = context;

  scroll_ = widgets::ScrollContainer::create(parent);
  lv_obj_set_style_pad_row(scroll_, 3, LV_PART_MAIN);

  statusLabel_ = lv_label_create(scroll_);
  lv_label_set_text(statusLabel_,
                    editing_ ? "EDITE E TOQUE EM SALVAR"
                             : "PREENCHA OS DADOS DO NOVO KEG");
  lv_obj_set_width(statusLabel_, layout::kCardWidth);
  lv_obj_set_style_text_align(statusLabel_, LV_TEXT_ALIGN_CENTER,
                              LV_PART_MAIN);
  lv_obj_set_style_text_color(statusLabel_, Theme::primary(), LV_PART_MAIN);
  lv_obj_set_style_text_font(statusLabel_, &lv_font_montserrat_10,
                             LV_PART_MAIN);

  idField_ = createField(scroll_, "ID", editing_ ? keg->id : suggestedId,
                         models::kKegIdBytes - 1U, nullptr, this);
  if (editing_) {
    lv_textarea_set_one_line(idField_, true);
    lv_obj_add_state(idField_, LV_STATE_DISABLED);
  }
  nameField_ = createField(scroll_, "CERVEJA", editing_ ? keg->name : "",
                           models::kKegTextBytes - 1U, nullptr, this);
  styleField_ = createField(scroll_, "ESTILO",
                            editing_ ? keg->beerStyle : "",
                            models::kKegTextBytes - 1U, nullptr, this);
  batchField_ = createField(scroll_, "LOTE", editing_ ? keg->batch : "",
                            models::kKegTextBytes - 1U, nullptr, this);
  filledDateField_ = createField(
      scroll_, "DATA DE ENVASE (DD/MM/AAAA)",
      editing_ ? keg->filledDate : "", 10U, kDateCharacters, this);

  char number[24]{};
  std::snprintf(number, sizeof(number), "%.1f",
                static_cast<double>(editing_ ? keg->capacityL : 20.0F));
  capacityField_ = createField(scroll_, "CAPACIDADE (L)", number, 7U,
                               kNumericCharacters, this);
  std::snprintf(number, sizeof(number), "%.2f",
                static_cast<double>(editing_ ? keg->tareKg : 4.35F));
  tareField_ = createField(scroll_, "TARA (kg)", number, 7U,
                           kNumericCharacters, this);
  std::snprintf(number, sizeof(number), "%.3f",
                static_cast<double>(editing_ ? keg->densityKgPerL : 1.000F));
  densityField_ = createField(scroll_, "DENSIDADE (kg/L)", number, 7U,
                              kNumericCharacters, this);
  nfcField_ = createField(scroll_, "NFC UID",
                          editing_ ? keg->nfcUid : safeText(prefilledNfc),
                          models::kNfcUidBytes - 1U, kNfcCharacters, this);
  notesField_ = createField(scroll_, "OBSERVACOES",
                            editing_ ? keg->notes : "",
                            models::kKegNotesBytes - 1U, nullptr, this);

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
  lv_obj_set_style_bg_color(keyboard_, Theme::primaryDark(),
                            LV_PART_ITEMS | LV_STATE_PRESSED);
  lv_obj_add_event_cb(keyboard_, handleKeyboard, LV_EVENT_READY, this);
  lv_obj_add_event_cb(keyboard_, handleKeyboard, LV_EVENT_CANCEL, this);
  lv_obj_add_flag(keyboard_, LV_OBJ_FLAG_HIDDEN);

  // Always present a usable keyboard when entering the editor. Field click
  // events still retarget it afterwards, but the first render no longer
  // depends on focus behavior from the resistive pointer driver.
  openKeyboard(focusNfc ? nfcField_ : nameField_);
}

app::KegEditRequest KegEditScreen::request() const {
  return {
      idField_ == nullptr ? "" : lv_textarea_get_text(idField_),
      nfcField_ == nullptr ? "" : lv_textarea_get_text(nfcField_),
      nameField_ == nullptr ? "" : lv_textarea_get_text(nameField_),
      styleField_ == nullptr ? "" : lv_textarea_get_text(styleField_),
      batchField_ == nullptr ? "" : lv_textarea_get_text(batchField_),
      notesField_ == nullptr ? "" : lv_textarea_get_text(notesField_),
      filledDateField_ == nullptr ? ""
                                  : lv_textarea_get_text(filledDateField_),
      parseDecimal(capacityField_ == nullptr
                       ? ""
                       : lv_textarea_get_text(capacityField_)),
      parseDecimal(tareField_ == nullptr ? ""
                                        : lv_textarea_get_text(tareField_)),
      parseDecimal(densityField_ == nullptr
                       ? ""
                       : lv_textarea_get_text(densityField_)),
  };
}

const char* KegEditScreen::originalId() const { return originalId_.data(); }

bool KegEditScreen::editing() const { return editing_; }

void KegEditScreen::showError(const services::KegError error) {
  if (statusLabel_ == nullptr) {
    return;
  }
  const char* message = "NAO FOI POSSIVEL SALVAR";
  switch (error) {
    case services::KegError::InvalidData:
      message = "CONFIRA NOME, MEDIDAS E NFC";
      break;
    case services::KegError::CatalogFull:
      message = "LIMITE DE 32 KEGS ATINGIDO";
      break;
    case services::KegError::DuplicateId:
      message = "ESTE ID JA EXISTE";
      break;
    case services::KegError::DuplicateNfc:
      message = "ESTA TAG JA PERTENCE A OUTRO KEG";
      break;
    case services::KegError::StorageError:
      message = "ERRO AO GRAVAR NO ARMAZENAMENTO";
      break;
    case services::KegError::NotFound:
      message = "KEG NAO ENCONTRADO";
      break;
    default:
      break;
  }
  lv_label_set_text(statusLabel_, message);
  lv_obj_set_style_text_color(statusLabel_, Theme::danger(), LV_PART_MAIN);
  lv_obj_scroll_to_view(statusLabel_, LV_ANIM_ON);
}

lv_obj_t* KegEditScreen::createField(
    lv_obj_t* const parent, const char* const label, const char* const value,
    const std::uint32_t maximumLength, const char* const acceptedCharacters,
    KegEditScreen* const owner) {
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

void KegEditScreen::handleFieldFocused(lv_event_t* const event) {
  auto* const self =
      static_cast<KegEditScreen*>(lv_event_get_user_data(event));
  if (self != nullptr) {
    self->openKeyboard(static_cast<lv_obj_t*>(lv_event_get_target(event)));
  }
}

void KegEditScreen::handleKeyboard(lv_event_t* const event) {
  auto* const self =
      static_cast<KegEditScreen*>(lv_event_get_user_data(event));
  if (self != nullptr) {
    self->closeKeyboard();
  }
}

void KegEditScreen::handleSave(lv_event_t* const event) {
  auto* const self =
      static_cast<KegEditScreen*>(lv_event_get_user_data(event));
  if (self != nullptr && self->saveHandler_ != nullptr) {
    self->closeKeyboard();
    self->saveHandler_(self->context_);
  }
}

void KegEditScreen::handleCancel(lv_event_t* const event) {
  auto* const self =
      static_cast<KegEditScreen*>(lv_event_get_user_data(event));
  if (self != nullptr && self->cancelHandler_ != nullptr) {
    self->closeKeyboard();
    self->cancelHandler_(self->context_);
  }
}

void KegEditScreen::openKeyboard(lv_obj_t* const field) {
  if (keyboard_ == nullptr || field == nullptr ||
      lv_obj_has_state(field, LV_STATE_DISABLED)) {
    return;
  }
  const bool numeric = field == capacityField_ || field == tareField_ ||
                       field == densityField_;
  lv_keyboard_set_mode(keyboard_, numeric ? LV_KEYBOARD_MODE_NUMBER
                                          : LV_KEYBOARD_MODE_TEXT_LOWER);
  lv_keyboard_set_textarea(keyboard_, field);
  lv_obj_clear_flag(keyboard_, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_height(scroll_, 132);
  lv_obj_move_foreground(keyboard_);
  lv_obj_update_layout(scroll_);
  lv_obj_scroll_to_view_recursive(field, LV_ANIM_ON);
  lv_obj_invalidate(keyboard_);
  KEEZER_LOG_INFO(kLogTag, "Virtual keyboard opened (%s)",
                  numeric ? "numeric" : "text");
}

void KegEditScreen::closeKeyboard() {
  if (keyboard_ == nullptr) {
    return;
  }
  lv_keyboard_set_textarea(keyboard_, nullptr);
  lv_obj_add_flag(keyboard_, LV_OBJ_FLAG_HIDDEN);
  if (scroll_ != nullptr) {
    lv_obj_set_height(scroll_, layout::kContentHeight);
  }
}

float KegEditScreen::parseDecimal(const char* const text) {
  if (text == nullptr || text[0] == '\0') {
    return 0.0F;
  }
  char normalized[24]{};
  std::snprintf(normalized, sizeof(normalized), "%s", text);
  std::replace(normalized, normalized + std::strlen(normalized), ',', '.');
  return std::strtof(normalized, nullptr);
}

}  // namespace keezer::ui::screens
