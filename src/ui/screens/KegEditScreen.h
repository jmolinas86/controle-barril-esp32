#pragma once

#include <lvgl.h>

#include <array>

#include "app/ViewData.h"
#include "models/Keg.h"
#include "services/KegService.h"

namespace keezer::ui::screens {

class KegEditScreen final {
 public:
  using ActionHandler = void (*)(void* context);

  void create(lv_obj_t* parent, const app::KegViewData* keg,
              const char* suggestedId, ActionHandler saveHandler,
              ActionHandler cancelHandler, void* context,
              bool focusNfc = false, const char* prefilledNfc = nullptr);
  app::KegEditRequest request() const;
  const char* originalId() const;
  bool editing() const;
  void showError(services::KegError error);

 private:
  static lv_obj_t* createField(lv_obj_t* parent, const char* label,
                               const char* value, std::uint32_t maximumLength,
                               const char* acceptedCharacters,
                               KegEditScreen* owner);
  static void handleFieldFocused(lv_event_t* event);
  static void handleKeyboard(lv_event_t* event);
  static void handleSave(lv_event_t* event);
  static void handleCancel(lv_event_t* event);

  void openKeyboard(lv_obj_t* field);
  void closeKeyboard();
  static float parseDecimal(const char* text);

  std::array<char, models::kKegIdBytes> originalId_{};
  bool editing_{false};
  lv_obj_t* scroll_{nullptr};
  lv_obj_t* keyboard_{nullptr};
  lv_obj_t* statusLabel_{nullptr};
  lv_obj_t* idField_{nullptr};
  lv_obj_t* nameField_{nullptr};
  lv_obj_t* styleField_{nullptr};
  lv_obj_t* batchField_{nullptr};
  lv_obj_t* filledDateField_{nullptr};
  lv_obj_t* capacityField_{nullptr};
  lv_obj_t* tareField_{nullptr};
  lv_obj_t* densityField_{nullptr};
  lv_obj_t* nfcField_{nullptr};
  lv_obj_t* notesField_{nullptr};
  ActionHandler saveHandler_{nullptr};
  ActionHandler cancelHandler_{nullptr};
  void* context_{nullptr};
};

}  // namespace keezer::ui::screens
