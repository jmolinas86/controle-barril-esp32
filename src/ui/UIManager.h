#pragma once

#include <lvgl.h>

#include <cstdint>

#include "BuildConfig.h"
#include "app/AppViewModel.h"
#include "ui/ScreenManager.h"
#include "ui/widgets/BottomNavigation.h"
#include "ui/widgets/Header.h"

namespace keezer::bsp {
class BoardSupport;
}

namespace keezer::ui {

class UIManager final {
 public:
  bool begin(bsp::BoardSupport& board, app::AppViewModel& viewModel);
  void update(std::uint32_t nowMs);
  void wakeDisplay(std::uint32_t nowMs);

 private:
  static void flushDisplay(lv_display_t* display, const lv_area_t* area,
                           std::uint8_t* pixelMap);
  static void readTouch(lv_indev_t* inputDevice, lv_indev_data_t* data);
  static void handleDisplayRefresh(lv_event_t* event);
  static std::uint32_t tickMs();

  bool initializeLvgl();
  bool createShell(app::AppViewModel& viewModel);
  bool touchInteractionActive() const;
  void logTelemetry(std::uint32_t nowMs);

  static UIManager* instance_;

  bsp::BoardSupport* board_{nullptr};
  app::AppViewModel* viewModel_{nullptr};
  lv_display_t* lvDisplay_{nullptr};
  lv_indev_t* lvTouch_{nullptr};
  lv_obj_t* contentRoot_{nullptr};
  widgets::Header header_;
  widgets::BottomNavigation navigation_;
  ScreenManager screenManager_;
  std::uint32_t nextTelemetryAtMs_{0U};
  std::uint32_t nextHandlerAtMs_{0U};
  std::uint32_t telemetryWindowStartedAtMs_{0U};
  std::uint32_t handlerCalls_{0U};
  std::uint64_t handlerTotalUs_{0U};
  std::uint32_t handlerMaxUs_{0U};
  std::uint32_t refreshCycles_{0U};
  std::uint32_t flushCalls_{0U};
  std::uint64_t flushTotalUs_{0U};
  std::uint32_t flushMaxUs_{0U};
  std::uint64_t flushedPixels_{0U};
  bool displayFrameOpen_{false};
  std::uint32_t lastInteractionAtMs_{0U};
  std::uint16_t appliedTimeoutSeconds_{60U};
  std::uint8_t appliedBrightnessPercent_{70U};
  bool consumeWakeTouch_{false};

  alignas(4) std::uint8_t
      drawBuffer1_[config::kDisplayBufferPixels * sizeof(std::uint16_t)]{};
};

}  // namespace keezer::ui
