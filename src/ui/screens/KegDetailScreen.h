#pragma once

#include <lvgl.h>

#include "app/ViewData.h"
#include "ui/widgets/ConsumptionChart.h"

namespace keezer::ui::screens {

class KegDetailScreen final {
 public:
  using ActionHandler = void (*)(void* context);

  void create(lv_obj_t* parent, const app::KegViewData& data,
              const app::ManualWeighingViewData& weighing,
              ActionHandler weighHandler, ActionHandler removeHandler,
              ActionHandler editHandler,
              ActionHandler nfcHandler,
              ActionHandler finishHandler, ActionHandler archiveHandler,
              void* context);
  void update(const app::KegViewData& data,
              const app::ManualWeighingViewData& weighing);

 private:
  static void handleWeigh(lv_event_t* event);
  static void handleRemove(lv_event_t* event);
  static void handleEdit(lv_event_t* event);
  static void handleNfc(lv_event_t* event);
  static void handleFinish(lv_event_t* event);
  static void handleArchive(lv_event_t* event);

  widgets::ConsumptionChart chart_;
  lv_obj_t* heroVolume_{nullptr};
  lv_obj_t* heroCapacity_{nullptr};
  lv_obj_t* heroProgress_{nullptr};
  lv_obj_t* tareValue_{nullptr};
  lv_obj_t* densityValue_{nullptr};
  lv_obj_t* volumeValue_{nullptr};
  lv_obj_t* weightValue_{nullptr};
  lv_obj_t* syncValue_{nullptr};
  lv_obj_t* capacityValue_{nullptr};
  lv_obj_t* nfcValue_{nullptr};
  lv_obj_t* statusValue_{nullptr};
  lv_obj_t* weighButton_{nullptr};
  lv_obj_t* weighButtonLabel_{nullptr};
  lv_obj_t* removeButton_{nullptr};
  ActionHandler weighHandler_{nullptr};
  ActionHandler removeHandler_{nullptr};
  ActionHandler editHandler_{nullptr};
  ActionHandler nfcHandler_{nullptr};
  ActionHandler finishHandler_{nullptr};
  ActionHandler archiveHandler_{nullptr};
  void* actionContext_{nullptr};
};

}  // namespace keezer::ui::screens
