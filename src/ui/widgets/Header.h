#pragma once

#include <lvgl.h>

#include <cstddef>
#include <cstdint>

#include "app/ViewData.h"

namespace keezer::ui::widgets {

class Header final {
 public:
  using BackHandler = void (*)(void* context);

  bool create(lv_obj_t* parent);
  void setBackHandler(BackHandler handler, void* context);
  void showHome(std::size_t kegCount);
  void showTitle(const char* title, bool showBack);
  void setNetworkState(const app::NetworkViewData& network);

 private:
  static void handleBack(lv_event_t* event);

  lv_obj_t* titleLabel_{nullptr};
  lv_obj_t* backButton_{nullptr};
  lv_obj_t* homeIcon_{nullptr};
  lv_obj_t* connectivityLabel_{nullptr};
  BackHandler backHandler_{nullptr};
  void* backContext_{nullptr};
};

}  // namespace keezer::ui::widgets
