#include "ui/widgets/KegIcon.h"

#include "ui/assets/MiniBitmaps.h"

namespace keezer::ui::widgets {

lv_obj_t* KegIcon::create(lv_obj_t* const parent, const std::int32_t width,
                          const std::int32_t height) {
  const lv_image_dsc_t* const source =
      (width >= 50 || height >= 70) ? &assets::kKegLarge
                                    : &assets::kKegSmall;
  lv_obj_t* const icon = lv_image_create(parent);
  lv_image_set_src(icon, source);
  return icon;
}

}  // namespace keezer::ui::widgets
