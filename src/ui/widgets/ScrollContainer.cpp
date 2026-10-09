#include "ui/widgets/ScrollContainer.h"

#include "ui/Layout.h"
#include "ui/Theme.h"

namespace keezer::ui::widgets {

lv_obj_t* ScrollContainer::create(lv_obj_t* const parent) {
  lv_obj_t* const scroll = lv_obj_create(parent);
  lv_obj_set_pos(scroll, 0, 0);
  lv_obj_set_size(scroll, layout::kScreenWidth, layout::kContentHeight);
  Theme::applyScrollContainer(scroll);
  lv_obj_set_scroll_dir(scroll, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(scroll, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_set_flex_flow(scroll, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(scroll, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  return scroll;
}

}  // namespace keezer::ui::widgets
