#pragma once

#include <lvgl.h>

#include <cstdint>

#include "app/ViewData.h"

namespace keezer::ui::screens {

class TemperatureScreen final {
 public:
  using SetpointHandler = app::TemperatureSetpointResult (*)(
      std::int16_t setpointCentiCelsius, void* context);

  void create(lv_obj_t* parent, const app::FreezerViewData& data,
              SetpointHandler setpointHandler, void* context);
  void update(const app::FreezerViewData& data);

 private:
  static void handleDecrease(lv_event_t* event);
  static void handleIncrease(lv_event_t* event);
  static void handleSave(lv_event_t* event);

  void changeDraft(std::int16_t deltaCentiCelsius);
  void refreshDraft();
  void showFeedback(const char* text, lv_color_t color);

  SetpointHandler setpointHandler_{nullptr};
  void* setpointContext_{nullptr};
  lv_obj_t* temperatureValue_{nullptr};
  lv_obj_t* setpointValue_{nullptr};
  lv_obj_t* compressorState_{nullptr};
  lv_obj_t* compressorPower_{nullptr};
  lv_obj_t* compressorDetail_{nullptr};
  lv_obj_t* sensorState_{nullptr};
  lv_obj_t* sensorDetail_{nullptr};
  lv_obj_t* feedback_{nullptr};
  std::int16_t savedSetpointCentiCelsius_{0};
  std::int16_t draftSetpointCentiCelsius_{0};
  bool dirty_{false};
};

}  // namespace keezer::ui::screens
