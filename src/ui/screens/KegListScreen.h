#pragma once

#include <lvgl.h>

#include <array>
#include <cstddef>
#include <cstdint>

#include "app/AppViewModel.h"
#include "ui/widgets/KegCard.h"

namespace keezer::ui::screens {

class KegListScreen final {
 public:
  using DetailsHandler = void (*)(std::size_t index, void* context);
  using NewKegHandler = void (*)(void* context);
  void create(lv_obj_t* parent, std::int32_t y, std::int32_t height,
              const app::AppViewModel& viewModel,
              DetailsHandler detailsHandler, NewKegHandler newKegHandler,
              void* context);
  bool refresh(const app::AppViewModel& viewModel);

 private:
  static void handleNewKeg(lv_event_t* event);

  std::array<widgets::KegCard, app::kMaxKegs> kegCards_{};
  std::array<std::size_t, app::kMaxKegs> renderedSourceIndices_{};
  std::size_t renderedKegCount_{0U};
  NewKegHandler newKegHandler_{nullptr};
  void* context_{nullptr};
};

}  // namespace keezer::ui::screens
