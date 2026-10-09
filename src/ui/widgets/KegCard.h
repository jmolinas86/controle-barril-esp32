#pragma once

#include <lvgl.h>

#include <cstddef>

#include "app/ViewData.h"

namespace keezer::ui::widgets {

class KegCard final {
 public:
  using DetailsHandler = void (*)(std::size_t index, void* context);

  lv_obj_t* create(lv_obj_t* parent, const app::KegViewData& data,
                   std::size_t index, DetailsHandler handler, void* context);
  void update(const app::KegViewData& data);

 private:
  static void handleDetails(lv_event_t* event);

  std::size_t index_{0U};
  DetailsHandler detailsHandler_{nullptr};
  void* detailsContext_{nullptr};
  lv_obj_t* numberLabel_{nullptr};
  lv_obj_t* nameLabel_{nullptr};
  lv_obj_t* volumeLabel_{nullptr};
  lv_obj_t* progress_{nullptr};
  lv_obj_t* percentageLabel_{nullptr};
  lv_obj_t* syncLabel_{nullptr};
  lv_obj_t* statusLabel_{nullptr};
};

}  // namespace keezer::ui::widgets
