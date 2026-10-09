#pragma once

#include <lvgl.h>

namespace keezer::ui {

class Theme final {
 public:
  static void initialize();

  static void applyScreen(lv_obj_t* object);
  static void applyContent(lv_obj_t* object);
  static void applyHeader(lv_obj_t* object);
  static void applyCard(lv_obj_t* object);
  static void applyInfoTile(lv_obj_t* object);
  static void applyNavContainer(lv_obj_t* object);
  static void applyNavButton(lv_obj_t* object, bool active);
  static void applyActionButton(lv_obj_t* object, bool filled = true);
  static void applyScrollContainer(lv_obj_t* object);
  static void applyOverlay(lv_obj_t* object);
  static void applyModal(lv_obj_t* object);

  static lv_color_t background();
  static lv_color_t surface();
  static lv_color_t surfaceRaised();
  static lv_color_t primary();
  static lv_color_t primaryDark();
  static lv_color_t border();
  static lv_color_t text();
  static lv_color_t muted();
  static lv_color_t good();
  static lv_color_t warning();
  static lv_color_t danger();

 private:
  static bool initialized_;
  static lv_style_t screenStyle_;
  static lv_style_t contentStyle_;
  static lv_style_t headerStyle_;
  static lv_style_t cardStyle_;
  static lv_style_t infoTileStyle_;
  static lv_style_t navContainerStyle_;
  static lv_style_t navButtonStyle_;
  static lv_style_t navButtonActiveStyle_;
  static lv_style_t actionButtonStyle_;
  static lv_style_t actionButtonOutlineStyle_;
  static lv_style_t scrollStyle_;
  static lv_style_t scrollBarStyle_;
  static lv_style_t overlayStyle_;
  static lv_style_t modalStyle_;
};

}  // namespace keezer::ui
