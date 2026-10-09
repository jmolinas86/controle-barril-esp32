#include "ui/screens/KegListScreen.h"

#include <algorithm>

#include "ui/Layout.h"
#include "ui/Theme.h"
#include "ui/widgets/ActionButton.h"
#include "ui/widgets/Card.h"
#include "ui/widgets/ScrollContainer.h"

namespace keezer::ui::screens {
namespace {

bool showOnHome(const app::KegViewData* const keg) {
  // Archived records remain persisted but hidden. Finished is accepted here
  // only for compatibility with records created by older firmware; pressing
  // FINALIZAR again deletes such a legacy record permanently.
  return keg != nullptr && keg->status != models::KegStatus::Archived;
}

}  // namespace

void KegListScreen::create(lv_obj_t* const parent, const std::int32_t y,
                           const std::int32_t height,
                           const app::AppViewModel& viewModel,
                           const DetailsHandler detailsHandler,
                           const NewKegHandler newKegHandler,
                           void* const context) {
  newKegHandler_ = newKegHandler;
  context_ = context;

  lv_obj_t* const scroll = widgets::ScrollContainer::create(parent);
  lv_obj_set_pos(scroll, 0, y);
  lv_obj_set_size(scroll, layout::kScreenWidth, height);

  renderedKegCount_ = 0U;
  for (std::size_t sourceIndex = 0U;
       sourceIndex < viewModel.kegCount() &&
       renderedKegCount_ < kegCards_.size();
       ++sourceIndex) {
    const app::KegViewData* const keg = viewModel.kegAt(sourceIndex);
    if (!showOnHome(keg)) {
      continue;
    }
    renderedSourceIndices_[renderedKegCount_] = sourceIndex;
    kegCards_[renderedKegCount_].create(scroll, *keg, sourceIndex,
                                        detailsHandler, context);
    ++renderedKegCount_;
  }

  if (renderedKegCount_ == 0U) {
    lv_obj_t* const empty =
        widgets::Card::create(scroll, 0, 0, layout::kCardWidth, 86);
    lv_obj_t* const title = widgets::Card::addLabel(
        empty, "NENHUM KEG", 0, 17, Theme::text(), &lv_font_montserrat_16);
    lv_obj_set_width(title, layout::kCardWidth);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_t* const help = widgets::Card::addLabel(
        empty, "Cadastre o primeiro keg", 0, 47, Theme::muted(),
        &lv_font_montserrat_12);
    lv_obj_set_width(help, layout::kCardWidth);
    lv_obj_set_style_text_align(help, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  }

  widgets::ActionButton::create(scroll, LV_SYMBOL_PLUS "  NOVO KEG",
                                layout::kCardWidth, 36, handleNewKeg, this,
                                true);
}

bool KegListScreen::refresh(const app::AppViewModel& viewModel) {
  std::array<std::size_t, app::kMaxKegs> sourceIndices{};
  std::size_t count = 0U;
  for (std::size_t sourceIndex = 0U;
       sourceIndex < viewModel.kegCount() && count < sourceIndices.size();
       ++sourceIndex) {
    if (showOnHome(viewModel.kegAt(sourceIndex))) {
      sourceIndices[count++] = sourceIndex;
    }
  }
  if (count != renderedKegCount_) {
    return false;
  }
  for (std::size_t index = 0U; index < count; ++index) {
    if (sourceIndices[index] != renderedSourceIndices_[index]) {
      return false;
    }
    const app::KegViewData* const keg = viewModel.kegAt(sourceIndices[index]);
    if (keg != nullptr) {
      kegCards_[index].update(*keg);
    }
  }
  return true;
}

void KegListScreen::handleNewKeg(lv_event_t* const event) {
  auto* const self =
      static_cast<KegListScreen*>(lv_event_get_user_data(event));
  if (self != nullptr && self->newKegHandler_ != nullptr) {
    self->newKegHandler_(self->context_);
  }
}

}  // namespace keezer::ui::screens
