#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "models/Scale.h"
#include "models/Keg.h"
#include "models/KegMeasurement.h"
#include "models/KegStatus.h"
#include "models/Network.h"
#include "models/Temperature.h"
#include "models/Display.h"

namespace keezer::app {

inline constexpr std::size_t kMaxKegs = 32U;

struct FreezerViewData {
  float temperatureC{0.0F};
  float setpointC{0.0F};
  const char* compressorState{nullptr};
  bool compressorOn{false};
  bool hasTemperature{false};
  bool demandCooling{false};
  models::TemperatureSampleStatus sampleStatus{
      models::TemperatureSampleStatus::NoData};
  models::TemperatureControlState controlState{
      models::TemperatureControlState::Idle};
  models::TemperatureFault temperatureFault{models::TemperatureFault::None};
  std::uint32_t protectionRemainingMs{0U};
};

enum class TemperatureSetpointResult : std::uint8_t {
  Ok = 0U,
  OutOfRange,
  StorageError,
  ServiceError,
};

enum class TrackingViewStatus : std::uint8_t {
  Idle = 0U,
  Acquiring,
  Monitoring,
  Paused,
  ChangePending,
};

struct KegViewData {
  std::uint8_t number;
  const char* name;
  float volumeL;
  float capacityL;
  std::uint8_t percentage;
  float weightKg;
  float tareKg;
  float densityKgPerL;
  const char* nfcUid;
  models::KegStatus status;
  bool onScale;
  std::int64_t lastSyncUtc{models::kUnknownTimestamp};
  float dailyAverageL;
  std::array<std::int16_t, 7U> historyDeciliters;
  const char* id;
  const char* beerStyle;
  const char* batch;
  const char* notes;
  const char* filledDate;
  TrackingViewStatus trackingStatus{TrackingViewStatus::Idle};
  std::uint8_t historyPointCount{0U};
};

struct KegEditRequest {
  const char* id;
  const char* nfcUid;
  const char* name;
  const char* beerStyle;
  const char* batch;
  const char* notes;
  const char* filledDate;
  float capacityL;
  float tareKg;
  float densityKgPerL;
};

struct ScaleViewData {
  models::ScaleLinkStatus linkStatus{models::ScaleLinkStatus::Offline};
  bool hasWeight{false};
  std::int32_t weightGrams{0};
  bool stable{false};
  bool hasNfcUid{false};
  std::array<char, models::kScaleNfcUidBytes> nfcUid{};
  bool hasBatteryPercentage{false};
  std::uint8_t batteryPercentage{0U};
  bool hasRssiDbm{false};
  std::int8_t rssiDbm{0};
  bool nfcAmbiguous{false};
};

struct NetworkViewData {
  models::NetworkStatus status{models::NetworkStatus::Disabled};
  std::array<char, models::kWifiSsidBytes> ssid{};
  std::array<char, models::kNetworkHostnameBytes> hostname{};
  std::array<char, models::kNetworkAddressBytes> scaleHost{};
  std::array<char, models::kIpAddressBytes> ipAddress{};
  std::uint16_t scalePort{80U};
  std::int16_t rssiDbm{0};
  std::uint32_t retryInMs{0U};
  bool configured{false};
  bool connected{false};
  bool hasRssi{false};
};

enum class HistoryStorageViewStatus : std::uint8_t {
  Ready = 0U,
  Missing,
  Full,
  Error,
};

struct HistoryStorageViewData {
  HistoryStorageViewStatus status{HistoryStorageViewStatus::Missing};
  std::uint16_t pendingCount{0U};
  std::uint32_t droppedCount{0U};
};

struct NetworkEditRequest {
  const char* ssid;
  const char* password;
  const char* hostname;
  const char* scaleHost;
  std::uint16_t scalePort;
};

enum class NetworkSaveResult : std::uint8_t {
  Ok = 0U,
  InvalidData,
  StorageError,
  ServiceError,
};

enum class DisplaySaveResult : std::uint8_t {
  Ok = 0U,
  InvalidData,
  StorageError,
};

enum class ManualWeighingStatus : std::uint8_t {
  Idle = 0U,
  WaitingForReading,
  ReadyToConfirm,
  Completed,
  TimedOut,
  Error,
};

struct ManualWeighingViewData {
  ManualWeighingStatus status{ManualWeighingStatus::Idle};
  std::array<char, models::kKegIdBytes> kegId{};
  std::int32_t weightGrams{0};
  std::uint32_t volumeMl{0U};
  std::uint16_t percentageBasisPoints{0U};
  models::MeasurementValidity validity{
      models::MeasurementValidity::InvalidWeight};
  bool nfcConflict{false};
  std::uint32_t revision{0U};
};

struct PendingUnknownNfcViewData {
  bool pending{false};
  std::array<char, models::kScaleNfcUidBytes> uid{};
  std::uint32_t revision{0U};
};

struct AppViewState {
  FreezerViewData freezer{};
  ScaleViewData scale{};
  ManualWeighingViewData manualWeighing{};
  PendingUnknownNfcViewData unknownNfc{};
  NetworkViewData network{};
  HistoryStorageViewData historyStorage{};
  models::DisplaySettings display{};
  std::array<KegViewData, kMaxKegs> kegs{};
  std::size_t kegCount{0U};
  std::size_t archivedKegCount{0U};
  std::uint32_t revision{0U};
};

inline const char* kegStatusText(const models::KegStatus status) {
  switch (status) {
    case models::KegStatus::ActiveOnScale:
      return "ATIVO";
    case models::KegStatus::Stored:
      return "ARMAZENADO";
    case models::KegStatus::Finished:
      return "FINALIZADO";
    default:
      return models::kegStatusName(status);
  }
}

inline const char* trackingStatusText(const TrackingViewStatus status) {
  switch (status) {
    case TrackingViewStatus::Acquiring:
      return "LENDO";
    case TrackingViewStatus::Monitoring:
      return "MONITORANDO";
    case TrackingViewStatus::Paused:
      return "PAUSADO";
    case TrackingViewStatus::ChangePending:
      return "VERIFICAR";
    case TrackingViewStatus::Idle:
      return "LIDO";
  }
  return "LIDO";
}

}  // namespace keezer::app
