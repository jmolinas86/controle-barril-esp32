#pragma once

#include <lvgl.h>

#include "app/AppViewModel.h"

namespace keezer::ui::screens {

class NetworkSettingsScreen final {
 public:
  using ActionHandler = void (*)(void* context);

  void create(lv_obj_t* parent, const app::NetworkViewData& state,
              ActionHandler saveHandler, ActionHandler cancelHandler,
              void* context);
  void update(const app::NetworkViewData& state);
  app::NetworkEditRequest request() const;
  void showResult(app::NetworkSaveResult result);

 private:
  static lv_obj_t* createField(lv_obj_t* parent, const char* label,
                               const char* value, std::uint32_t maximumLength,
                               const char* acceptedCharacters,
                               NetworkSettingsScreen* owner);
  static void handleFieldFocused(lv_event_t* event);
  static void handleKeyboard(lv_event_t* event);
  static void handleSave(lv_event_t* event);
  static void handleCancel(lv_event_t* event);
  void openKeyboard(lv_obj_t* field);
  void closeKeyboard();

  lv_obj_t* scroll_{nullptr};
  lv_obj_t* keyboard_{nullptr};
  lv_obj_t* resultLabel_{nullptr};
  lv_obj_t* connectionLabel_{nullptr};
  lv_obj_t* ipLabel_{nullptr};
  lv_obj_t* rssiLabel_{nullptr};
  lv_obj_t* ssidField_{nullptr};
  lv_obj_t* passwordField_{nullptr};
  lv_obj_t* hostnameField_{nullptr};
  lv_obj_t* scaleHostField_{nullptr};
  lv_obj_t* scalePortField_{nullptr};
  ActionHandler saveHandler_{nullptr};
  ActionHandler cancelHandler_{nullptr};
  void* context_{nullptr};
};

}  // namespace keezer::ui::screens
