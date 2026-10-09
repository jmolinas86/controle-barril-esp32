#pragma once

#include <lvgl.h>

namespace keezer::ui::widgets {

class Modal final {
 public:
  using ConfirmHandler = void (*)(void* context);

  void showNewKeg(lv_obj_t* parent);
  void showConfirm(lv_obj_t* parent, const char* title, const char* message,
                   ConfirmHandler handler, void* context,
                   const char* confirmText = "CONFIRMAR",
                   const char* cancelText = "CANCELAR",
                   ConfirmHandler cancelHandler = nullptr,
                   void* cancelContext = nullptr);
  void showInfo(lv_obj_t* parent, const char* title, const char* message);
  void close();
  void reset();
  bool isOpen() const;

 private:
  static void handleClose(lv_event_t* event);
  static void handleConfirm(lv_event_t* event);
  static void handleCancel(lv_event_t* event);

  lv_obj_t* overlay_{nullptr};
  ConfirmHandler confirmHandler_{nullptr};
  void* confirmContext_{nullptr};
  ConfirmHandler cancelHandler_{nullptr};
  void* cancelContext_{nullptr};
};

}  // namespace keezer::ui::widgets
