#pragma once

#include <lvgl.h>

#include <cstddef>

#include "app/AppViewModel.h"
#include "ui/widgets/FreezerCard.h"
#include "ui/screens/KegListScreen.h"

namespace keezer::ui::screens {

class HomeScreen final {
 public:
  using DetailsHandler = void (*)(std::size_t index, void* context);
  using NewKegHandler = void (*)(void* context);
  void create(lv_obj_t* parent, const app::AppViewModel& viewModel,
              DetailsHandler detailsHandler, NewKegHandler newKegHandler,
              void* context);
  bool refresh(const app::AppViewModel& viewModel);

 private:
  widgets::FreezerCard freezerCard_;
  KegListScreen kegListScreen_;
};

}  // namespace keezer::ui::screens
