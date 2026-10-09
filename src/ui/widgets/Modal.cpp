#include "ui/widgets/Modal.h"

#include <cstring>

#include "ui/Layout.h"
#include "ui/Theme.h"
#include "ui/widgets/ActionButton.h"
#include "ui/widgets/Card.h"

namespace keezer::ui::widgets {

void Modal::showNewKeg(lv_obj_t* const parent) {
  close();
  overlay_ = lv_obj_create(parent);
  lv_obj_set_pos(overlay_, 0, 0);
  lv_obj_set_size(overlay_, layout::kScreenWidth, layout::kContentHeight);
  Theme::applyOverlay(overlay_);
  lv_obj_add_flag(overlay_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(overlay_, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* const dialog = lv_obj_create(overlay_);
  lv_obj_set_size(dialog, 202, 116);
  lv_obj_center(dialog);
  Theme::applyModal(dialog);
  lv_obj_clear_flag(dialog, LV_OBJ_FLAG_SCROLLABLE);

  Card::addLabel(dialog, LV_SYMBOL_PLUS "  NOVO BARRIL", 24, 18,
                 Theme::text(), &lv_font_montserrat_16);
  Card::addLabel(dialog, "Cadastro disponivel em fase futura", 13, 48,
                 Theme::muted(), &lv_font_montserrat_12);
  lv_obj_t* const closeButton = ActionButton::create(
      dialog, "FECHAR", 112, 36, handleClose, this, true);
  lv_obj_set_pos(closeButton, 45, 73);
}

void Modal::showConfirm(lv_obj_t* const parent, const char* const title,
                        const char* const message,
                        const ConfirmHandler handler, void* const context,
                        const char* const confirmText,
                        const char* const cancelText,
                        const ConfirmHandler cancelHandler,
                        void* const cancelContext) {
  close();
  confirmHandler_ = handler;
  confirmContext_ = context;
  cancelHandler_ = cancelHandler;
  cancelContext_ = cancelContext;
  overlay_ = lv_obj_create(parent);
  lv_obj_set_pos(overlay_, 0, 0);
  lv_obj_set_size(overlay_, layout::kScreenWidth, layout::kContentHeight);
  Theme::applyOverlay(overlay_);
  lv_obj_add_flag(overlay_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(overlay_, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* const dialog = lv_obj_create(overlay_);
  lv_obj_set_size(dialog, 216, 126);
  lv_obj_center(dialog);
  Theme::applyModal(dialog);
  lv_obj_clear_flag(dialog, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* const titleLabel = Card::addLabel(
      dialog, title, 8, 14, Theme::text(), &lv_font_montserrat_16);
  lv_obj_set_width(titleLabel, 200);
  lv_obj_set_style_text_align(titleLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  const std::int32_t messageY =
      message != nullptr &&
              (std::strncmp(message, "ATENCAO:", 8U) == 0 ||
               std::strchr(message, '\n') != nullptr)
          ? 35
          : 43;
  lv_color_t messageColor = Theme::muted();
  if (message != nullptr &&
      (std::strncmp(message, "PESO ACIMA", 10U) == 0 ||
       std::strncmp(message, "PESO ABAIXO", 11U) == 0)) {
    messageColor = Theme::warning();
  } else if (message != nullptr &&
             std::strncmp(message, "NFC INDICA OUTRO KEG", 20U) == 0) {
    messageColor = Theme::danger();
  }
  lv_obj_t* const messageLabel = Card::addLabel(
      dialog, message, 12, messageY, messageColor, &lv_font_montserrat_12);
  lv_obj_set_width(messageLabel, 192);
  lv_label_set_long_mode(messageLabel, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(messageLabel, LV_TEXT_ALIGN_CENTER,
                              LV_PART_MAIN);

  lv_obj_t* const cancel = ActionButton::create(
      dialog, cancelText == nullptr ? "CANCELAR" : cancelText, 92, 34,
      handleCancel, this, false);
  lv_obj_set_pos(cancel, 8, 84);
  lv_obj_t* const confirm = ActionButton::create(
      dialog, confirmText == nullptr ? "CONFIRMAR" : confirmText, 102, 34,
      handleConfirm, this, true);
  lv_obj_set_pos(confirm, 106, 84);
}

void Modal::showInfo(lv_obj_t* const parent, const char* const title,
                     const char* const message) {
  close();
  overlay_ = lv_obj_create(parent);
  lv_obj_set_pos(overlay_, 0, 0);
  lv_obj_set_size(overlay_, layout::kScreenWidth, layout::kContentHeight);
  Theme::applyOverlay(overlay_);
  lv_obj_add_flag(overlay_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(overlay_, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* dialog = lv_obj_create(overlay_);
  lv_obj_set_size(dialog, 216, 126);
  lv_obj_center(dialog);
  Theme::applyModal(dialog);
  lv_obj_clear_flag(dialog, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t* titleLabel = Card::addLabel(
      dialog, title, 8, 14, Theme::text(), &lv_font_montserrat_16);
  lv_obj_set_width(titleLabel, 200);
  lv_obj_set_style_text_align(titleLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_t* messageLabel = Card::addLabel(
      dialog, message, 12, 42, Theme::muted(), &lv_font_montserrat_12);
  lv_obj_set_width(messageLabel, 192);
  lv_label_set_long_mode(messageLabel, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(messageLabel, LV_TEXT_ALIGN_CENTER,
                              LV_PART_MAIN);
  lv_obj_t* closeButton = ActionButton::create(
      dialog, "FECHAR", 112, 32, handleClose, this, true);
  lv_obj_set_pos(closeButton, 52, 88);
}

void Modal::close() {
  if (overlay_ != nullptr && lv_obj_is_valid(overlay_)) {
    lv_obj_delete(overlay_);
  }
  overlay_ = nullptr;
}

void Modal::reset() {
  overlay_ = nullptr;
  confirmHandler_ = nullptr;
  confirmContext_ = nullptr;
  cancelHandler_ = nullptr;
  cancelContext_ = nullptr;
}

bool Modal::isOpen() const {
  return overlay_ != nullptr && lv_obj_is_valid(overlay_);
}

void Modal::handleClose(lv_event_t* const event) {
  auto* const self = static_cast<Modal*>(lv_event_get_user_data(event));
  if (self != nullptr) {
    self->close();
  }
}

void Modal::handleConfirm(lv_event_t* const event) {
  auto* const self = static_cast<Modal*>(lv_event_get_user_data(event));
  if (self == nullptr) {
    return;
  }
  const ConfirmHandler handler = self->confirmHandler_;
  void* const context = self->confirmContext_;
  self->close();
  self->confirmHandler_ = nullptr;
  self->confirmContext_ = nullptr;
  if (handler != nullptr) {
    handler(context);
  }
}

void Modal::handleCancel(lv_event_t* const event) {
  auto* const self = static_cast<Modal*>(lv_event_get_user_data(event));
  if (self == nullptr) {
    return;
  }
  const ConfirmHandler handler = self->cancelHandler_;
  void* const context = self->cancelContext_;
  self->close();
  self->cancelHandler_ = nullptr;
  self->cancelContext_ = nullptr;
  if (handler != nullptr) {
    handler(context);
  }
}

}  // namespace keezer::ui::widgets
