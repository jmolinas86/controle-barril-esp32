#pragma once

#include <lvgl.h>

#include "app/ViewData.h"

namespace keezer::ui::widgets {

class FreezerCard final {
 public:
  lv_obj_t* create(lv_obj_t* parent, const app::FreezerViewData& data);
  void update(const app::FreezerViewData& data);

 private:
  lv_obj_t* temperatureValue_{nullptr};
  lv_obj_t* setpointValue_{nullptr};
  lv_obj_t* stateLabel_{nullptr};
  lv_obj_t* stateIcon_{nullptr};
};

}  // namespace keezer::ui::widgets
