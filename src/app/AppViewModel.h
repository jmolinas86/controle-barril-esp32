#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "app/DemoDataSource.h"
#include "app/ViewData.h"
#include "services/KegService.h"
#include "services/HistoryService.h"
#include "services/NetworkService.h"
#include "services/ScaleService.h"
#include "services/TemperatureService.h"

namespace keezer::app {

class AppViewModel final {
 public:
  void begin(services::KegService& kegService,
             services::ScaleService& scaleService,
             services::TemperatureService& temperatureService,
             services::HistoryService& historyService,
             services::NetworkService& networkService,
             std::uint32_t nowMs);
  void update(std::uint32_t nowMs);

  const FreezerViewData& freezer() const;
  const ScaleViewData& scale() const;
  const ManualWeighingViewData& manualWeighing() const;
  const PendingUnknownNfcViewData& unknownNfc() const;
  const NetworkViewData& network() const;
  const HistoryStorageViewData& historyStorage() const;
  const models::DisplaySettings& display() const;
  const std::array<KegViewData, kMaxKegs>& kegs() const;
  const KegViewData* kegAt(std::size_t index) const;
  const KegViewData* findKegById(const char* id) const;
  std::size_t kegCount() const;
  const KegViewData* archivedKegAt(std::size_t index) const;
  const KegViewData* findArchivedKegById(const char* id) const;
  std::size_t archivedKegCount() const;
  std::uint32_t revision() const;

  services::KegError createKeg(const KegEditRequest& request);
  services::KegError updateKeg(const char* currentId,
                               const KegEditRequest& request);
  services::KegError archiveKeg(const char* id);
  services::KegError deleteKeg(const char* id);
  services::KegError setActiveKeg(const char* id, bool confirmed = false);
  services::KegError removeActiveKeg();
  services::KegError beginManualWeighing(const char* id);
  services::KegError confirmManualWeighing();
  void cancelManualWeighing();
  bool manualWeighingActive() const;
  void setTrackingStatus(const char* kegId, TrackingViewStatus status);
  void reportUnknownNfc(const char* uid);
  void clearUnknownNfc();
  void suggestNextKegId(char* destination, std::size_t size) const;
  TemperatureSetpointResult setTemperatureSetpointCentiCelsius(
      std::int16_t setpointCentiCelsius);
  NetworkSaveResult saveNetworkSettings(const NetworkEditRequest& request);
  DisplaySaveResult saveDisplaySettings(std::uint8_t brightnessPercent,
                                        std::uint16_t timeoutSeconds);

 private:
  AppViewState state_{};
  demo::DemoDataSource demoDataSource_{};
  services::KegService* kegService_{nullptr};
  services::ScaleService* scaleService_{nullptr};
  services::TemperatureService* temperatureService_{nullptr};
  services::HistoryService* historyService_{nullptr};
  services::NetworkService* networkService_{nullptr};
  std::array<std::array<char, 11U>, kMaxKegs> filledDateText_{};
  std::array<std::array<char, 11U>, kMaxKegs> archivedFilledDateText_{};
  std::uint32_t catalogRevision_{0U};
  std::uint32_t scaleRevision_{0U};
  std::uint32_t temperatureRevision_{0U};
  std::uint32_t historyRevision_{0U};
  std::uint32_t networkRevision_{0U};
  bool initialized_{false};
  std::uint32_t lastNowMs_{0U};
  std::uint32_t manualWeighingStartedAtMs_{0U};
  std::uint32_t manualWeighingStartReadingCount_{0U};
  std::array<char, models::kKegIdBytes> trackingKegId_{};
  TrackingViewStatus trackingStatus_{TrackingViewStatus::Idle};

  void syncCatalog();
  void syncScale();
  void syncTemperature();
  void syncHistory();
  void syncNetwork();
  void updateManualWeighing(std::uint32_t nowMs);
  services::KegError applyCatalogResult(services::KegError result);
  static bool parseDate(const char* text, std::int64_t& timestamp);
  static void formatDate(std::int64_t timestamp, char* destination,
                         std::size_t size);
};

}  // namespace keezer::app
