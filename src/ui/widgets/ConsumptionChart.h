#pragma once

#include <lvgl.h>

#include "app/ViewData.h"

namespace keezer::ui::widgets {

class ConsumptionChart final {
 public:
  lv_obj_t* create(lv_obj_t* parent, const app::KegViewData& data);
  void update(const app::KegViewData& data);

 private:
  lv_obj_t* chart_{nullptr};
  lv_obj_t* averageLabel_{nullptr};
  lv_chart_series_t* series_{nullptr};
  std::array<std::int16_t, 7U> renderedHistory_{};
  std::uint8_t renderedPointCount_{0U};
  std::int16_t renderedRangeMaximum_{-1};
  float renderedAverage_{-1.0F};
};

}  // namespace keezer::ui::widgets
