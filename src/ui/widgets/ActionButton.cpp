#include "ui/widgets/ActionButton.h"

#include "ui/Theme.h"

namespace keezer::ui::widgets {

lv_obj_t* ActionButton::create(lv_obj_t* const parent, const char* const text,
                               const std::int32_t width,
                               const std::int32_t height,
                               const lv_event_cb_t handler,
                               void* const context, const bool filled) {
  lv_obj_t* const button = lv_button_create(parent);
  lv_obj_set_size(button, width, height);
  Theme::applyActionButton(button, filled);
  if (handler != nullptr) {
    lv_obj_add_event_cb(button, handler, LV_EVENT_CLICKED, context);
  }

  lv_obj_t* const label = lv_label_create(button);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_center(label);
  return button;
}

}  // namespace keezer::ui::widgets
