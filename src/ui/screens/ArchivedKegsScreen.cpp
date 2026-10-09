#include "ui/screens/ArchivedKegsScreen.h"

#include <cstdio>

#include "ui/Layout.h"
#include "ui/Theme.h"
#include "ui/widgets/ActionButton.h"
#include "ui/widgets/Card.h"
#include "ui/widgets/ScrollContainer.h"

namespace keezer::ui::screens {

void ArchivedKegsScreen::createList(
    lv_obj_t* const parent, const app::AppViewModel& viewModel,
    const SelectionHandler selectionHandler, void* const context) {
  cards_.fill(nullptr);
  cardCount_ = viewModel.archivedKegCount();
  selectionHandler_ = selectionHandler;
  context_ = context;

  char countText[28]{};
  std::snprintf(countText, sizeof(countText), "%u KEG(S) ARQUIVADO(S)",
                static_cast<unsigned int>(cardCount_));
  widgets::Card::addLabel(parent, countText, 7, 7, Theme::primary(),
                          &lv_font_montserrat_12);

  lv_obj_t* scroll = widgets::ScrollContainer::create(parent);
  lv_obj_set_pos(scroll, 0, 28);
  lv_obj_set_size(scroll, layout::kScreenWidth, 222);
  if (cardCount_ == 0U) {
    lv_obj_t* empty = widgets::Card::create(scroll, 0, 0, 228, 70);
    lv_obj_t* title = widgets::Card::addLabel(
        empty, "NENHUM KEG ARQUIVADO", 0, 17, Theme::text(),
        &lv_font_montserrat_12);
    lv_obj_set_width(title, 228);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    widgets::Card::addLabel(empty, "Use ARQUIVAR nos detalhes", 34, 39,
                            Theme::muted(), &lv_font_montserrat_10);
    return;
  }

  for (std::size_t index = 0U; index < cardCount_; ++index) {
    const app::KegViewData* keg = viewModel.archivedKegAt(index);
    if (keg == nullptr) continue;
    lv_obj_t* card = widgets::Card::create(scroll, 0, 0, 228, 62);
    cards_[index] = card;
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(card, handleSelection, LV_EVENT_CLICKED, this);
    char title[42]{};
    std::snprintf(title, sizeof(title), "%s", keg->name == nullptr ? "KEG" : keg->name);
    widgets::Card::addLabel(card, title, 8, 7, Theme::text(),
                            &lv_font_montserrat_14);
    widgets::Card::addLabel(card, keg->id == nullptr ? "--" : keg->id, 8, 29,
                            Theme::muted(), &lv_font_montserrat_10);
    char volume[20]{};
    std::snprintf(volume, sizeof(volume), "%.1f L",
                  static_cast<double>(keg->volumeL));
    widgets::Card::addLabel(card, volume, 151, 12, Theme::primary(),
                            &lv_font_montserrat_12);
    widgets::Card::addLabel(card, "HISTORICO  " LV_SYMBOL_RIGHT, 125, 36,
                            Theme::muted(), &lv_font_montserrat_10);
  }
}

void ArchivedKegsScreen::createDetail(lv_obj_t* const parent,
                                      const app::KegViewData& keg,
                                      const ActionHandler deleteHandler,
                                      void* const context) {
  deleteHandler_ = deleteHandler;
  context_ = context;
  lv_obj_t* summary = widgets::Card::create(parent, 6, 7, 228, 70);
  widgets::Card::addLabel(summary, keg.name == nullptr ? "KEG" : keg.name,
                          9, 8, Theme::text(), &lv_font_montserrat_16);
  widgets::Card::addLabel(summary, keg.id == nullptr ? "--" : keg.id, 9, 32,
                          Theme::muted(), &lv_font_montserrat_10);
  char metrics[40]{};
  std::snprintf(metrics, sizeof(metrics), "%.1f L  |  %.2f kg",
                static_cast<double>(keg.volumeL),
                static_cast<double>(keg.weightKg));
  widgets::Card::addLabel(summary, metrics, 95, 38, Theme::primary(),
                          &lv_font_montserrat_12);

  lv_obj_t* chart = chart_.create(parent, keg);
  lv_obj_set_pos(chart, 6, 82);
  lv_obj_t* remove = widgets::ActionButton::create(
      parent, LV_SYMBOL_TRASH "  APAGAR DEFINITIVO", 228, 28, handleDelete,
      this, false);
  lv_obj_set_pos(remove, 6, 219);
  lv_obj_set_style_text_color(remove, Theme::danger(), LV_PART_MAIN);
  lv_obj_set_style_border_color(remove, Theme::danger(), LV_PART_MAIN);
}

void ArchivedKegsScreen::updateDetail(const app::KegViewData& keg) {
  chart_.update(keg);
}

void ArchivedKegsScreen::handleSelection(lv_event_t* const event) {
  auto* self = static_cast<ArchivedKegsScreen*>(lv_event_get_user_data(event));
  if (self == nullptr || self->selectionHandler_ == nullptr) return;
  lv_obj_t* target = static_cast<lv_obj_t*>(lv_event_get_target(event));
  for (std::size_t index = 0U; index < self->cardCount_; ++index) {
    if (self->cards_[index] == target) {
      self->selectionHandler_(index, self->context_);
      return;
    }
  }
}

void ArchivedKegsScreen::handleDelete(lv_event_t* const event) {
  auto* self = static_cast<ArchivedKegsScreen*>(lv_event_get_user_data(event));
  if (self != nullptr && self->deleteHandler_ != nullptr) {
    self->deleteHandler_(self->context_);
  }
}

}  // namespace keezer::ui::screens
