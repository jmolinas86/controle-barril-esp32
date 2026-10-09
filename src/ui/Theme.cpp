#include "ui/Theme.h"

namespace keezer::ui {

bool Theme::initialized_ = false;
lv_style_t Theme::screenStyle_{};
lv_style_t Theme::contentStyle_{};
lv_style_t Theme::headerStyle_{};
lv_style_t Theme::cardStyle_{};
lv_style_t Theme::infoTileStyle_{};
lv_style_t Theme::navContainerStyle_{};
lv_style_t Theme::navButtonStyle_{};
lv_style_t Theme::navButtonActiveStyle_{};
lv_style_t Theme::actionButtonStyle_{};
lv_style_t Theme::actionButtonOutlineStyle_{};
lv_style_t Theme::scrollStyle_{};
lv_style_t Theme::scrollBarStyle_{};
lv_style_t Theme::overlayStyle_{};
lv_style_t Theme::modalStyle_{};

void Theme::initialize() {
  if (initialized_) {
    return;
  }

  lv_style_init(&screenStyle_);
  lv_style_set_bg_color(&screenStyle_, background());
  lv_style_set_bg_opa(&screenStyle_, LV_OPA_COVER);
  lv_style_set_text_color(&screenStyle_, text());
  lv_style_set_text_font(&screenStyle_, &lv_font_montserrat_14);
  lv_style_set_border_width(&screenStyle_, 0);
  lv_style_set_pad_all(&screenStyle_, 0);

  lv_style_init(&contentStyle_);
  lv_style_set_bg_color(&contentStyle_, background());
  lv_style_set_bg_opa(&contentStyle_, LV_OPA_COVER);
  lv_style_set_border_width(&contentStyle_, 0);
  lv_style_set_radius(&contentStyle_, 0);
  lv_style_set_pad_all(&contentStyle_, 0);

  lv_style_init(&headerStyle_);
  lv_style_set_bg_color(&headerStyle_, surface());
  lv_style_set_bg_opa(&headerStyle_, LV_OPA_COVER);
  lv_style_set_border_width(&headerStyle_, 0);
  lv_style_set_radius(&headerStyle_, 0);
  lv_style_set_pad_all(&headerStyle_, 0);

  lv_style_init(&cardStyle_);
  lv_style_set_bg_color(&cardStyle_, surface());
  lv_style_set_bg_opa(&cardStyle_, LV_OPA_COVER);
  lv_style_set_border_color(&cardStyle_, border());
  lv_style_set_border_width(&cardStyle_, 1);
  lv_style_set_radius(&cardStyle_, 5);
  lv_style_set_pad_all(&cardStyle_, 0);

  lv_style_init(&infoTileStyle_);
  lv_style_set_bg_color(&infoTileStyle_, surfaceRaised());
  lv_style_set_bg_opa(&infoTileStyle_, LV_OPA_COVER);
  lv_style_set_border_color(&infoTileStyle_, border());
  lv_style_set_border_width(&infoTileStyle_, 1);
  lv_style_set_radius(&infoTileStyle_, 5);
  lv_style_set_pad_all(&infoTileStyle_, 0);

  lv_style_init(&navContainerStyle_);
  lv_style_set_bg_color(&navContainerStyle_, surface());
  lv_style_set_bg_opa(&navContainerStyle_, LV_OPA_COVER);
  lv_style_set_border_color(&navContainerStyle_, border());
  lv_style_set_border_width(&navContainerStyle_, 1);
  lv_style_set_border_side(&navContainerStyle_, LV_BORDER_SIDE_TOP);
  lv_style_set_radius(&navContainerStyle_, 6);
  lv_style_set_pad_all(&navContainerStyle_, 0);

  lv_style_init(&navButtonStyle_);
  lv_style_set_bg_color(&navButtonStyle_, surface());
  lv_style_set_bg_opa(&navButtonStyle_, LV_OPA_COVER);
  lv_style_set_text_color(&navButtonStyle_, muted());
  lv_style_set_text_font(&navButtonStyle_, &lv_font_montserrat_12);
  lv_style_set_border_color(&navButtonStyle_, border());
  lv_style_set_border_width(&navButtonStyle_, 1);
  lv_style_set_border_side(&navButtonStyle_, LV_BORDER_SIDE_RIGHT);
  lv_style_set_radius(&navButtonStyle_, 5);
  lv_style_set_pad_all(&navButtonStyle_, 0);

  lv_style_init(&navButtonActiveStyle_);
  lv_style_set_bg_color(&navButtonActiveStyle_, primaryDark());
  lv_style_set_bg_grad_color(&navButtonActiveStyle_, LV_COLOR_MAKE(0, 30, 70));
  lv_style_set_bg_grad_dir(&navButtonActiveStyle_, LV_GRAD_DIR_VER);
  lv_style_set_text_color(&navButtonActiveStyle_, text());
  lv_style_set_border_color(&navButtonActiveStyle_, primary());
  lv_style_set_border_width(&navButtonActiveStyle_, 1);
  lv_style_set_border_side(&navButtonActiveStyle_, LV_BORDER_SIDE_FULL);

  lv_style_init(&actionButtonStyle_);
  lv_style_set_bg_color(&actionButtonStyle_, LV_COLOR_MAKE(0, 67, 145));
  lv_style_set_bg_grad_color(&actionButtonStyle_, LV_COLOR_MAKE(0, 27, 67));
  lv_style_set_bg_grad_dir(&actionButtonStyle_, LV_GRAD_DIR_VER);
  lv_style_set_bg_opa(&actionButtonStyle_, LV_OPA_COVER);
  lv_style_set_text_color(&actionButtonStyle_, text());
  lv_style_set_text_font(&actionButtonStyle_, &lv_font_montserrat_12);
  lv_style_set_border_color(&actionButtonStyle_, primary());
  lv_style_set_border_width(&actionButtonStyle_, 1);
  lv_style_set_radius(&actionButtonStyle_, 5);
  lv_style_set_pad_all(&actionButtonStyle_, 0);

  lv_style_init(&actionButtonOutlineStyle_);
  lv_style_set_bg_color(&actionButtonOutlineStyle_, surfaceRaised());
  lv_style_set_bg_opa(&actionButtonOutlineStyle_, LV_OPA_COVER);
  lv_style_set_text_color(&actionButtonOutlineStyle_, text());

  lv_style_init(&scrollStyle_);
  lv_style_set_bg_color(&scrollStyle_, background());
  lv_style_set_bg_opa(&scrollStyle_, LV_OPA_COVER);
  lv_style_set_border_width(&scrollStyle_, 0);
  lv_style_set_radius(&scrollStyle_, 0);
  // Keep the 228 px card width while shifting the card column 4 px left.
  lv_style_set_pad_left(&scrollStyle_, 1);
  lv_style_set_pad_right(&scrollStyle_, 11);
  lv_style_set_pad_top(&scrollStyle_, 3);
  lv_style_set_pad_bottom(&scrollStyle_, 4);
  lv_style_set_pad_row(&scrollStyle_, 4);

  lv_style_init(&scrollBarStyle_);
  lv_style_set_bg_color(&scrollBarStyle_, primary());
  lv_style_set_bg_opa(&scrollBarStyle_, LV_OPA_80);
  lv_style_set_width(&scrollBarStyle_, 3);
  lv_style_set_radius(&scrollBarStyle_, 2);

  lv_style_init(&overlayStyle_);
  lv_style_set_bg_color(&overlayStyle_, lv_color_black());
  lv_style_set_bg_opa(&overlayStyle_, LV_OPA_70);
  lv_style_set_border_width(&overlayStyle_, 0);
  lv_style_set_radius(&overlayStyle_, 0);
  lv_style_set_pad_all(&overlayStyle_, 0);

  lv_style_init(&modalStyle_);
  lv_style_set_bg_color(&modalStyle_, surfaceRaised());
  lv_style_set_bg_opa(&modalStyle_, LV_OPA_COVER);
  lv_style_set_border_color(&modalStyle_, primary());
  lv_style_set_border_width(&modalStyle_, 1);
  lv_style_set_radius(&modalStyle_, 8);
  lv_style_set_pad_all(&modalStyle_, 0);

  initialized_ = true;
}

void Theme::applyScreen(lv_obj_t* const object) {
  lv_obj_add_style(object, &screenStyle_, LV_PART_MAIN);
}

void Theme::applyContent(lv_obj_t* const object) {
  lv_obj_add_style(object, &contentStyle_, LV_PART_MAIN);
}

void Theme::applyHeader(lv_obj_t* const object) {
  lv_obj_add_style(object, &headerStyle_, LV_PART_MAIN);
}

void Theme::applyCard(lv_obj_t* const object) {
  lv_obj_add_style(object, &cardStyle_, LV_PART_MAIN);
}

void Theme::applyInfoTile(lv_obj_t* const object) {
  lv_obj_add_style(object, &infoTileStyle_, LV_PART_MAIN);
}

void Theme::applyNavContainer(lv_obj_t* const object) {
  lv_obj_add_style(object, &navContainerStyle_, LV_PART_MAIN);
}

void Theme::applyNavButton(lv_obj_t* const object, const bool active) {
  lv_obj_remove_style(object, &navButtonStyle_, LV_PART_MAIN);
  lv_obj_remove_style(object, &navButtonActiveStyle_, LV_PART_MAIN);
  lv_obj_add_style(object, &navButtonStyle_, LV_PART_MAIN);
  if (active) {
    lv_obj_add_style(object, &navButtonActiveStyle_, LV_PART_MAIN);
  }
}

void Theme::applyActionButton(lv_obj_t* const object, const bool filled) {
  lv_obj_add_style(object, &actionButtonStyle_, LV_PART_MAIN);
  if (!filled) {
    lv_obj_add_style(object, &actionButtonOutlineStyle_, LV_PART_MAIN);
  }
}

void Theme::applyScrollContainer(lv_obj_t* const object) {
  lv_obj_add_style(object, &scrollStyle_, LV_PART_MAIN);
  lv_obj_add_style(object, &scrollBarStyle_, LV_PART_SCROLLBAR);
}

void Theme::applyOverlay(lv_obj_t* const object) {
  lv_obj_add_style(object, &overlayStyle_, LV_PART_MAIN);
}

void Theme::applyModal(lv_obj_t* const object) {
  lv_obj_add_style(object, &modalStyle_, LV_PART_MAIN);
}

lv_color_t Theme::background() { return LV_COLOR_MAKE(0, 5, 10); }
lv_color_t Theme::surface() { return LV_COLOR_MAKE(1, 15, 27); }
lv_color_t Theme::surfaceRaised() { return LV_COLOR_MAKE(3, 22, 37); }
lv_color_t Theme::primary() { return LV_COLOR_MAKE(0, 174, 239); }
lv_color_t Theme::primaryDark() { return LV_COLOR_MAKE(0, 55, 120); }
lv_color_t Theme::border() { return LV_COLOR_MAKE(5, 67, 101); }
lv_color_t Theme::text() { return LV_COLOR_MAKE(238, 240, 242); }
lv_color_t Theme::muted() { return LV_COLOR_MAKE(155, 163, 171); }
lv_color_t Theme::good() { return LV_COLOR_MAKE(125, 211, 0); }
lv_color_t Theme::warning() { return LV_COLOR_MAKE(255, 166, 0); }
lv_color_t Theme::danger() { return LV_COLOR_MAKE(242, 63, 42); }

}  // namespace keezer::ui
