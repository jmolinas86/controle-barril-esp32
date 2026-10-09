#pragma once

#include <lvgl.h>

#include <array>
#include <cstddef>

#include "app/AppViewModel.h"
#include "ui/widgets/ConsumptionChart.h"

namespace keezer::ui::screens {

class ArchivedKegsScreen final {
 public:
  using SelectionHandler = void (*)(std::size_t index, void* context);
  using ActionHandler = void (*)(void* context);

  void createList(lv_obj_t* parent, const app::AppViewModel& viewModel,
                  SelectionHandler selectionHandler, void* context);
  void createDetail(lv_obj_t* parent, const app::KegViewData& keg,
                    ActionHandler deleteHandler, void* context);
  void updateDetail(const app::KegViewData& keg);

 private:
  static void handleSelection(lv_event_t* event);
  static void handleDelete(lv_event_t* event);

  std::array<lv_obj_t*, app::kMaxKegs> cards_{};
  std::size_t cardCount_{0U};
  SelectionHandler selectionHandler_{nullptr};
  ActionHandler deleteHandler_{nullptr};
  void* context_{nullptr};
  widgets::ConsumptionChart chart_;
};

}  // namespace keezer::ui::screens
