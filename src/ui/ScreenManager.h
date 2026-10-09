#pragma once

#include <lvgl.h>

#include <array>
#include <cstddef>
#include <cstdint>

#include "app/AppViewModel.h"
#include "models/Keg.h"
#include "ui/Routes.h"
#include "ui/screens/HomeScreen.h"
#include "ui/screens/ArchivedKegsScreen.h"
#include "ui/screens/DisplaySettingsScreen.h"
#include "ui/screens/KegDetailScreen.h"
#include "ui/screens/KegEditScreen.h"
#include "ui/screens/NetworkSettingsScreen.h"
#include "ui/screens/SettingsScreen.h"
#include "ui/screens/TemperatureScreen.h"
#include "ui/widgets/Modal.h"

namespace keezer::ui::widgets {
class BottomNavigation;
class Header;
}

namespace keezer::ui {

class ScreenManager final {
 public:
  bool begin(lv_obj_t* contentRoot, widgets::Header& header,
             widgets::BottomNavigation& navigation,
             app::AppViewModel& viewModel);
  void navigate(Route route);
  void update(std::uint32_t nowMs);
  Route currentRoute() const;

 private:
  static void handleRouteRequest(Route route, void* context);
  static void handleKegDetails(std::size_t index, void* context);
  static void handleNewKeg(void* context);
  static void handleEditKeg(void* context);
  static void handleEditNfc(void* context);
  static void handleWeighRequest(void* context);
  static void handleWeighConfirmed(void* context);
  static void handleWeighCancelled(void* context);
  static void handleRemoveRequest(void* context);
  static void handleRemoveConfirmed(void* context);
  static void handleUnknownNfcRegister(void* context);
  static void handleUnknownNfcIgnore(void* context);
  static void handleSaveKeg(void* context);
  static void handleCancelEdit(void* context);
  static void handleFinishRequest(void* context);
  static void handleArchiveRequest(void* context);
  static void handleFinishConfirmed(void* context);
  static void handleArchiveConfirmed(void* context);
  static void handleOpenNetwork(void* context);
  static void handleOpenArchived(void* context);
  static void handleArchivedSelection(std::size_t index, void* context);
  static void handleArchivedDeleteRequest(void* context);
  static void handleArchivedDeleteConfirmed(void* context);
  static void handleOpenDisplay(void* context);
  static void handleShowOta(void* context);
  static app::DisplaySaveResult handleSaveDisplay(
      std::uint8_t brightness, std::uint16_t timeoutSeconds, void* context);
  static void handleSaveNetwork(void* context);
  static void handleCancelNetwork(void* context);
  static app::TemperatureSetpointResult handleTemperatureSetpoint(
      std::int16_t setpointCentiCelsius, void* context);
  static void handleBack(void* context);

  void openKeg(std::size_t index);
  void openKegEditor(bool focusNfc);
  const app::KegViewData* selectedKeg() const;
  const app::KegViewData* selectedArchivedKeg() const;
  void configureHeader();
  void renderCurrent();
  void showWorkflowPrompts();

  lv_obj_t* contentRoot_{nullptr};
  widgets::Header* header_{nullptr};
  widgets::BottomNavigation* navigation_{nullptr};
  app::AppViewModel* viewModel_{nullptr};
  Route currentRoute_{Route::Home};
  std::array<char, models::kKegIdBytes> selectedKegId_{};
  std::array<char, models::kNfcUidBytes> pendingNfcPrefill_{};
  bool creatingKeg_{false};
  bool focusNfcOnEdit_{false};
  std::uint32_t nextViewRefreshAtMs_{0U};
  std::uint32_t lastViewRevision_{0U};
  std::uint32_t lastManualPromptRevision_{0U};
  std::uint32_t lastUnknownPromptRevision_{0U};
  bool ready_{false};

  screens::HomeScreen homeScreen_;
  screens::KegDetailScreen kegDetailScreen_;
  screens::KegEditScreen kegEditScreen_;
  screens::TemperatureScreen temperatureScreen_;
  screens::SettingsScreen settingsScreen_;
  screens::NetworkSettingsScreen networkSettingsScreen_;
  screens::ArchivedKegsScreen archivedKegsScreen_;
  screens::DisplaySettingsScreen displaySettingsScreen_;
  widgets::Modal actionModal_;
};

}  // namespace keezer::ui
