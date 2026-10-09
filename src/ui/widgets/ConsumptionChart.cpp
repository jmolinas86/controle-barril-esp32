#include "ui/widgets/ConsumptionChart.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "ui/Layout.h"
#include "ui/Theme.h"
#include "ui/widgets/Card.h"

namespace keezer::ui::widgets {

lv_obj_t* ConsumptionChart::create(lv_obj_t* const parent,
                                   const app::KegViewData& data) {
  lv_obj_t* const card = Card::create(parent, 0, 0, layout::kCardWidth, 132);
  Card::addLabel(card, "HISTORICO - ULTIMAS PESAGENS", 7, 6, Theme::primary(),
                 &lv_font_montserrat_12);

  averageLabel_ = Card::addLabel(card, "MEDIA: -- L", 112, 21,
                                 Theme::muted(), &lv_font_montserrat_12);
  lv_obj_set_width(averageLabel_, 108);
  lv_obj_set_style_text_align(averageLabel_, LV_TEXT_ALIGN_RIGHT,
                              LV_PART_MAIN);

  chart_ = lv_chart_create(card);
  lv_obj_set_pos(chart_, 8, 40);
  lv_obj_set_size(chart_, 212, 66);
  lv_chart_set_type(chart_, LV_CHART_TYPE_LINE);
  lv_chart_set_point_count(chart_, data.historyDeciliters.size());
  lv_chart_set_range(chart_, LV_CHART_AXIS_PRIMARY_Y, 0, 200);
  lv_obj_set_style_bg_color(chart_, Theme::surfaceRaised(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(chart_, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_color(chart_, Theme::border(), LV_PART_MAIN);
  lv_obj_set_style_border_width(chart_, 1, LV_PART_MAIN);
  lv_obj_set_style_line_color(chart_, Theme::border(), LV_PART_MAIN);
  lv_obj_set_style_line_width(chart_, 1, LV_PART_MAIN);
  lv_obj_set_style_size(chart_, 4, 4, LV_PART_INDICATOR);
  lv_obj_set_style_line_width(chart_, 2, LV_PART_ITEMS);

  series_ =
      lv_chart_add_series(chart_, Theme::primary(), LV_CHART_AXIS_PRIMARY_Y);

  Card::addLabel(card, "ANTERIOR", 8, 110, Theme::muted(),
                 &lv_font_montserrat_12);
  Card::addLabel(card, "RECENTE", 174, 110, Theme::muted(),
                 &lv_font_montserrat_12);
  renderedAverage_ = -1.0F;
  renderedHistory_.fill(-1);
  renderedPointCount_ = 0U;
  update(data);
  return card;
}

void ConsumptionChart::update(const app::KegViewData& data) {
  if (chart_ == nullptr || series_ == nullptr || averageLabel_ == nullptr) {
    return;
  }

  if (std::fabs(renderedAverage_ - data.dailyAverageL) > 0.001F) {
    char average[24]{};
    std::snprintf(average, sizeof(average), "MEDIA: %.2f L",
                  static_cast<double>(data.dailyAverageL));
    if (std::strcmp(lv_label_get_text(averageLabel_), average) != 0) {
      lv_label_set_text(averageLabel_, average);
    }
    renderedAverage_ = data.dailyAverageL;
  }

  const std::int16_t rangeMaximum = static_cast<std::int16_t>(std::max(
      10.0F, std::ceil(data.capacityL) * 10.0F));
  if (renderedRangeMaximum_ != rangeMaximum) {
    lv_chart_set_range(chart_, LV_CHART_AXIS_PRIMARY_Y, 0, rangeMaximum);
    renderedRangeMaximum_ = rangeMaximum;
  }

  if (renderedHistory_ != data.historyDeciliters ||
      renderedPointCount_ != data.historyPointCount) {
    const std::size_t pointCount = std::min<std::size_t>(
        data.historyPointCount, data.historyDeciliters.size());
    const std::size_t firstVisible = data.historyDeciliters.size() - pointCount;
    for (std::size_t index = 0U; index < data.historyDeciliters.size();
         ++index) {
      lv_chart_set_series_value_by_id(chart_, series_, index,
                                      index < firstVisible
                                          ? LV_CHART_POINT_NONE
                                          : data.historyDeciliters[index]);
    }
    renderedHistory_ = data.historyDeciliters;
    renderedPointCount_ = data.historyPointCount;
    lv_chart_refresh(chart_);
  }
}

}  // namespace keezer::ui::widgets
