#pragma once

#include <lvgl.h>

#include <cstdint>

#include "app/ViewData.h"

namespace keezer::ui::screens {

class DisplaySettingsScreen final {
 public:
  using SaveHandler = app::DisplaySaveResult (*)(std::uint8_t brightness,
                                                  std::uint16_t timeoutSeconds,
                                                  void* context);

  void create(lv_obj_t* parent, const models::DisplaySettings& settings,
              SaveHandler saveHandler, void* context);
  void showResult(app::DisplaySaveResult result);

 private:
  static void handleBrightnessDown(lv_event_t* event);
  static void handleBrightnessUp(lv_event_t* event);
  static void handleTimeoutDown(lv_event_t* event);
  static void handleTimeoutUp(lv_event_t* event);
  static void handleSave(lv_event_t* event);
  void refreshValues();

  lv_obj_t* brightnessLabel_{nullptr};
  lv_obj_t* timeoutLabel_{nullptr};
  lv_obj_t* resultLabel_{nullptr};
  std::uint8_t brightness_{70U};
  std::uint8_t timeoutIndex_{2U};
  SaveHandler saveHandler_{nullptr};
  void* context_{nullptr};
};

}  // namespace keezer::ui::screens
