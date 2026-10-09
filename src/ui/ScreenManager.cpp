#include "ui/ScreenManager.h"

#include <cstdio>
#include <cstring>

#include "BuildConfig.h"
#include "diagnostics/Logger.h"
#include "ui/widgets/BottomNavigation.h"
#include "ui/widgets/Header.h"

namespace keezer::ui {
namespace {

constexpr char kLogTag[] = "ROUTER";

}  // namespace

bool ScreenManager::begin(lv_obj_t* const contentRoot,
                          widgets::Header& header,
                          widgets::BottomNavigation& navigation,
                          app::AppViewModel& viewModel) {
  contentRoot_ = contentRoot;
  header_ = &header;
  navigation_ = &navigation;
  viewModel_ = &viewModel;
  ready_ = contentRoot_ != nullptr;
  if (!ready_) {
    return false;
  }

  header_->setBackHandler(handleBack, this);
  if (!navigation_->create(lv_screen_active(), handleRouteRequest, this)) {
    ready_ = false;
    return false;
  }
  navigate(Route::Home);
  return true;
}

void ScreenManager::navigate(const Route route) {
  if (!ready_ || viewModel_ == nullptr) {
    return;
  }
  currentRoute_ = route;
  configureHeader();
  navigation_->setActive(route);
  renderCurrent();
  lastViewRevision_ = viewModel_->revision();
  nextViewRefreshAtMs_ = 0U;
  KEEZER_LOG_INFO(kLogTag, "Route changed to %s", routeTitle(route));
  showWorkflowPrompts();
}

void ScreenManager::update(const std::uint32_t nowMs) {
  if (!ready_ || viewModel_ == nullptr ||
      static_cast<std::int32_t>(nowMs - nextViewRefreshAtMs_) < 0) {
    return;
  }
  nextViewRefreshAtMs_ = nowMs + config::kUiDataRefreshPeriodMs;
  header_->setNetworkState(viewModel_->network());
  if (lastViewRevision_ == viewModel_->revision()) {
    return;
  }

  switch (currentRoute_) {
    case Route::Home:
      if (!homeScreen_.refresh(*viewModel_)) {
        renderCurrent();
      }
      break;
    case Route::KegDetail: {
      const app::KegViewData* const keg = selectedKeg();
      if (keg == nullptr) {
        navigate(Route::Home);
        return;
      }
      kegDetailScreen_.update(*keg, viewModel_->manualWeighing());
      break;
    }
    case Route::KegEdit:
      break;
    case Route::Temperature:
      temperatureScreen_.update(viewModel_->freezer());
      break;
    case Route::Settings:
      settingsScreen_.update(viewModel_->scale(), viewModel_->network(),
                             viewModel_->historyStorage());
      break;
    case Route::NetworkSettings:
      networkSettingsScreen_.update(viewModel_->network());
      break;
    case Route::ArchivedKegs:
      break;
    case Route::ArchivedKegDetail: {
      const app::KegViewData* keg = selectedArchivedKeg();
      if (keg == nullptr) {
        navigate(Route::ArchivedKegs);
        return;
      }
      archivedKegsScreen_.updateDetail(*keg);
      break;
    }
    case Route::DisplaySettings:
      break;
  }
  lastViewRevision_ = viewModel_->revision();
  showWorkflowPrompts();
}

Route ScreenManager::currentRoute() const { return currentRoute_; }

void ScreenManager::handleRouteRequest(const Route route,
                                       void* const context) {
  auto* const self = static_cast<ScreenManager*>(context);
  if (self != nullptr && route != self->currentRoute_) {
    self->navigate(route);
  }
}

void ScreenManager::handleKegDetails(const std::size_t index,
                                     void* const context) {
  auto* const self = static_cast<ScreenManager*>(context);
  if (self != nullptr) {
    self->openKeg(index);
  }
}

void ScreenManager::handleNewKeg(void* const context) {
  auto* const self = static_cast<ScreenManager*>(context);
  if (self == nullptr) {
    return;
  }
  self->selectedKegId_ = {};
  self->pendingNfcPrefill_ = {};
  self->creatingKeg_ = true;
  self->focusNfcOnEdit_ = false;
  self->navigate(Route::KegEdit);
}

void ScreenManager::handleEditKeg(void* const context) {
  auto* const self = static_cast<ScreenManager*>(context);
  if (self != nullptr) {
    self->openKegEditor(false);
  }
}

void ScreenManager::handleEditNfc(void* const context) {
  auto* const self = static_cast<ScreenManager*>(context);
  if (self != nullptr) {
    self->openKegEditor(true);
  }
}

void ScreenManager::handleWeighRequest(void* const context) {
  auto* const self = static_cast<ScreenManager*>(context);
  if (self == nullptr || self->viewModel_ == nullptr ||
      self->selectedKeg() == nullptr) {
    return;
  }
  const app::ManualWeighingViewData& session =
      self->viewModel_->manualWeighing();
  const bool sameKeg = session.kegId[0] != '\0' &&
                       std::strcmp(session.kegId.data(),
                                   self->selectedKegId_.data()) == 0;
  if (sameKeg && self->viewModel_->manualWeighingActive()) {
    self->viewModel_->cancelManualWeighing();
    KEEZER_LOG_INFO(kLogTag, "Manual weighing cancelled by user");
    return;
  }
  const services::KegError result = self->viewModel_->beginManualWeighing(
      self->selectedKegId_.data());
  if (result != services::KegError::None) {
    KEEZER_LOG_WARN(kLogTag, "Could not start manual weighing: %s",
                    services::kegErrorName(result));
  } else {
    KEEZER_LOG_INFO(kLogTag, "Manual weighing started: keg=%s",
                    self->selectedKegId_.data());
  }
}

void ScreenManager::handleWeighConfirmed(void* const context) {
  auto* const self = static_cast<ScreenManager*>(context);
  if (self == nullptr || self->viewModel_ == nullptr) {
    return;
  }
  const services::KegError result =
      self->viewModel_->confirmManualWeighing();
  if (result != services::KegError::None) {
    KEEZER_LOG_WARN(kLogTag, "Could not save manual weighing: %s",
                    services::kegErrorName(result));
  } else {
    KEEZER_LOG_INFO(kLogTag, "Manual weighing confirmed");
  }
}

void ScreenManager::handleWeighCancelled(void* const context) {
  auto* const self = static_cast<ScreenManager*>(context);
  if (self != nullptr && self->viewModel_ != nullptr) {
    self->viewModel_->cancelManualWeighing();
  }
}

void ScreenManager::handleRemoveRequest(void* const context) {
  auto* const self = static_cast<ScreenManager*>(context);
  const app::KegViewData* const keg =
      self == nullptr ? nullptr : self->selectedKeg();
  if (self == nullptr || keg == nullptr || !keg->onScale ||
      self->contentRoot_ == nullptr) {
    return;
  }
  self->actionModal_.showConfirm(
      self->contentRoot_, "RETIRAR KEG?",
      "Confirma que este KEG saiu da balanca?",
      handleRemoveConfirmed, self, "RETIRAR");
}

void ScreenManager::handleRemoveConfirmed(void* const context) {
  auto* const self = static_cast<ScreenManager*>(context);
  if (self == nullptr || self->viewModel_ == nullptr) {
    return;
  }
  const services::KegError result = self->viewModel_->removeActiveKeg();
  if (result == services::KegError::None) {
    KEEZER_LOG_INFO(kLogTag, "KEG removed manually from scale");
  } else {
    KEEZER_LOG_WARN(kLogTag, "Manual removal failed: %s",
                    services::kegErrorName(result));
  }
}

void ScreenManager::handleUnknownNfcRegister(void* const context) {
  auto* const self = static_cast<ScreenManager*>(context);
  if (self == nullptr || self->viewModel_ == nullptr) {
    return;
  }
  const app::PendingUnknownNfcViewData& unknown =
      self->viewModel_->unknownNfc();
  std::snprintf(self->pendingNfcPrefill_.data(),
                self->pendingNfcPrefill_.size(), "%s", unknown.uid.data());
  self->viewModel_->clearUnknownNfc();
  self->selectedKegId_ = {};
  self->creatingKeg_ = true;
  self->focusNfcOnEdit_ = true;
  self->navigate(Route::KegEdit);
}

void ScreenManager::handleUnknownNfcIgnore(void* const context) {
  auto* const self = static_cast<ScreenManager*>(context);
  if (self != nullptr && self->viewModel_ != nullptr) {
    self->viewModel_->clearUnknownNfc();
  }
}

void ScreenManager::handleSaveKeg(void* const context) {
  auto* const self = static_cast<ScreenManager*>(context);
  if (self == nullptr || self->viewModel_ == nullptr) {
    return;
  }
  const app::KegEditRequest request = self->kegEditScreen_.request();
  const services::KegError result =
      self->kegEditScreen_.editing()
          ? self->viewModel_->updateKeg(self->kegEditScreen_.originalId(),
                                        request)
          : self->viewModel_->createKeg(request);
  if (result != services::KegError::None) {
    KEEZER_LOG_WARN(kLogTag, "Could not save keg: %s",
                    services::kegErrorName(result));
    self->kegEditScreen_.showError(result);
    return;
  }
  std::snprintf(self->selectedKegId_.data(), self->selectedKegId_.size(),
                "%s", request.id == nullptr ? "" : request.id);
  self->creatingKeg_ = false;
  self->focusNfcOnEdit_ = false;
  self->pendingNfcPrefill_ = {};
  self->navigate(Route::KegDetail);
}

void ScreenManager::handleCancelEdit(void* const context) {
  auto* const self = static_cast<ScreenManager*>(context);
  if (self == nullptr) {
    return;
  }
  const bool creating = self->creatingKeg_;
  self->pendingNfcPrefill_ = {};
  self->navigate(creating ? Route::Home : Route::KegDetail);
}

void ScreenManager::handleFinishRequest(void* const context) {
  auto* const self = static_cast<ScreenManager*>(context);
  if (self != nullptr && self->contentRoot_ != nullptr) {
    self->actionModal_.showConfirm(
        self->contentRoot_, "APAGAR KEG?",
        "ATENCAO: apaga permanentemente todo o registro. Nao pode ser desfeito.",
        handleFinishConfirmed, self);
  }
}

void ScreenManager::handleArchiveRequest(void* const context) {
  auto* const self = static_cast<ScreenManager*>(context);
  if (self != nullptr && self->contentRoot_ != nullptr) {
    self->actionModal_.showConfirm(
        self->contentRoot_, "ARQUIVAR KEG",
        "Oculta este keg da lista principal sem apagar seus dados.",
        handleArchiveConfirmed, self);
  }
}

void ScreenManager::handleFinishConfirmed(void* const context) {
  auto* const self = static_cast<ScreenManager*>(context);
  if (self == nullptr || self->viewModel_ == nullptr) {
    return;
  }
  const services::KegError result =
      self->viewModel_->deleteKeg(self->selectedKegId_.data());
  if (result == services::KegError::None) {
    self->selectedKegId_ = {};
    self->navigate(Route::Home);
  } else {
    KEEZER_LOG_WARN(kLogTag, "Could not delete finalized keg: %s",
                    services::kegErrorName(result));
  }
}

void ScreenManager::handleArchiveConfirmed(void* const context) {
  auto* const self = static_cast<ScreenManager*>(context);
  if (self == nullptr || self->viewModel_ == nullptr) {
    return;
  }
  const services::KegError result =
      self->viewModel_->archiveKeg(self->selectedKegId_.data());
  if (result == services::KegError::None) {
    self->selectedKegId_ = {};
    self->navigate(Route::Home);
  } else {
    KEEZER_LOG_WARN(kLogTag, "Could not archive keg: %s",
                    services::kegErrorName(result));
  }
}

void ScreenManager::handleOpenNetwork(void* const context) {
  auto* const self = static_cast<ScreenManager*>(context);
  if (self != nullptr) self->navigate(Route::NetworkSettings);
}

void ScreenManager::handleOpenArchived(void* const context) {
  auto* self = static_cast<ScreenManager*>(context);
  if (self != nullptr) self->navigate(Route::ArchivedKegs);
}

void ScreenManager::handleArchivedSelection(const std::size_t index,
                                            void* const context) {
  auto* self = static_cast<ScreenManager*>(context);
  if (self == nullptr || self->viewModel_ == nullptr) return;
  const app::KegViewData* keg = self->viewModel_->archivedKegAt(index);
  if (keg == nullptr || keg->id == nullptr) return;
  std::snprintf(self->selectedKegId_.data(), self->selectedKegId_.size(),
                "%s", keg->id);
  self->navigate(Route::ArchivedKegDetail);
}

void ScreenManager::handleArchivedDeleteRequest(void* const context) {
  auto* self = static_cast<ScreenManager*>(context);
  if (self == nullptr || self->contentRoot_ == nullptr ||
      self->selectedArchivedKeg() == nullptr) return;
  self->actionModal_.showConfirm(
      self->contentRoot_, "APAGAR ARQUIVADO?",
      "Remove definitivamente o cadastro. O historico bruto no SD sera preservado.",
      handleArchivedDeleteConfirmed, self, "APAGAR");
}

void ScreenManager::handleArchivedDeleteConfirmed(void* const context) {
  auto* self = static_cast<ScreenManager*>(context);
  if (self == nullptr || self->viewModel_ == nullptr) return;
  const services::KegError result =
      self->viewModel_->deleteKeg(self->selectedKegId_.data());
  if (result == services::KegError::None) {
    self->selectedKegId_ = {};
    self->navigate(Route::ArchivedKegs);
  } else {
    KEEZER_LOG_WARN(kLogTag, "Could not delete archived keg: %s",
                    services::kegErrorName(result));
  }
}

void ScreenManager::handleOpenDisplay(void* const context) {
  auto* self = static_cast<ScreenManager*>(context);
  if (self != nullptr) self->navigate(Route::DisplaySettings);
}

void ScreenManager::handleShowOta(void* const context) {
  auto* self = static_cast<ScreenManager*>(context);
  if (self == nullptr || self->viewModel_ == nullptr ||
      self->contentRoot_ == nullptr) return;
  const app::NetworkViewData& network = self->viewModel_->network();
  char message[160]{};
  if (network.connected && network.ipAddress[0] != '\0') {
    std::snprintf(message, sizeof(message),
                  "OTA: http://%s/update\nLOGS: http://%s/logs",
                  network.ipAddress.data(), network.ipAddress.data());
  } else {
    std::snprintf(message, sizeof(message),
                  "Conecte o controlador ao Wi-Fi para usar a atualizacao OTA.");
  }
  self->actionModal_.showInfo(self->contentRoot_, "SERVICOS WEB", message);
}

app::DisplaySaveResult ScreenManager::handleSaveDisplay(
    const std::uint8_t brightness, const std::uint16_t timeoutSeconds,
    void* const context) {
  auto* self = static_cast<ScreenManager*>(context);
  if (self == nullptr || self->viewModel_ == nullptr) {
    return app::DisplaySaveResult::StorageError;
  }
  return self->viewModel_->saveDisplaySettings(brightness, timeoutSeconds);
}

void ScreenManager::handleSaveNetwork(void* const context) {
  auto* const self = static_cast<ScreenManager*>(context);
  if (self == nullptr || self->viewModel_ == nullptr) return;
  const app::NetworkSaveResult result = self->viewModel_->saveNetworkSettings(
      self->networkSettingsScreen_.request());
  self->networkSettingsScreen_.showResult(result);
  if (result != app::NetworkSaveResult::Ok) {
    KEEZER_LOG_WARN(kLogTag, "Network settings rejected: result=%u",
                    static_cast<unsigned int>(result));
  }
}

void ScreenManager::handleCancelNetwork(void* const context) {
  auto* const self = static_cast<ScreenManager*>(context);
  if (self != nullptr) self->navigate(Route::Settings);
}

app::TemperatureSetpointResult ScreenManager::handleTemperatureSetpoint(
    const std::int16_t setpointCentiCelsius, void* const context) {
  auto* const self = static_cast<ScreenManager*>(context);
  if (self == nullptr || self->viewModel_ == nullptr) {
    return app::TemperatureSetpointResult::ServiceError;
  }
  return self->viewModel_->setTemperatureSetpointCentiCelsius(
      setpointCentiCelsius);
}

void ScreenManager::handleBack(void* const context) {
  auto* const self = static_cast<ScreenManager*>(context);
  if (self != nullptr) {
    if (self->currentRoute_ == Route::NetworkSettings ||
        self->currentRoute_ == Route::ArchivedKegs ||
        self->currentRoute_ == Route::DisplaySettings) {
      self->navigate(Route::Settings);
    } else if (self->currentRoute_ == Route::ArchivedKegDetail) {
      self->navigate(Route::ArchivedKegs);
    } else if (self->currentRoute_ == Route::KegEdit &&
               !self->creatingKeg_) {
      self->navigate(Route::KegDetail);
    } else {
      self->navigate(Route::Home);
    }
  }
}

void ScreenManager::openKeg(const std::size_t index) {
  if (viewModel_ == nullptr) {
    return;
  }
  const app::KegViewData* const keg = viewModel_->kegAt(index);
  if (keg == nullptr || keg->id == nullptr) {
    return;
  }
  std::snprintf(selectedKegId_.data(), selectedKegId_.size(), "%s", keg->id);
  creatingKeg_ = false;
  navigate(Route::KegDetail);
}

void ScreenManager::openKegEditor(const bool focusNfc) {
  if (selectedKeg() == nullptr) {
    return;
  }
  creatingKeg_ = false;
  focusNfcOnEdit_ = focusNfc;
  navigate(Route::KegEdit);
}

const app::KegViewData* ScreenManager::selectedKeg() const {
  return viewModel_ == nullptr
             ? nullptr
             : viewModel_->findKegById(selectedKegId_.data());
}

const app::KegViewData* ScreenManager::selectedArchivedKeg() const {
  return viewModel_ == nullptr
             ? nullptr
             : viewModel_->findArchivedKegById(selectedKegId_.data());
}

void ScreenManager::configureHeader() {
  switch (currentRoute_) {
    case Route::Home:
      header_->showHome(viewModel_->kegCount());
      break;
    case Route::KegDetail: {
      const app::KegViewData* const keg = selectedKeg();
      char title[20]{};
      std::snprintf(title, sizeof(title), "BARRIL %02u",
                    keg == nullptr ? 0U
                                   : static_cast<unsigned int>(keg->number));
      header_->showTitle(title, true);
      break;
    }
    case Route::KegEdit:
      header_->showTitle(creatingKeg_ ? "NOVO KEG" : "EDITAR KEG", true);
      break;
    case Route::Temperature:
      header_->showTitle("CONTROLE KEEZER", false);
      break;
    case Route::Settings:
      header_->showTitle("CONFIG.", false);
      break;
    case Route::NetworkSettings:
      header_->showTitle("REDE", true);
      break;
    case Route::ArchivedKegs:
      header_->showTitle("BARRIS ARQUIVADOS", true);
      break;
    case Route::ArchivedKegDetail:
      header_->showTitle("HISTORICO KEG", true);
      break;
    case Route::DisplaySettings:
      header_->showTitle("DISPLAY", true);
      break;
  }
}

void ScreenManager::renderCurrent() {
  actionModal_.reset();
  lv_obj_clean(contentRoot_);
  switch (currentRoute_) {
    case Route::Home:
      homeScreen_.create(contentRoot_, *viewModel_, handleKegDetails,
                         handleNewKeg, this);
      break;
    case Route::KegDetail: {
      const app::KegViewData* const keg = selectedKeg();
      if (keg != nullptr) {
        kegDetailScreen_.create(
            contentRoot_, *keg, viewModel_->manualWeighing(),
            handleWeighRequest, handleRemoveRequest, handleEditKeg, handleEditNfc,
            handleFinishRequest, handleArchiveRequest, this);
      }
      break;
    }
    case Route::KegEdit: {
      char suggestedId[models::kKegIdBytes]{};
      viewModel_->suggestNextKegId(suggestedId, sizeof(suggestedId));
      kegEditScreen_.create(contentRoot_, creatingKeg_ ? nullptr : selectedKeg(),
                            suggestedId, handleSaveKeg, handleCancelEdit, this,
                            focusNfcOnEdit_,
                            creatingKeg_ ? pendingNfcPrefill_.data() : nullptr);
      break;
    }
    case Route::Temperature:
      temperatureScreen_.create(contentRoot_, viewModel_->freezer(),
                                handleTemperatureSetpoint, this);
      break;
    case Route::Settings:
      settingsScreen_.create(contentRoot_, viewModel_->scale(),
                             viewModel_->network(),
                             viewModel_->historyStorage(), handleOpenArchived,
                             handleShowOta, handleOpenNetwork,
                             handleOpenDisplay, this);
      break;
    case Route::NetworkSettings:
      networkSettingsScreen_.create(
          contentRoot_, viewModel_->network(), handleSaveNetwork,
          handleCancelNetwork, this);
      break;
    case Route::ArchivedKegs:
      archivedKegsScreen_.createList(contentRoot_, *viewModel_,
                                     handleArchivedSelection, this);
      break;
    case Route::ArchivedKegDetail: {
      const app::KegViewData* keg = selectedArchivedKeg();
      if (keg != nullptr) {
        archivedKegsScreen_.createDetail(contentRoot_, *keg,
                                         handleArchivedDeleteRequest, this);
      }
      break;
    }
    case Route::DisplaySettings:
      displaySettingsScreen_.create(contentRoot_, viewModel_->display(),
                                    handleSaveDisplay, this);
      break;
  }
}

void ScreenManager::showWorkflowPrompts() {
  if (viewModel_ == nullptr || contentRoot_ == nullptr ||
      actionModal_.isOpen()) {
    return;
  }

  const app::ManualWeighingViewData& weighing =
      viewModel_->manualWeighing();
  if (currentRoute_ == Route::KegDetail &&
      weighing.status == app::ManualWeighingStatus::ReadyToConfirm &&
      weighing.revision != lastManualPromptRevision_ &&
      std::strcmp(weighing.kegId.data(), selectedKegId_.data()) == 0) {
    lastManualPromptRevision_ = weighing.revision;
    char message[128]{};
    const char* warning = "";
    if (weighing.nfcConflict) {
      warning = "NFC INDICA OUTRO KEG.\n";
    } else if (weighing.validity ==
               models::MeasurementValidity::AboveExpectedMaximum) {
      warning = "PESO ACIMA DO ESPERADO.\n";
    } else if (weighing.validity ==
               models::MeasurementValidity::BelowTare) {
      warning = "PESO ABAIXO DA TARA.\n";
    }
    if (warning[0] == '\0') {
      std::snprintf(
          message, sizeof(message),
          "PESO: %.2f kg\nVOLUME: %.1f L\nSalvar neste KEG?",
          static_cast<double>(weighing.weightGrams) / 1'000.0,
          static_cast<double>(weighing.volumeMl) / 1'000.0);
    } else {
      std::snprintf(
          message, sizeof(message), "%sPESO %.2f kg | %.1f L\nSalvar neste KEG?",
          warning, static_cast<double>(weighing.weightGrams) / 1'000.0,
          static_cast<double>(weighing.volumeMl) / 1'000.0);
    }
    actionModal_.showConfirm(contentRoot_, "CONFIRMAR PESAGEM", message,
                             handleWeighConfirmed, this, "SALVAR", "CANCELAR",
                             handleWeighCancelled, this);
    return;
  }

  const app::PendingUnknownNfcViewData& unknown = viewModel_->unknownNfc();
  if (currentRoute_ != Route::KegEdit && !viewModel_->manualWeighingActive() &&
      unknown.pending && unknown.revision != lastUnknownPromptRevision_) {
    lastUnknownPromptRevision_ = unknown.revision;
    char message[112]{};
    std::snprintf(message, sizeof(message),
                  "TAG %s NAO CADASTRADA.\nCriar um novo KEG?",
                  unknown.uid.data());
    actionModal_.showConfirm(contentRoot_, "NFC DESCONHECIDO", message,
                             handleUnknownNfcRegister, this, "CADASTRAR",
                             "IGNORAR", handleUnknownNfcIgnore, this);
  }
}

}  // namespace keezer::ui
