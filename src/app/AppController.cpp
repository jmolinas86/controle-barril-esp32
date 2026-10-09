#include "app/AppController.h"

#include <Arduino.h>
#include <esp_system.h>

#include <cstdio>
#include <cstring>
#include <time.h>

#include "BuildConfig.h"
#include "NetworkConfig.h"
#include "diagnostics/Logger.h"
#include "storage/TemperatureSettingsStore.h"

namespace keezer::app {
namespace {

constexpr char kLogTag[] = "APP";

const char* resetReasonName(const esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON:
      return "POWER_ON";
    case ESP_RST_SW:
      return "SOFTWARE";
    case ESP_RST_PANIC:
      return "PANIC";
    case ESP_RST_INT_WDT:
      return "INTERRUPT_WATCHDOG";
    case ESP_RST_TASK_WDT:
      return "TASK_WATCHDOG";
    case ESP_RST_WDT:
      return "WATCHDOG";
    case ESP_RST_DEEPSLEEP:
      return "DEEP_SLEEP";
    case ESP_RST_BROWNOUT:
      return "BROWNOUT";
    default:
      return "OTHER";
  }
}

TrackingViewStatus trackingViewStatus(
    const services::KegTrackingState state) {
  switch (state) {
    case services::KegTrackingState::Idle:
      return TrackingViewStatus::Idle;
    case services::KegTrackingState::Acquiring:
      return TrackingViewStatus::Acquiring;
    case services::KegTrackingState::Monitoring:
      return TrackingViewStatus::Monitoring;
    case services::KegTrackingState::Paused:
      return TrackingViewStatus::Paused;
    case services::KegTrackingState::ChangePending:
      return TrackingViewStatus::ChangePending;
  }
  return TrackingViewStatus::Idle;
}

constexpr models::KegDraft kInitialKegs[] = {
    {"KEG_001", "04A23F891C", "GERMAN PILSNER", "GERMAN PILSNER", "",
     "", 20'000U, 4'350, 1'000U, 18'550, 14'200U},
    {"KEG_002", "04B17D2210", "WEST COAST IPA", "WEST COAST IPA", "",
     "", 20'000U, 4'350, 1'000U, 10'650, 6'300U},
    {"KEG_003", "04C09E118A", "VIENNA LAGER", "VIENNA LAGER", "", "",
     20'000U, 4'350, 1'000U, 4'750, 400U},
};

bool seedDemoCatalog(services::KegService& service) {
  if (service.count() != 0U) {
    return true;
  }
  for (const models::KegDraft& keg : kInitialKegs) {
    const services::KegError result = service.createKeg(keg);
    if (result != services::KegError::None) {
      KEEZER_LOG_ERROR(kLogTag, "Demo seed failed: %s",
                       services::kegErrorName(result));
      return false;
    }
  }
  return service.setActiveKeg("KEG_001", services::ActivationSource::Manual) ==
         services::KegError::None;
}

}  // namespace

bool AppController::begin() {
  diagnostics::Logger::begin(config::kSerialBaud);
  diagnostics::Logger::setLevel(diagnostics::LogLevel::Info);

  KEEZER_LOG_INFO(kLogTag,
                  "Keezer Controller - Phase 19.3 Adaptive Polling");
  const esp_reset_reason_t resetReason = esp_reset_reason();
  KEEZER_LOG_INFO(kLogTag, "BOOT_RECOVERY reset=%s(%d) CPU=%luMHz outputs=SAFE_OFF",
                  resetReasonName(resetReason), static_cast<int>(resetReason),
                  static_cast<unsigned long>(ESP.getCpuFreqMHz()));

  if (!board_.begin()) {
    KEEZER_LOG_ERROR(kLogTag, "Board bring-up failed");
    return false;
  }

  models::NetworkSettings networkSettings{};
  std::snprintf(networkSettings.ssid.data(), networkSettings.ssid.size(),
                "%s", KEEZER_WIFI_SSID);
  std::snprintf(networkSettings.password.data(),
                networkSettings.password.size(), "%s",
                KEEZER_WIFI_PASSWORD);
  std::snprintf(networkSettings.hostname.data(),
                networkSettings.hostname.size(), "%s",
                KEEZER_CONTROLLER_HOSTNAME);
  std::snprintf(networkSettings.scaleHost.data(),
                networkSettings.scaleHost.size(), "%s",
                KEEZER_SCALE_HTTP_HOST);
  networkSettings.scalePort = KEEZER_SCALE_HTTP_PORT;
  const storage::NetworkSettingsResult storedNetwork =
      storage::NetworkSettingsStore::load(networkSettings);
  if (storedNetwork == storage::NetworkSettingsResult::Invalid ||
      storedNetwork == storage::NetworkSettingsResult::IoError) {
    KEEZER_LOG_WARN(kLogTag,
                    "Stored network settings unavailable; using build defaults");
  }
  services::NetworkServiceSettings networkServiceSettings{};
  networkServiceSettings.connectionTimeoutMs =
      config::kNetworkConnectionTimeoutMs;
  networkServiceSettings.initialRetryMs = config::kNetworkInitialRetryMs;
  networkServiceSettings.maximumRetryMs = config::kNetworkMaximumRetryMs;
  networkServiceSettings.telemetryRefreshMs =
      config::kNetworkTelemetryRefreshMs;
  if (!networkService_.begin(networkSettings, millis(),
                             networkServiceSettings)) {
    KEEZER_LOG_WARN(kLogTag,
                    "Network service started with invalid configuration");
  }
  if (!otaService_.begin(KEEZER_OTA_USERNAME, KEEZER_OTA_PASSWORD,
                         enterOtaMaintenance, this)) {
    KEEZER_LOG_WARN(kLogTag, "Web OTA unavailable; USB upload remains active");
  }

  const bool catalogLoaded = kegService_.begin();
  if (!catalogLoaded) {
    KEEZER_LOG_WARN(kLogTag, "Keg storage unavailable; UI will continue");
  }
  if (config::kDemoMode && catalogLoaded && !seedDemoCatalog(kegService_)) {
    KEEZER_LOG_WARN(kLogTag, "Demo catalog could not be seeded");
  }

  const bool historyReady = historyService_.begin(millis(), esp_random());
  if (!historyReady) {
    KEEZER_LOG_WARN(kLogTag,
                    "History storage unavailable; buffering recent records in RAM");
  }

  services::ScaleSettings scaleSettings{};
  scaleSettings.maximumWeightGrams = config::kScaleMaximumWeightGrams;
  if (!config::kScaleUseHttp) {
    scaleSettings.onlineTimeoutMs = config::kScaleSimulatorOnlineTimeoutMs;
    scaleSettings.offlineTimeoutMs = config::kScaleSimulatorOfflineTimeoutMs;
  } else {
    scaleSettings.onlineTimeoutMs = config::kScaleOnlineTimeoutMs;
    scaleSettings.offlineTimeoutMs = config::kScaleOfflineTimeoutMs;
  }
  scaleSettings.nfcMinimumConsecutiveReadings =
      config::kNfcMinimumConsecutiveReadings;
  scaleSettings.nfcStabilizationMs = config::kNfcStabilizationMs;
  scaleSettings.nfcAmbiguityWindowMs = config::kNfcAmbiguityWindowMs;
  scaleSettings.weightFilterWindowSize = config::kWeightFilterWindowSize;
  scaleSettings.weightDeadbandGrams = config::kWeightFilterDeadbandGrams;
  scaleSettings.significantWeightChangeGrams =
      config::kSignificantWeightChangeGrams;
  const bool scaleReady = scaleService_.begin(millis(), scaleSettings);
  if (!scaleReady) {
    KEEZER_LOG_WARN(kLogTag, "Scale source unavailable; UI will continue");
  }

  services::KegPresenceSettings presenceSettings{};
  presenceSettings.removalWeightThresholdGrams =
      config::kKegRemovalWeightThresholdGrams;
  presenceSettings.significantDropGrams =
      config::kKegRemovalSignificantDropGrams;
  presenceSettings.strongConfirmationMs =
      config::kKegRemovalStrongConfirmationMs;
  presenceSettings.moderateConfirmationMs =
      config::kKegRemovalModerateConfirmationMs;
  // In hybrid mode, a large consumption is not proof that the KEG was
  // removed. Only the near-zero platform reading may remove it automatically;
  // ambiguous changes are handed to KegTrackingService for confirmation.
  presenceSettings.allowSignificantDropRemovalWithoutNfc = false;
  if (!presenceService_.begin(presenceSettings)) {
    KEEZER_LOG_ERROR(kLogTag, "Keg presence service failed to start");
    return false;
  }

  services::KegTrackingSettings trackingSettings{};
  trackingSettings.minimumConsumptionGrams =
      config::kAutoTrackingMinimumConsumptionGrams;
  trackingSettings.stableConfirmationMs =
      config::kAutoTrackingStableConfirmationMs;
  trackingSettings.minimumRecordIntervalMs =
      config::kAutoTrackingMinimumRecordIntervalMs;
  trackingSettings.suspiciousIncreaseGrams =
      config::kAutoTrackingSuspiciousIncreaseGrams;
  trackingSettings.suspiciousChangeGrams =
      config::kAutoTrackingSuspiciousChangeGrams;
  trackingSettings.removalWeightThresholdGrams =
      config::kKegRemovalWeightThresholdGrams;
  trackingSettings.candidateDeadbandGrams =
      config::kWeightFilterDeadbandGrams;
  if (!trackingService_.begin(trackingSettings)) {
    KEEZER_LOG_ERROR(kLogTag, "Automatic tracking service failed to start");
    return false;
  }

  services::TemperatureSettings temperatureSettings{};
  temperatureSettings.setpointCentiCelsius =
      config::kTemperatureSetpointCentiCelsius;
  const storage::TemperatureSettingsResult storedSetpoint =
      storage::TemperatureSettingsStore::loadSetpointCentiCelsius(
          temperatureSettings.setpointCentiCelsius);
  if (storedSetpoint == storage::TemperatureSettingsResult::IoError ||
      storedSetpoint == storage::TemperatureSettingsResult::Invalid) {
    temperatureSettings.setpointCentiCelsius =
        config::kTemperatureSetpointCentiCelsius;
    KEEZER_LOG_WARN(kLogTag,
                    "Stored temperature setpoint unavailable; using default");
  }
  temperatureSettings.hysteresisCentiCelsius =
      config::kTemperatureHysteresisCentiCelsius;
  temperatureSettings.minimumOffTimeMs =
      config::kDemoMode ? config::kDemoCompressorMinimumOffTimeMs
                        : config::kCompressorMinimumOffTimeMs;
  temperatureSettings.minimumOnTimeMs =
      config::kDemoMode ? config::kDemoCompressorMinimumOnTimeMs
                        : config::kCompressorMinimumOnTimeMs;
  temperatureSettings.sensorTimeoutMs = config::kTemperatureSensorTimeoutMs;
  temperatureSettings.minimumValidCentiCelsius =
      config::kTemperatureMinimumValidCentiCelsius;
  temperatureSettings.maximumValidCentiCelsius =
      config::kTemperatureMaximumValidCentiCelsius;
  temperatureSettings.recoveryValidSamples =
      config::kTemperatureRecoveryValidSamples;
  const bool temperatureReady =
      temperatureService_.begin(millis(), temperatureSettings);
  if (!temperatureReady) {
    KEEZER_LOG_WARN(kLogTag,
                    "Temperature control unavailable; compressor stays OFF");
  }

  viewModel_.begin(kegService_, scaleService_, temperatureService_,
                   historyService_, networkService_, millis());
  uiReady_ = ui_.begin(board_, viewModel_);
  if (!uiReady_) {
    KEEZER_LOG_ERROR(kLogTag, "UI framework failed to start");
    return false;
  }
  nextPerformanceAtMs_ = millis() + config::kPerformanceTelemetryPeriodMs;

  KEEZER_LOG_INFO(kLogTag,
                  "Phase 19.3 initialized: kegs=%u catalog=%s history=%s scale=%s network=%s temperature=%s setpoint=%d.%02dC ota=WEB_AUTH poll=ADAPTIVE",
                  static_cast<unsigned int>(kegService_.count()),
                  kegService_.catalogAvailable() ? "OK" : "DEGRADED",
                  services::historyStorageStatusName(
                      historyService_.storageStatus()),
                  scaleReady ? (config::kScaleUseHttp ? "HTTP" : "SIMULATOR")
                             : "DEGRADED",
                  models::networkStatusName(networkService_.state().status),
                  temperatureReady ? "DS18B20_GPIO22_RELAY_GPIO27" : "ERROR",
                  static_cast<int>(temperatureSettings.setpointCentiCelsius /
                                   100),
                  static_cast<int>(
                      temperatureSettings.setpointCentiCelsius >= 0
                          ? temperatureSettings.setpointCentiCelsius % 100
                          : -(temperatureSettings.setpointCentiCelsius %
                              100)));
  return true;
}

void AppController::update() {
  const std::uint32_t loopStartedAtUs = micros();
  const std::uint32_t nowMs = millis();
  networkService_.update(nowMs);
  processNetworkStatus();
  otaService_.update(networkService_.connected(), nowMs);
  if (otaService_.uploadInProgress() || otaService_.restartPending()) {
    if (uiReady_) ui_.update(nowMs);
    yield();
    return;
  }
  temperatureService_.update(nowMs);
  scaleService_.update(nowMs);
  processNfcIdentification(nowMs);
  processKegPresence(nowMs);
  processAutomaticTracking(nowMs);
  historyService_.update(nowMs);
  if (historyService_.recoveryDue(nowMs)) {
    board_.remountSdCard();
    historyService_.retryStorage(nowMs);
  }
  if (uiReady_) {
    viewModel_.update(nowMs);
    ui_.update(nowMs);
  }
  recordPerformance(nowMs, micros() - loopStartedAtUs);
  yield();
}

void AppController::enterOtaMaintenance(void* const context) {
  auto* const self = static_cast<AppController*>(context);
  if (self == nullptr) return;
  // Stop the physical output before flash writing pauses the control loop.
  self->compressorOutput_.setEnergized(false);
  self->board_.beginSafeOutputs();
  KEEZER_LOG_WARN(kLogTag,
                  "OTA_MAINTENANCE business_writes=PAUSED outputs=SAFE_OFF");
}

void AppController::recordPerformance(const std::uint32_t nowMs,
                                      const std::uint32_t loopDurationUs) {
  ++loopCount_;
  loopTotalUs_ += loopDurationUs;
  if (loopDurationUs > loopMaxUs_) {
    loopMaxUs_ = loopDurationUs;
  }
  if (loopDurationUs >= config::kSlowLoopThresholdUs) {
    ++slowLoopCount_;
  }
  if (static_cast<std::int32_t>(nowMs - nextPerformanceAtMs_) < 0) {
    return;
  }

  const std::uint32_t averageUs =
      loopCount_ == 0U
          ? 0U
          : static_cast<std::uint32_t>(loopTotalUs_ / loopCount_);
  KEEZER_LOG_INFO(
      "PERF",
      "LOOP calls=%lu avg=%luus max=%luus slow_over_%luus=%lu",
      static_cast<unsigned long>(loopCount_),
      static_cast<unsigned long>(averageUs),
      static_cast<unsigned long>(loopMaxUs_),
      static_cast<unsigned long>(config::kSlowLoopThresholdUs),
      static_cast<unsigned long>(slowLoopCount_));

  const storage::SdPerformanceStats sd =
      historyRepository_.takePerformanceStats();
  const std::uint32_t operationCount = sd.writeAttempts + sd.readAttempts;
  const std::uint32_t averageOperationUs =
      operationCount == 0U
          ? 0U
          : static_cast<std::uint32_t>(sd.operationTotalUs / operationCount);
  KEEZER_LOG_INFO(
      "PERF",
      "SD writes=%lu reads=%lu failures=%lu bytes_write=%llu bytes_read=%llu avg=%luus max=%luus pending=%u",
      static_cast<unsigned long>(sd.writeAttempts),
      static_cast<unsigned long>(sd.readAttempts),
      static_cast<unsigned long>(sd.failedOperations),
      static_cast<unsigned long long>(sd.bytesWritten),
      static_cast<unsigned long long>(sd.bytesRead),
      static_cast<unsigned long>(averageOperationUs),
      static_cast<unsigned long>(sd.operationMaxUs),
      static_cast<unsigned int>(historyService_.pendingCount()));

  loopCount_ = 0U;
  loopTotalUs_ = 0U;
  loopMaxUs_ = 0U;
  slowLoopCount_ = 0U;
  nextPerformanceAtMs_ = nowMs + config::kPerformanceTelemetryPeriodMs;
}

void AppController::processNetworkStatus() {
  const models::NetworkState& network = networkService_.state();
  if (network.revision == observedNetworkRevision_) return;
  observedNetworkRevision_ = network.revision;
  if (network.status == observedNetworkStatus_) return;
  observedNetworkStatus_ = network.status;
  if (network.connected) {
    if (!timeSynchronizationStarted_) {
      configTzTime(KEEZER_TIMEZONE, KEEZER_NTP_SERVER_1,
                   KEEZER_NTP_SERVER_2);
      timeSynchronizationStarted_ = true;
      KEEZER_LOG_INFO(kLogTag, "TIME_SYNC_STARTED timezone=%s",
                      KEEZER_TIMEZONE);
    }
    KEEZER_LOG_INFO(kLogTag,
                    "NETWORK_CONNECTED ssid=%s ip=%s rssi=%ddBm",
                    network.ssid.data(), network.ipAddress.data(),
                    static_cast<int>(network.rssiDbm));
  } else {
    KEEZER_LOG_INFO(kLogTag, "NETWORK_STATE status=%s retry_ms=%lu",
                    models::networkStatusName(network.status),
                    static_cast<unsigned long>(network.retryInMs));
  }
}

void AppController::processNfcIdentification(const std::uint32_t nowMs) {
  const models::ScaleState& scale = scaleService_.state();
  if (scale.nfcEventRevision != handledNfcEventRevision_) {
    handledNfcEventRevision_ = scale.nfcEventRevision;
    if (!scale.hasStableNfcUid) {
      pendingNfcSelection_ = false;
      pendingNfcUid_ = {};
      viewModel_.clearUnknownNfc();
      KEEZER_LOG_INFO(kLogTag,
                      "NFC tag removed; checking removal evidence");
    } else {
      std::snprintf(pendingNfcUid_.data(), pendingNfcUid_.size(), "%s",
                    scale.stableNfcUid.data());
      pendingNfcSelection_ = true;
    }
  }

  if (!pendingNfcSelection_) {
    return;
  }

  if (viewModel_.manualWeighingActive()) {
    pendingNfcSelection_ = false;
    KEEZER_LOG_INFO(kLogTag,
                    "NFC automatic selection skipped: manual weighing active");
    return;
  }

  if (scale.linkStatus != models::ScaleLinkStatus::Online || !scale.stable ||
      !scale.hasFilteredWeight) {
    return;
  }

  const models::Keg* const keg =
      kegService_.findByNfcUid(pendingNfcUid_.data());
  if (keg == nullptr) {
    viewModel_.reportUnknownNfc(pendingNfcUid_.data());
    KEEZER_LOG_WARN(kLogTag, "UNKNOWN_NFC_TAG uid=%s",
                    pendingNfcUid_.data());
    pendingNfcSelection_ = false;
    return;
  }

  std::array<char, models::kKegIdBytes> previousActiveId{};
  const models::Keg* const previousActive = kegService_.activeKeg();
  if (previousActive != nullptr) {
    std::snprintf(previousActiveId.data(), previousActiveId.size(), "%s",
                  previousActive->id.data());
  }
  models::KegMeasurement measurement{};
  const services::KegError result = kegService_.recordMeasurement(
      keg->id.data(), scale.filteredWeightGrams,
      services::ActivationSource::Nfc, false, measurement);
  if (result == services::KegError::None) {
    historyService_.recordMeasurement(
        keg->id.data(), measurement,
        scale.hasRawWeight ? scale.rawWeightGrams : scale.filteredWeightGrams,
        models::HistorySource::Nfc, nowMs);
    viewModel_.clearUnknownNfc();
    trackingService_.activate(keg->id.data(), scale.filteredWeightGrams,
                              scale.acceptedReadingCount, nowMs);
    if (previousActiveId[0] != '\0' &&
        std::strcmp(previousActiveId.data(), keg->id.data()) != 0) {
      KEEZER_LOG_INFO(kLogTag,
                      "KEG_REMOVED_FROM_SCALE keg=%s reason=SWITCH",
                      previousActiveId.data());
    }
    KEEZER_LOG_INFO(kLogTag, "KEG_ACTIVATED keg=%s uid=%s source=NFC",
                    keg->id.data(), pendingNfcUid_.data());
    KEEZER_LOG_INFO(
        kLogTag,
        "VOLUME_CALCULATED keg=%s gross=%ldg beer=%lug volume=%luml percent=%u.%02u%% validity=%s",
        keg->id.data(), static_cast<long>(measurement.filteredWeightGrams),
        static_cast<unsigned long>(measurement.beerWeightGrams),
        static_cast<unsigned long>(measurement.volumeMl),
        static_cast<unsigned int>(measurement.percentageBasisPoints / 100U),
        static_cast<unsigned int>(measurement.percentageBasisPoints % 100U),
        models::measurementValidityName(measurement.validity));
    if (uiReady_) {
      ui_.wakeDisplay(nowMs);
      KEEZER_LOG_INFO(kLogTag, "DISPLAY_WAKE reason=NFC_MEASUREMENT_SAVED");
    }
  } else {
    KEEZER_LOG_WARN(kLogTag, "NFC activation rejected: uid=%s error=%s",
                    pendingNfcUid_.data(), services::kegErrorName(result));
  }
  pendingNfcSelection_ = false;
}

void AppController::processAutomaticTracking(const std::uint32_t nowMs) {
  const ManualWeighingViewData& manual = viewModel_.manualWeighing();
  if (manual.status == ManualWeighingStatus::Completed &&
      manual.revision != handledManualTrackingRevision_) {
    handledManualTrackingRevision_ = manual.revision;
    trackingService_.activate(manual.kegId.data(), manual.weightGrams,
                              scaleService_.state().acceptedReadingCount,
                              nowMs);
    KEEZER_LOG_INFO(kLogTag,
                    "AUTO_TRACKING_STARTED keg=%s source=MANUAL baseline=%ldg",
                    manual.kegId.data(), static_cast<long>(manual.weightGrams));
  }

  const models::Keg* active = kegService_.activeKeg();
  trackingService_.observeActiveKeg(
      active == nullptr ? nullptr : active->id.data(),
      active == nullptr ? 0 : active->lastWeightGrams, nowMs);

  const models::ScaleState& scale = scaleService_.state();
  bool identityConflict = false;
  bool identityVerified = false;
  if (active != nullptr && scale.hasStableNfcUid) {
    const models::Keg* const identified =
        kegService_.findByNfcUid(scale.stableNfcUid.data());
    identityVerified = identified != nullptr &&
                       std::strcmp(identified->id.data(), active->id.data()) ==
                           0;
    identityConflict = !identityVerified;
  }

  const services::KegTrackingDecision decision = trackingService_.update(
      nowMs, scale, identityConflict, identityVerified);
  if (decision.action == services::KegTrackingAction::RecordConsumption &&
      active != nullptr) {
    models::KegMeasurement measurement{};
    const services::KegError result = kegService_.recordMeasurement(
        active->id.data(), decision.weightGrams,
        services::ActivationSource::AutomaticTracking, true, measurement);
    const bool saved = result == services::KegError::None;
    if (saved) {
      historyService_.recordMeasurement(
          active->id.data(), measurement,
          scale.hasRawWeight ? scale.rawWeightGrams : decision.weightGrams,
          models::HistorySource::AutomaticTracking, nowMs);
      KEEZER_LOG_INFO(
          kLogTag,
          "AUTO_CONSUMPTION_RECORDED keg=%s from=%ldg to=%ldg delta=%ldg volume=%luml",
          active->id.data(),
          static_cast<long>(trackingService_.baselineWeightGrams()),
          static_cast<long>(decision.weightGrams),
          static_cast<long>(trackingService_.baselineWeightGrams() -
                            decision.weightGrams),
          static_cast<unsigned long>(measurement.volumeMl));
      if (uiReady_) {
        ui_.wakeDisplay(nowMs);
        KEEZER_LOG_INFO(kLogTag,
                        "DISPLAY_WAKE reason=AUTO_MEASUREMENT_SAVED");
      }
    } else {
      KEEZER_LOG_WARN(kLogTag,
                      "AUTO_CONSUMPTION_FAILED keg=%s error=%s",
                      active->id.data(), services::kegErrorName(result));
    }
    trackingService_.completeRecord(saved, decision.weightGrams, nowMs);
  } else if (decision.action == services::KegTrackingAction::ConfirmKeg) {
    if (uiReady_) {
      ui_.wakeDisplay(nowMs);
      KEEZER_LOG_INFO(kLogTag, "DISPLAY_WAKE reason=KEG_CONFIRM_REQUIRED");
    }
    KEEZER_LOG_WARN(
        kLogTag,
        "AUTO_TRACKING_CONFIRM_REQUIRED keg=%s baseline=%ldg current=%ldg nfc_conflict=%s",
        active == nullptr ? "NONE" : active->id.data(),
        static_cast<long>(trackingService_.baselineWeightGrams()),
        static_cast<long>(decision.weightGrams),
        identityConflict ? "YES" : "NO");
  }

  if (trackingService_.revision() != observedTrackingRevision_) {
    observedTrackingRevision_ = trackingService_.revision();
    KEEZER_LOG_INFO(kLogTag, "Auto tracking=%s keg=%s baseline=%ldg",
                    services::kegTrackingStateName(trackingService_.state()),
                    trackingService_.activeKegId()[0] == '\0'
                        ? "NONE"
                        : trackingService_.activeKegId(),
                    static_cast<long>(
                        trackingService_.baselineWeightGrams()));
  }
  viewModel_.setTrackingStatus(
      trackingService_.activeKegId(),
      trackingViewStatus(trackingService_.state()));
}

void AppController::processKegPresence(const std::uint32_t nowMs) {
  const models::Keg* const active = kegService_.activeKeg();
  const models::ScaleState& scale = scaleService_.state();
  const std::int32_t referenceWeight =
      active == nullptr
          ? 0
          : (scale.linkStatus == models::ScaleLinkStatus::Online &&
                     scale.stable && scale.hasFilteredWeight
                 ? scale.filteredWeightGrams
                 : active->lastWeightGrams);
  presenceService_.synchronizeActiveKeg(
      active == nullptr ? nullptr : active->id.data(), referenceWeight, nowMs);

  if (presenceService_.revision() != observedPresenceRevision_) {
    observedPresenceRevision_ = presenceService_.revision();
    KEEZER_LOG_INFO(kLogTag, "Keg presence=%s active=%s",
                    services::kegPresenceStateName(presenceService_.state()),
                    presenceService_.activeKegId()[0] == '\0'
                        ? "NONE"
                        : presenceService_.activeKegId());
  }

  const services::RemovalDecision decision =
      presenceService_.update(nowMs, scale);
  if (decision == services::RemovalDecision::None) {
    return;
  }
  std::array<char, models::kKegIdBytes> removedId{};
  std::snprintf(removedId.data(), removedId.size(), "%s",
                presenceService_.activeKegId());
  const services::KegError result = kegService_.removeActiveKeg(
      services::RemovalReason::ConfirmedSignals);
  if (result == services::KegError::None) {
    presenceService_.synchronizeActiveKeg(nullptr, 0, nowMs);
    KEEZER_LOG_INFO(kLogTag,
                    "KEG_REMOVED_FROM_SCALE keg=%s reason=%s",
                    removedId.data(), services::removalDecisionName(decision));
  } else {
    KEEZER_LOG_WARN(kLogTag, "Automatic removal failed: %s",
                    services::kegErrorName(result));
  }
}

}  // namespace keezer::app
