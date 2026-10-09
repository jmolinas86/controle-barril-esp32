#include "ui/UIManager.h"

#include <Arduino.h>
#include <esp_heap_caps.h>

#include <algorithm>

#include "bsp/BoardSupport.h"
#include "diagnostics/Logger.h"
#include "ui/Layout.h"
#include "ui/Theme.h"

namespace keezer::ui {
namespace {

constexpr char kLogTag[] = "UI";

}  // namespace

UIManager* UIManager::instance_ = nullptr;

bool UIManager::begin(bsp::BoardSupport& board,
                      app::AppViewModel& viewModel) {
  board_ = &board;
  viewModel_ = &viewModel;
  instance_ = this;

  if (!initializeLvgl() || !createShell(viewModel)) {
    KEEZER_LOG_ERROR(kLogTag, "Could not initialize Phase 18 UI");
    return false;
  }

  const std::uint32_t nowMs = millis();
  appliedBrightnessPercent_ = viewModel.display().brightnessPercent;
  appliedTimeoutSeconds_ = viewModel.display().timeoutSeconds;
  board_->setDisplayBrightnessPercent(appliedBrightnessPercent_);
  board_->setDisplayAwake(true);
  lastInteractionAtMs_ = nowMs;
  telemetryWindowStartedAtMs_ = nowMs;
  nextHandlerAtMs_ = nowMs;
  logTelemetry(nowMs);
  nextTelemetryAtMs_ = nowMs + config::kUiTelemetryPeriodMs;
  KEEZER_LOG_INFO(kLogTag, "Phase 18 UI ready");
  return true;
}

void UIManager::update(const std::uint32_t nowMs) {
  if (board_ != nullptr && viewModel_ != nullptr) {
    const models::DisplaySettings& settings = viewModel_->display();
    if (settings.brightnessPercent != appliedBrightnessPercent_) {
      appliedBrightnessPercent_ = settings.brightnessPercent;
      board_->setDisplayBrightnessPercent(appliedBrightnessPercent_);
    }
    appliedTimeoutSeconds_ = settings.timeoutSeconds;
    if (board_->displayAwake() && appliedTimeoutSeconds_ != 0U &&
        nowMs - lastInteractionAtMs_ >=
            static_cast<std::uint32_t>(appliedTimeoutSeconds_) * 1'000U) {
      board_->setDisplayAwake(false);
    }
  }
  // Do not invalidate unrelated labels while the list is being dragged or
  // coasting. Sensor and scale services continue running; only their visual
  // refresh is deferred until the gesture finishes.
  if (!touchInteractionActive()) {
    screenManager_.update(nowMs);
  }
  if (static_cast<std::int32_t>(nowMs - nextHandlerAtMs_) < 0) {
    return;
  }
  const std::uint32_t startedAtUs = micros();
  const std::uint32_t requestedWaitMs = lv_timer_handler();
  if (displayFrameOpen_ && board_ != nullptr) {
    board_->endDisplayFrame();
    displayFrameOpen_ = false;
  }
  const std::uint32_t handlerDurationUs = micros() - startedAtUs;
  ++handlerCalls_;
  handlerTotalUs_ += handlerDurationUs;
  if (handlerDurationUs > handlerMaxUs_) {
    handlerMaxUs_ = handlerDurationUs;
  }
  const std::uint32_t handlerIntervalMs =
      std::min<std::uint32_t>(
          config::kLvglMaximumHandlerIntervalMs,
          std::max<std::uint32_t>(1U, requestedWaitMs));
  nextHandlerAtMs_ = nowMs + handlerIntervalMs;
  if (static_cast<std::int32_t>(nowMs - nextTelemetryAtMs_) >= 0) {
    logTelemetry(nowMs);
    nextTelemetryAtMs_ = nowMs + config::kUiTelemetryPeriodMs;
  }
}

void UIManager::wakeDisplay(const std::uint32_t nowMs) {
  if (board_ == nullptr) return;
  lastInteractionAtMs_ = nowMs;
  board_->setDisplayAwake(true);
}

void UIManager::flushDisplay(lv_display_t* const display,
                             const lv_area_t* const area,
                             std::uint8_t* const pixelMap) {
  if (instance_ == nullptr || instance_->board_ == nullptr) {
    lv_display_flush_ready(display);
    return;
  }

  const std::int32_t width = area->x2 - area->x1 + 1;
  const std::int32_t height = area->y2 - area->y1 + 1;
  const std::uint32_t startedAtUs = micros();
  if (!instance_->displayFrameOpen_) {
    instance_->board_->beginDisplayFrame(area->y1);
    instance_->displayFrameOpen_ = true;
  }
  instance_->board_->flushDisplay(
      area->x1, area->y1, width, height,
      reinterpret_cast<const std::uint16_t*>(pixelMap));
  const std::uint32_t durationUs = micros() - startedAtUs;
  ++instance_->flushCalls_;
  instance_->flushTotalUs_ += durationUs;
  if (durationUs > instance_->flushMaxUs_) {
    instance_->flushMaxUs_ = durationUs;
  }
  instance_->flushedPixels_ +=
      static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height);
  const bool lastFlush = lv_display_flush_is_last(display);
  if (lastFlush) {
    instance_->board_->endDisplayFrame();
    instance_->displayFrameOpen_ = false;
  } else {
    // A single LVGL draw buffer can be reused immediately after
    // lv_display_flush_ready(). Keep it immutable until DMA finishes.
    instance_->board_->waitDisplayTransfer();
  }
  lv_display_flush_ready(display);
}

void UIManager::readTouch(lv_indev_t* const inputDevice,
                          lv_indev_data_t* const data) {
  (void)inputDevice;
  if (instance_ == nullptr || instance_->board_ == nullptr) {
    data->state = LV_INDEV_STATE_RELEASED;
    return;
  }

  std::uint16_t x = 0U;
  std::uint16_t y = 0U;
  if (instance_->board_->readTouch(x, y)) {
    instance_->lastInteractionAtMs_ = millis();
    if (!instance_->board_->displayAwake()) {
      instance_->board_->setDisplayAwake(true);
      instance_->consumeWakeTouch_ = true;
    }
    if (instance_->consumeWakeTouch_) {
      data->state = LV_INDEV_STATE_RELEASED;
      return;
    }
    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = static_cast<std::int32_t>(x);
    data->point.y = static_cast<std::int32_t>(y);
  } else {
    instance_->consumeWakeTouch_ = false;
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void UIManager::handleDisplayRefresh(lv_event_t* const event) {
  auto* const self =
      static_cast<UIManager*>(lv_event_get_user_data(event));
  if (self != nullptr) {
    ++self->refreshCycles_;
  }
}

std::uint32_t UIManager::tickMs() { return millis(); }

bool UIManager::initializeLvgl() {
  lv_init();
  lv_tick_set_cb(tickMs);

  lvDisplay_ = lv_display_create(config::kDisplayWidth, config::kDisplayHeight);
  if (lvDisplay_ == nullptr) {
    return false;
  }
  lv_display_set_color_format(lvDisplay_, LV_COLOR_FORMAT_RGB565_SWAPPED);
  lv_display_set_buffers(lvDisplay_, drawBuffer1_, nullptr,
                         sizeof(drawBuffer1_), LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(lvDisplay_, flushDisplay);
  lv_display_add_event_cb(lvDisplay_, handleDisplayRefresh,
                          LV_EVENT_REFR_READY, this);

  lvTouch_ = lv_indev_create();
  if (lvTouch_ == nullptr) {
    return false;
  }
  lv_indev_set_type(lvTouch_, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(lvTouch_, readTouch);
  lv_indev_set_scroll_limit(lvTouch_, config::kTouchScrollLimitPixels);
  lv_indev_set_scroll_throw(lvTouch_, config::kTouchScrollThrowPercent);
  return true;
}

bool UIManager::createShell(app::AppViewModel& viewModel) {
  Theme::initialize();
  lv_obj_t* const screen = lv_screen_active();
  Theme::applyScreen(screen);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

  if (!header_.create(screen)) {
    return false;
  }

  contentRoot_ = lv_obj_create(screen);
  if (contentRoot_ == nullptr) {
    return false;
  }
  lv_obj_set_pos(contentRoot_, 0, layout::kContentY);
  lv_obj_set_size(contentRoot_, layout::kScreenWidth,
                  layout::kContentHeight);
  Theme::applyContent(contentRoot_);
  lv_obj_clear_flag(contentRoot_, LV_OBJ_FLAG_SCROLLABLE);

  return screenManager_.begin(contentRoot_, header_, navigation_, viewModel);
}

bool UIManager::touchInteractionActive() const {
  return lvTouch_ != nullptr &&
         (lv_indev_get_state(lvTouch_) == LV_INDEV_STATE_PRESSED ||
          lv_indev_get_scroll_obj(lvTouch_) != nullptr);
}

void UIManager::logTelemetry(const std::uint32_t nowMs) {
  const std::uint32_t elapsedMs = nowMs - telemetryWindowStartedAtMs_;
  const std::uint32_t averageHandlerUs =
      handlerCalls_ == 0U
          ? 0U
          : static_cast<std::uint32_t>(handlerTotalUs_ / handlerCalls_);
  const std::uint32_t fpsTimesTen =
      elapsedMs == 0U
          ? 0U
          : static_cast<std::uint32_t>(
                (static_cast<std::uint64_t>(refreshCycles_) * 10'000U) /
                elapsedMs);
  const std::uint32_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_8BIT);
  const std::uint32_t minimumHeap =
      heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT);
  const std::uint32_t largestBlock =
      heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  const std::uint32_t fragmentationPercent =
      freeHeap == 0U
          ? 100U
          : 100U - static_cast<std::uint32_t>(
                       (static_cast<std::uint64_t>(largestBlock) * 100U) /
                       freeHeap);
  const std::uint32_t averageFlushUs =
      flushCalls_ == 0U
          ? 0U
          : static_cast<std::uint32_t>(flushTotalUs_ / flushCalls_);
  KEEZER_LOG_INFO(
      kLogTag,
      "PERF_HEAP free=%lu minimum=%lu largest=%lu fragmentation=%lu%%",
      static_cast<unsigned long>(freeHeap),
      static_cast<unsigned long>(minimumHeap),
      static_cast<unsigned long>(largestBlock),
      static_cast<unsigned long>(fragmentationPercent));
  KEEZER_LOG_INFO(
      kLogTag,
      "PERF_UI fps=%lu.%lu handler_calls=%lu avg=%luus max=%luus flush_calls=%lu flush_avg=%luus flush_max=%luus pixels=%llu",
      static_cast<unsigned long>(fpsTimesTen / 10U),
      static_cast<unsigned long>(fpsTimesTen % 10U),
      static_cast<unsigned long>(handlerCalls_),
      static_cast<unsigned long>(averageHandlerUs),
      static_cast<unsigned long>(handlerMaxUs_),
      static_cast<unsigned long>(flushCalls_),
      static_cast<unsigned long>(averageFlushUs),
      static_cast<unsigned long>(flushMaxUs_),
      static_cast<unsigned long long>(flushedPixels_));

  telemetryWindowStartedAtMs_ = nowMs;
  handlerCalls_ = 0U;
  handlerTotalUs_ = 0U;
  handlerMaxUs_ = 0U;
  refreshCycles_ = 0U;
  flushCalls_ = 0U;
  flushTotalUs_ = 0U;
  flushMaxUs_ = 0U;
  flushedPixels_ = 0U;
}

}  // namespace keezer::ui
