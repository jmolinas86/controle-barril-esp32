#pragma once

#include <lvgl.h>

#include "app/ViewData.h"

namespace keezer::ui::screens {

class SettingsScreen final {
 public:
  using ActionHandler = void (*)(void* context);

  void create(lv_obj_t* parent, const app::ScaleViewData& scale,
              const app::NetworkViewData& network,
              const app::HistoryStorageViewData& history,
              ActionHandler archivesHandler, ActionHandler otaHandler,
              ActionHandler networkHandler, ActionHandler displayHandler,
              void* context);
  void update(const app::ScaleViewData& scale,
              const app::NetworkViewData& network,
              const app::HistoryStorageViewData& history);

 private:
  static void handleArchives(lv_event_t* event);
  static void handleOta(lv_event_t* event);
  static void handleNetwork(lv_event_t* event);
  static void handleDisplay(lv_event_t* event);

  lv_obj_t* scaleStatusLabel_{nullptr};
  lv_obj_t* scaleWeightLabel_{nullptr};
  lv_obj_t* scaleNfcLabel_{nullptr};
  lv_obj_t* networkStatusLabel_{nullptr};
  lv_obj_t* networkSignalLabel_{nullptr};
  lv_obj_t* otaStatusLabel_{nullptr};
  lv_obj_t* storageStatusLabel_{nullptr};
  lv_obj_t* storagePendingLabel_{nullptr};
  ActionHandler archivesHandler_{nullptr};
  ActionHandler otaHandler_{nullptr};
  ActionHandler networkHandler_{nullptr};
  ActionHandler displayHandler_{nullptr};
  void* context_{nullptr};
};

}  // namespace keezer::ui::screens
