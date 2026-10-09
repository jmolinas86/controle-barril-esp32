#pragma once

#include <lvgl.h>

#include <array>

#include "ui/Routes.h"

namespace keezer::ui::widgets {

class BottomNavigation final {
 public:
  using RouteHandler = void (*)(Route route, void* context);

  bool create(lv_obj_t* parent, RouteHandler handler, void* context);
  void setActive(Route route);

 private:
  static void handleClick(lv_event_t* event);

  std::array<lv_obj_t*, 3U> buttons_{};
  std::array<lv_obj_t*, 3U> iconLabels_{};
  std::array<lv_obj_t*, 3U> textLabels_{};
  RouteHandler routeHandler_{nullptr};
  void* routeContext_{nullptr};
};

}  // namespace keezer::ui::widgets
