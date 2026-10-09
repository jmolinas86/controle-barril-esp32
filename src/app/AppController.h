#pragma once

#include <array>
#include <cstdint>

#include "BuildConfig.h"
#include "app/AppViewModel.h"
#include "bsp/BoardSupport.h"
#include "services/KegService.h"
#include "services/HistoryService.h"
#include "services/KegPresenceService.h"
#include "services/KegTrackingService.h"
#include "services/NetworkService.h"
#include "services/ScaleService.h"
#include "services/TemperatureService.h"
#if KEEZER_SCALE_USE_HTTP
#include "scale/HttpScaleTransport.h"
#else
#include "scale/SimulatedScaleTransport.h"
#endif
#include "storage/LittleFsKegRepository.h"
#include "storage/NetworkSettingsStore.h"
#include "storage/SdHistoryRepository.h"
#include "network/Esp32WiFiAdapter.h"
#include "ota/OtaService.h"
#include "temperature/Ds18b20TemperatureSensor.h"
#include "temperature/GpioCompressorOutput.h"
#include "ui/UIManager.h"

namespace keezer::app {

class AppController final {
 public:
  bool begin();
  void update();

 private:
  void processNfcIdentification(std::uint32_t nowMs);
  void processKegPresence(std::uint32_t nowMs);
  void processAutomaticTracking(std::uint32_t nowMs);
  void processNetworkStatus();
  static void enterOtaMaintenance(void* context);
  void recordPerformance(std::uint32_t nowMs, std::uint32_t loopDurationUs);

  bsp::BoardSupport board_;
  storage::LittleFsKegRepository kegRepository_;
  services::KegService kegService_{kegRepository_};
  storage::SdHistoryRepository historyRepository_;
  services::HistoryService historyService_{historyRepository_};
  network::Esp32WiFiAdapter networkAdapter_;
  services::NetworkService networkService_{networkAdapter_};
  ota::OtaService otaService_;
#if KEEZER_SCALE_USE_HTTP
  scale::HttpScaleTransport scaleTransport_{networkService_};
#else
  scale::SimulatedScaleTransport scaleTransport_;
#endif
  services::ScaleService scaleService_{scaleTransport_};
  services::KegPresenceService presenceService_;
  services::KegTrackingService trackingService_;
  temperature::Ds18b20TemperatureSensor temperatureSensor_{
      static_cast<std::uint8_t>(bsp::pins::kTemperatureDataCandidate)};
  temperature::GpioCompressorOutput compressorOutput_{
      static_cast<std::uint8_t>(bsp::pins::kCompressorRelayCandidate),
      bsp::pins::kCompressorRelayActiveHigh};
  services::TemperatureService temperatureService_{temperatureSensor_,
                                                   compressorOutput_};
  AppViewModel viewModel_;
  ui::UIManager ui_;
  bool uiReady_{false};
  std::uint32_t handledNfcEventRevision_{0U};
  std::uint32_t observedPresenceRevision_{0U};
  std::uint32_t observedTrackingRevision_{0U};
  std::uint32_t handledManualTrackingRevision_{0U};
  std::uint32_t observedNetworkRevision_{0U};
  models::NetworkStatus observedNetworkStatus_{
      models::NetworkStatus::Disabled};
  std::array<char, models::kScaleNfcUidBytes> pendingNfcUid_{};
  bool pendingNfcSelection_{false};
  bool timeSynchronizationStarted_{false};
  std::uint32_t nextPerformanceAtMs_{0U};
  std::uint32_t loopCount_{0U};
  std::uint64_t loopTotalUs_{0U};
  std::uint32_t loopMaxUs_{0U};
  std::uint32_t slowLoopCount_{0U};
};

}  // namespace keezer::app
