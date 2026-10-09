#include "ui/screens/HomeScreen.h"

#include "ui/Layout.h"

namespace keezer::ui::screens {

void HomeScreen::create(lv_obj_t* const parent,
                        const app::AppViewModel& viewModel,
                        const DetailsHandler detailsHandler,
                        const NewKegHandler newKegHandler,
                        void* const context) {
  // The keezer temperature is always visible. Only the keg list below it
  // scrolls, so the operator never loses sight of the control temperature.
  lv_obj_t* const freezerCard = freezerCard_.create(parent, viewModel.freezer());
  lv_obj_set_pos(freezerCard, 1, 3);

  constexpr std::int32_t kScrollableY = 68;
  kegListScreen_.create(parent, kScrollableY,
                        layout::kContentHeight - kScrollableY, viewModel,
                        detailsHandler, newKegHandler, context);
}

bool HomeScreen::refresh(const app::AppViewModel& viewModel) {
  freezerCard_.update(viewModel.freezer());
  return kegListScreen_.refresh(viewModel);
}

}  // namespace keezer::ui::screens
