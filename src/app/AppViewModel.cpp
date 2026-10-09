#include "app/AppViewModel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>

#include "BuildConfig.h"
#include "diagnostics/Logger.h"
#include "storage/DisplaySettingsStore.h"
#include "storage/TemperatureSettingsStore.h"
#include "storage/NetworkSettingsStore.h"

namespace keezer::app {
namespace {

constexpr char kTemperatureLogTag[] = "TEMP_UI";
constexpr std::int64_t kMinimumValidUtcSeconds = 1'577'836'800LL;

bool leapYear(const unsigned int year) {
  return (year % 4U == 0U && year % 100U != 0U) || year % 400U == 0U;
}

unsigned int monthDays(const unsigned int month, const unsigned int year) {
  constexpr unsigned int days[]{31U, 28U, 31U, 30U, 31U, 30U,
                                31U, 31U, 30U, 31U, 30U, 31U};
  if (month == 0U || month > 12U) {
    return 0U;
  }
  return month == 2U && leapYear(year) ? 29U : days[month - 1U];
}

std::int64_t daysFromCivil(int year, const unsigned int month,
                           const unsigned int day) {
  year -= month <= 2U ? 1 : 0;
  const int era = (year >= 0 ? year : year - 399) / 400;
  const unsigned int yearOfEra =
      static_cast<unsigned int>(year - era * 400);
  const unsigned int adjustedMonth = month > 2U ? month - 3U : month + 9U;
  const unsigned int dayOfYear =
      (153U * adjustedMonth + 2U) /
          5U +
      day - 1U;
  const unsigned int dayOfEra =
      yearOfEra * 365U + yearOfEra / 4U - yearOfEra / 100U + dayOfYear;
  return static_cast<std::int64_t>(era) * 146097LL + dayOfEra - 719468LL;
}

void civilFromDays(std::int64_t days, int& year, unsigned int& month,
                   unsigned int& day) {
  days += 719468LL;
  const std::int64_t era = (days >= 0 ? days : days - 146096LL) / 146097LL;
  const unsigned int dayOfEra =
      static_cast<unsigned int>(days - era * 146097LL);
  const unsigned int yearOfEra =
      (dayOfEra - dayOfEra / 1460U + dayOfEra / 36524U -
       dayOfEra / 146096U) /
      365U;
  year = static_cast<int>(yearOfEra) + static_cast<int>(era) * 400;
  const unsigned int dayOfYear =
      dayOfEra - (365U * yearOfEra + yearOfEra / 4U - yearOfEra / 100U);
  const unsigned int monthPrime = (5U * dayOfYear + 2U) / 153U;
  day = dayOfYear - (153U * monthPrime + 2U) / 5U + 1U;
  month = monthPrime < 10U ? monthPrime + 3U : monthPrime - 9U;
  year += month <= 2U ? 1 : 0;
}

}  // namespace

void AppViewModel::begin(services::KegService& kegService,
                         services::ScaleService& scaleService,
                         services::TemperatureService& temperatureService,
                         services::HistoryService& historyService,
                         services::NetworkService& networkService,
                         const std::uint32_t nowMs) {
  state_ = {};
  const storage::DisplaySettingsResult displayResult =
      storage::DisplaySettingsStore::load(state_.display);
  if (displayResult == storage::DisplaySettingsResult::Invalid ||
      displayResult == storage::DisplaySettingsResult::IoError) {
    state_.display = {};
  }
  kegService_ = &kegService;
  scaleService_ = &scaleService;
  temperatureService_ = &temperatureService;
  historyService_ = &historyService;
  networkService_ = &networkService;
  lastNowMs_ = nowMs;
  trackingKegId_ = {};
  trackingStatus_ = TrackingViewStatus::Idle;
  if (config::kDemoMode) {
    demoDataSource_.begin(state_, nowMs);
  } else {
    state_.freezer.compressorState = "OFFLINE";
    state_.revision = 1U;
  }
  syncCatalog();
  syncScale();
  syncTemperature();
  syncHistory();
  syncNetwork();
  initialized_ = true;
}

void AppViewModel::update(const std::uint32_t nowMs) {
  if (!initialized_) {
    return;
  }
  lastNowMs_ = nowMs;
  bool catalogChanged = false;
  if (kegService_ != nullptr && kegService_->revision() != catalogRevision_) {
    syncCatalog();
    catalogChanged = true;
    ++state_.revision;
  }
  if (scaleService_ != nullptr && scaleService_->revision() != scaleRevision_) {
    syncScale();
    ++state_.revision;
  }
  if (temperatureService_ != nullptr &&
      temperatureService_->revision() != temperatureRevision_) {
    syncTemperature();
    ++state_.revision;
  }
  if (historyService_ != nullptr &&
      (catalogChanged || historyService_->revision() != historyRevision_)) {
    syncHistory();
    ++state_.revision;
  }
  if (networkService_ != nullptr &&
      networkService_->state().revision != networkRevision_) {
    syncNetwork();
    ++state_.revision;
  }
  updateManualWeighing(nowMs);
  if (config::kDemoMode && config::kDemoAutoplay &&
      demoDataSource_.update(state_, nowMs)) {
    syncTemperature();
    ++state_.revision;
  }
}

const FreezerViewData& AppViewModel::freezer() const { return state_.freezer; }

const ScaleViewData& AppViewModel::scale() const { return state_.scale; }

const ManualWeighingViewData& AppViewModel::manualWeighing() const {
  return state_.manualWeighing;
}

const PendingUnknownNfcViewData& AppViewModel::unknownNfc() const {
  return state_.unknownNfc;
}

const NetworkViewData& AppViewModel::network() const {
  return state_.network;
}

const HistoryStorageViewData& AppViewModel::historyStorage() const {
  return state_.historyStorage;
}

const models::DisplaySettings& AppViewModel::display() const {
  return state_.display;
}

const std::array<KegViewData, kMaxKegs>& AppViewModel::kegs() const {
  return state_.kegs;
}

const KegViewData* AppViewModel::kegAt(const std::size_t index) const {
  return index < state_.kegCount ? &state_.kegs[index] : nullptr;
}

const KegViewData* AppViewModel::findKegById(const char* const id) const {
  if (id == nullptr || id[0] == '\0') {
    return nullptr;
  }
  for (std::size_t index = 0U; index < state_.kegCount; ++index) {
    const KegViewData& keg = state_.kegs[index];
    if (keg.id != nullptr && std::strcmp(keg.id, id) == 0) {
      return &keg;
    }
  }
  return nullptr;
}

std::size_t AppViewModel::kegCount() const { return state_.kegCount; }

const KegViewData* AppViewModel::archivedKegAt(
    const std::size_t index) const {
  return index < state_.archivedKegCount
             ? &state_.kegs[kMaxKegs - 1U - index]
             : nullptr;
}

const KegViewData* AppViewModel::findArchivedKegById(
    const char* const id) const {
  if (id == nullptr || id[0] == '\0') return nullptr;
  for (std::size_t index = 0U; index < state_.archivedKegCount; ++index) {
    const KegViewData& keg = state_.kegs[kMaxKegs - 1U - index];
    if (keg.id != nullptr && std::strcmp(keg.id, id) == 0) return &keg;
  }
  return nullptr;
}

std::size_t AppViewModel::archivedKegCount() const {
  return state_.archivedKegCount;
}

std::uint32_t AppViewModel::revision() const { return state_.revision; }

services::KegError AppViewModel::createKeg(const KegEditRequest& request) {
  if (kegService_ == nullptr) {
    return services::KegError::StorageError;
  }
  if (request.capacityL < 1.0F || request.capacityL > 100.0F ||
      request.tareKg < 0.0F || request.tareKg >= 100.0F ||
      request.densityKgPerL < 0.9F || request.densityKgPerL > 1.3F) {
    return services::KegError::InvalidData;
  }
  std::int64_t filledAtUtc = models::kUnknownTimestamp;
  if (!parseDate(request.filledDate, filledAtUtc)) {
    return services::KegError::InvalidData;
  }
  const models::KegDraft draft{
      request.id,
      request.nfcUid,
      request.name,
      request.beerStyle,
      request.batch,
      request.notes,
      static_cast<std::uint32_t>(std::lround(request.capacityL * 1'000.0F)),
      static_cast<std::int32_t>(std::lround(request.tareKg * 1'000.0F)),
      static_cast<std::uint16_t>(
          std::lround(request.densityKgPerL * 1'000.0F)),
      0,
      0U,
      filledAtUtc,
  };
  return applyCatalogResult(kegService_->createKeg(draft));
}

services::KegError AppViewModel::updateKeg(
    const char* const currentId, const KegEditRequest& request) {
  if (kegService_ == nullptr) {
    return services::KegError::StorageError;
  }
  const models::Keg* const current = kegService_->findById(currentId);
  if (current == nullptr) {
    return services::KegError::NotFound;
  }
  if (request.capacityL < 1.0F || request.capacityL > 100.0F ||
      request.tareKg < 0.0F || request.tareKg >= 100.0F ||
      request.densityKgPerL < 0.9F || request.densityKgPerL > 1.3F) {
    return services::KegError::InvalidData;
  }
  std::int64_t filledAtUtc = models::kUnknownTimestamp;
  if (!parseDate(request.filledDate, filledAtUtc)) {
    return services::KegError::InvalidData;
  }
  const models::KegDraft draft{
      request.id,
      request.nfcUid,
      request.name,
      request.beerStyle,
      request.batch,
      request.notes,
      static_cast<std::uint32_t>(std::lround(request.capacityL * 1'000.0F)),
      static_cast<std::int32_t>(std::lround(request.tareKg * 1'000.0F)),
      static_cast<std::uint16_t>(
          std::lround(request.densityKgPerL * 1'000.0F)),
      current->lastWeightGrams,
      current->lastVolumeMl,
      filledAtUtc,
  };
  return applyCatalogResult(kegService_->updateKeg(currentId, draft));
}

services::KegError AppViewModel::archiveKeg(const char* const id) {
  return kegService_ == nullptr
             ? services::KegError::StorageError
             : applyCatalogResult(kegService_->archiveKeg(id));
}

services::KegError AppViewModel::deleteKeg(const char* const id) {
  return kegService_ == nullptr
             ? services::KegError::StorageError
             : applyCatalogResult(kegService_->deleteKeg(id, true));
}

services::KegError AppViewModel::setActiveKeg(const char* const id,
                                               const bool confirmed) {
  return kegService_ == nullptr
             ? services::KegError::StorageError
             : applyCatalogResult(kegService_->setActiveKeg(
                   id, services::ActivationSource::Manual, confirmed));
}

services::KegError AppViewModel::removeActiveKeg() {
  return kegService_ == nullptr
             ? services::KegError::StorageError
             : applyCatalogResult(kegService_->removeActiveKeg());
}

services::KegError AppViewModel::beginManualWeighing(const char* const id) {
  if (kegService_ == nullptr || scaleService_ == nullptr) {
    return services::KegError::StorageError;
  }
  if (kegService_->findById(id) == nullptr) {
    return services::KegError::NotFound;
  }
  ManualWeighingViewData& session = state_.manualWeighing;
  const std::uint32_t nextRevision = session.revision + 1U;
  session = {};
  session.status = ManualWeighingStatus::WaitingForReading;
  std::snprintf(session.kegId.data(), session.kegId.size(), "%s", id);
  session.revision = nextRevision;
  manualWeighingStartedAtMs_ = lastNowMs_;
  const models::ScaleState& scaleState = scaleService_->state();
  manualWeighingStartReadingCount_ = scaleState.acceptedReadingCount;
  ++state_.revision;
  return services::KegError::None;
}

services::KegError AppViewModel::confirmManualWeighing() {
  ManualWeighingViewData& session = state_.manualWeighing;
  if (kegService_ == nullptr ||
      session.status != ManualWeighingStatus::ReadyToConfirm) {
    return services::KegError::InvalidData;
  }
  models::KegMeasurement result{};
  const services::KegError saved = kegService_->recordMeasurement(
      session.kegId.data(), session.weightGrams,
      services::ActivationSource::Manual, true, result);
  if (saved == services::KegError::None) {
    session.status = ManualWeighingStatus::Completed;
    session.volumeMl = result.volumeMl;
    session.percentageBasisPoints = result.percentageBasisPoints;
    if (historyService_ != nullptr) {
      historyService_->recordMeasurement(
          session.kegId.data(), result, session.weightGrams,
          models::HistorySource::Manual, lastNowMs_);
    }
    syncCatalog();
    syncHistory();
  } else {
    session.status = ManualWeighingStatus::Error;
  }
  ++session.revision;
  ++state_.revision;
  return saved;
}

void AppViewModel::cancelManualWeighing() {
  if (state_.manualWeighing.status == ManualWeighingStatus::Idle) {
    return;
  }
  const std::uint32_t nextRevision = state_.manualWeighing.revision + 1U;
  state_.manualWeighing = {};
  state_.manualWeighing.revision = nextRevision;
  ++state_.revision;
}

bool AppViewModel::manualWeighingActive() const {
  return state_.manualWeighing.status ==
             ManualWeighingStatus::WaitingForReading ||
         state_.manualWeighing.status ==
             ManualWeighingStatus::ReadyToConfirm;
}

void AppViewModel::setTrackingStatus(const char* const kegId,
                                     const TrackingViewStatus status) {
  const char* const normalized = kegId == nullptr ? "" : kegId;
  if (std::strcmp(trackingKegId_.data(), normalized) == 0 &&
      trackingStatus_ == status) {
    return;
  }
  std::snprintf(trackingKegId_.data(), trackingKegId_.size(), "%s",
                normalized);
  trackingStatus_ = status;
  for (std::size_t index = 0U; index < state_.kegCount; ++index) {
    KegViewData& keg = state_.kegs[index];
    keg.trackingStatus =
        keg.id != nullptr && trackingKegId_[0] != '\0' &&
                std::strcmp(keg.id, trackingKegId_.data()) == 0
            ? trackingStatus_
            : TrackingViewStatus::Idle;
  }
  ++state_.revision;
}

void AppViewModel::reportUnknownNfc(const char* const uid) {
  if (uid == nullptr || uid[0] == '\0') {
    clearUnknownNfc();
    return;
  }
  if (state_.unknownNfc.pending &&
      std::strcmp(state_.unknownNfc.uid.data(), uid) == 0) {
    return;
  }
  state_.unknownNfc.pending = true;
  std::snprintf(state_.unknownNfc.uid.data(), state_.unknownNfc.uid.size(),
                "%s", uid);
  ++state_.unknownNfc.revision;
  ++state_.revision;
}

void AppViewModel::clearUnknownNfc() {
  if (!state_.unknownNfc.pending) {
    return;
  }
  state_.unknownNfc.pending = false;
  state_.unknownNfc.uid = {};
  ++state_.unknownNfc.revision;
  ++state_.revision;
}

void AppViewModel::suggestNextKegId(char* const destination,
                                    const std::size_t size) const {
  if (destination == nullptr || size == 0U) {
    return;
  }
  for (unsigned int number = 1U; number <= 999U; ++number) {
    char candidate[models::kKegIdBytes]{};
    std::snprintf(candidate, sizeof(candidate), "KEG_%03u", number);
    if (kegService_ == nullptr || kegService_->findById(candidate) == nullptr) {
      std::snprintf(destination, size, "%s", candidate);
      return;
    }
  }
  destination[0] = '\0';
}

services::KegError AppViewModel::applyCatalogResult(
    const services::KegError result) {
  if (result == services::KegError::None) {
    syncCatalog();
    syncHistory();
    ++state_.revision;
  }
  return result;
}

bool AppViewModel::parseDate(const char* const text,
                             std::int64_t& timestamp) {
  timestamp = models::kUnknownTimestamp;
  if (text == nullptr || text[0] == '\0') {
    return true;
  }
  unsigned int day = 0U;
  unsigned int month = 0U;
  unsigned int year = 0U;
  char trailing = '\0';
  if (std::sscanf(text, "%u/%u/%u %c", &day, &month, &year, &trailing) != 3 ||
      year < 2000U || year > 2199U || day == 0U ||
      day > monthDays(month, year)) {
    return false;
  }
  timestamp = daysFromCivil(static_cast<int>(year), month, day) * 86'400LL;
  return true;
}

void AppViewModel::formatDate(const std::int64_t timestamp,
                              char* const destination,
                              const std::size_t size) {
  if (destination == nullptr || size == 0U) {
    return;
  }
  if (timestamp < 0) {
    destination[0] = '\0';
    return;
  }
  int year = 0;
  unsigned int month = 0U;
  unsigned int day = 0U;
  civilFromDays(timestamp / 86'400LL, year, month, day);
  std::snprintf(destination, size, "%02u/%02u/%04d", day, month, year);
}

void AppViewModel::syncCatalog() {
  if (kegService_ == nullptr) {
    return;
  }
  // Keep the visual demo usable while a storage fault is diagnosed. A healthy,
  // intentionally empty catalog still shows the real "NENHUM KEG" state.
  if (config::kDemoMode && !kegService_->catalogAvailable()) {
    catalogRevision_ = kegService_->revision();
    return;
  }
  const auto previousKegs = state_.kegs;
  const std::size_t previousKegCount = state_.kegCount;
  std::size_t visibleCount = 0U;
  std::size_t archivedCount = 0U;
  for (std::size_t sourceIndex = 0U;
       sourceIndex < kegService_->count();
       ++sourceIndex) {
    const models::Keg* source = kegService_->at(sourceIndex);
    if (source == nullptr) {
      continue;
    }
    const bool archived = source->status == models::KegStatus::Archived;
    if (visibleCount + archivedCount >= kMaxKegs) {
      continue;
    }
    const std::size_t targetIndex = archived ? archivedCount : visibleCount;
    const std::size_t storageIndex =
        archived ? kMaxKegs - 1U - archivedCount : visibleCount;
    KegViewData& target = state_.kegs[storageIndex];
    bool restoredDemoTelemetry = false;
    if (config::kDemoMode && !archived) {
      for (std::size_t oldIndex = 0U; oldIndex < previousKegCount;
           ++oldIndex) {
        const KegViewData& old = previousKegs[oldIndex];
        if (old.id != nullptr && std::strcmp(old.id, source->id.data()) == 0) {
          target = old;
          restoredDemoTelemetry = true;
          break;
        }
      }
    }
    if (!restoredDemoTelemetry) {
      target = {};
      target.volumeL = static_cast<float>(source->lastVolumeMl) / 1'000.0F;
      target.weightKg =
          static_cast<float>(source->lastWeightGrams) / 1'000.0F;
      target.percentage = static_cast<std::uint8_t>(std::min<std::uint32_t>(
          100U, source->capacityMl == 0U
                  ? 0U
                  : (source->lastVolumeMl * 100U +
                     source->capacityMl / 2U) /
                        source->capacityMl));
      target.status = source->status;
      target.onScale = source->status == models::KegStatus::ActiveOnScale;
    }
    target.number = static_cast<std::uint8_t>(sourceIndex + 1U);
    target.id = source->id.data();
    target.name = source->name.data();
    target.beerStyle = source->beerStyle.data();
    target.batch = source->batch.data();
    target.notes = source->notes.data();
    auto& dateText = archived ? archivedFilledDateText_[targetIndex]
                              : filledDateText_[targetIndex];
    formatDate(source->filledAtUtc, dateText.data(), dateText.size());
    target.filledDate = dateText.data();
    target.capacityL = static_cast<float>(source->capacityMl) / 1'000.0F;
    target.tareKg = static_cast<float>(source->tareGrams) / 1'000.0F;
    target.densityKgPerL =
        static_cast<float>(source->densityGramsPerLiter) / 1'000.0F;
    target.nfcUid = source->nfcUid.data();
    target.volumeL = static_cast<float>(source->lastVolumeMl) / 1'000.0F;
    target.weightKg =
        static_cast<float>(source->lastWeightGrams) / 1'000.0F;
    target.percentage = static_cast<std::uint8_t>(std::min<std::uint32_t>(
        100U, source->capacityMl == 0U
                  ? 0U
                  : (source->lastVolumeMl * 100U +
                     source->capacityMl / 2U) /
                        source->capacityMl));
    target.status = source->status;
    target.onScale = source->status == models::KegStatus::ActiveOnScale;
    target.lastSyncUtc = source->lastSeenAtUtc;
    target.trackingStatus =
        trackingKegId_[0] != '\0' &&
                std::strcmp(source->id.data(), trackingKegId_.data()) == 0
            ? trackingStatus_
            : TrackingViewStatus::Idle;
    if (archived) {
      ++archivedCount;
    } else {
      ++visibleCount;
    }
  }
  state_.kegCount = visibleCount;
  state_.archivedKegCount = archivedCount;
  for (std::size_t index = visibleCount;
       index < kMaxKegs - archivedCount; ++index) {
    state_.kegs[index] = {};
  }
  catalogRevision_ = kegService_->revision();
}

void AppViewModel::syncHistory() {
  if (historyService_ == nullptr) {
    return;
  }
  switch (historyService_->storageStatus()) {
    case services::HistoryStorageStatus::Ready:
      state_.historyStorage.status = HistoryStorageViewStatus::Ready;
      break;
    case services::HistoryStorageStatus::Missing:
      state_.historyStorage.status = HistoryStorageViewStatus::Missing;
      break;
    case services::HistoryStorageStatus::Full:
      state_.historyStorage.status = HistoryStorageViewStatus::Full;
      break;
    case services::HistoryStorageStatus::IoError:
      state_.historyStorage.status = HistoryStorageViewStatus::Error;
      break;
  }
  state_.historyStorage.pendingCount = static_cast<std::uint16_t>(
      std::min<std::size_t>(historyService_->pendingCount(), 65'535U));
  state_.historyStorage.droppedCount = historyService_->droppedCount();
  const auto syncKegHistory = [this](KegViewData& target) {
    target.historyDeciliters.fill(0);
    target.historyPointCount = 0U;
    target.dailyAverageL = 0.0F;
    if (target.id == nullptr || target.id[0] == '\0') {
      return;
    }
    std::array<std::uint32_t, services::HistoryService::kGraphPointCount>
        volumes{};
    std::size_t count = 0U;
    if (!historyService_->loadRecentVolumes(target.id, volumes, count)) {
      return;
    }
    const std::size_t first = target.historyDeciliters.size() - count;
    std::uint64_t totalMl = 0U;
    for (std::size_t index = 0U; index < count; ++index) {
      const std::uint32_t deciliters = (volumes[index] + 50U) / 100U;
      target.historyDeciliters[first + index] =
          static_cast<std::int16_t>(std::min<std::uint32_t>(32'767U,
                                                           deciliters));
      totalMl += volumes[index];
    }
    target.historyPointCount = static_cast<std::uint8_t>(count);
    target.dailyAverageL =
        count == 0U ? 0.0F
                    : static_cast<float>(totalMl) /
                          (static_cast<float>(count) * 1'000.0F);
  };
  for (std::size_t kegIndex = 0U; kegIndex < state_.kegCount; ++kegIndex) {
    syncKegHistory(state_.kegs[kegIndex]);
  }
  for (std::size_t kegIndex = 0U; kegIndex < state_.archivedKegCount;
       ++kegIndex) {
    syncKegHistory(state_.kegs[kMaxKegs - 1U - kegIndex]);
  }
  historyRevision_ = historyService_->revision();
}

void AppViewModel::syncScale() {
  if (scaleService_ == nullptr) {
    return;
  }
  const models::ScaleState& source = scaleService_->state();
  state_.scale.linkStatus = source.linkStatus;
  state_.scale.hasWeight = source.hasFilteredWeight;
  state_.scale.weightGrams = source.filteredWeightGrams;
  state_.scale.stable = source.stable;
  state_.scale.hasNfcUid = source.hasStableNfcUid;
  state_.scale.nfcUid = source.stableNfcUid;
  state_.scale.hasBatteryPercentage = source.hasBatteryPercentage;
  state_.scale.batteryPercentage = source.batteryPercentage;
  state_.scale.hasRssiDbm = source.hasRssiDbm;
  state_.scale.rssiDbm = source.rssiDbm;
  state_.scale.nfcAmbiguous = source.nfcAmbiguous;
  if (source.linkStatus == models::ScaleLinkStatus::Online &&
      source.hasLastUpdate && kegService_ != nullptr) {
    const std::time_t now = std::time(nullptr);
    if (static_cast<std::int64_t>(now) >= kMinimumValidUtcSeconds) {
      const models::Keg* const active = kegService_->activeKeg();
      if (active != nullptr) {
        for (std::size_t index = 0U; index < state_.kegCount; ++index) {
          KegViewData& target = state_.kegs[index];
          if (target.id != nullptr &&
              std::strcmp(target.id, active->id.data()) == 0) {
            target.lastSyncUtc = static_cast<std::int64_t>(now);
            break;
          }
        }
      }
    }
  }
  scaleRevision_ = scaleService_->revision();
}

void AppViewModel::syncTemperature() {
  if (temperatureService_ == nullptr) {
    return;
  }
  const models::TemperatureState& source = temperatureService_->state();
  state_.freezer.hasTemperature = source.hasCurrentTemperature;
  if (source.hasCurrentTemperature) {
    state_.freezer.temperatureC =
        static_cast<float>(source.currentCentiCelsius) / 100.0F;
  }
  state_.freezer.setpointC =
      static_cast<float>(source.setpointCentiCelsius) / 100.0F;
  state_.freezer.compressorState =
      models::temperatureControlStateName(source.controlState);
  state_.freezer.compressorOn = source.compressorOn;
  state_.freezer.demandCooling = source.demandCooling;
  state_.freezer.sampleStatus = source.sampleStatus;
  state_.freezer.controlState = source.controlState;
  state_.freezer.temperatureFault = source.fault;
  state_.freezer.protectionRemainingMs = source.protectionRemainingMs;
  temperatureRevision_ = temperatureService_->revision();
}

void AppViewModel::syncNetwork() {
  if (networkService_ == nullptr) return;
  const models::NetworkState& source = networkService_->state();
  const models::NetworkSettings& settings = networkService_->settings();
  state_.network.status = source.status;
  state_.network.ssid = source.ssid;
  state_.network.hostname = source.hostname;
  state_.network.ipAddress = source.ipAddress;
  state_.network.scaleHost = settings.scaleHost;
  state_.network.scalePort = settings.scalePort;
  state_.network.rssiDbm = source.rssiDbm;
  state_.network.retryInMs = source.retryInMs;
  state_.network.configured = source.configured;
  state_.network.connected = source.connected;
  state_.network.hasRssi = source.hasRssi;
  networkRevision_ = source.revision;
}

NetworkSaveResult AppViewModel::saveNetworkSettings(
    const NetworkEditRequest& request) {
  if (networkService_ == nullptr) return NetworkSaveResult::ServiceError;
  models::NetworkSettings candidate{};
  std::snprintf(candidate.ssid.data(), candidate.ssid.size(), "%s",
                request.ssid == nullptr ? "" : request.ssid);
  std::snprintf(candidate.password.data(), candidate.password.size(), "%s",
                request.password == nullptr ? "" : request.password);
  std::snprintf(candidate.hostname.data(), candidate.hostname.size(), "%s",
                request.hostname == nullptr ? "" : request.hostname);
  std::snprintf(candidate.scaleHost.data(), candidate.scaleHost.size(), "%s",
                request.scaleHost == nullptr ? "" : request.scaleHost);
  candidate.scalePort = request.scalePort;
  if (candidate.password[0] == '\0') {
    candidate.password = networkService_->settings().password;
  }
  if (!services::NetworkService::validSettings(candidate)) {
    return NetworkSaveResult::InvalidData;
  }

  const models::NetworkSettings previous = networkService_->settings();
  if (storage::NetworkSettingsStore::save(candidate) !=
      storage::NetworkSettingsResult::Ok) {
    return NetworkSaveResult::StorageError;
  }
  const services::NetworkError applied =
      networkService_->applySettings(candidate, lastNowMs_);
  if (applied != services::NetworkError::None) {
    storage::NetworkSettingsStore::save(previous);
    networkService_->applySettings(previous, lastNowMs_);
    return NetworkSaveResult::ServiceError;
  }
  syncNetwork();
  ++state_.revision;
  KEEZER_LOG_INFO("NETWORK_UI",
                  "NETWORK_SETTINGS_SAVED ssid=%s host=%s port=%u",
                  candidate.ssid[0] == '\0' ? "<stored>" : candidate.ssid.data(),
                  candidate.scaleHost.data(),
                  static_cast<unsigned int>(candidate.scalePort));
  return NetworkSaveResult::Ok;
}

DisplaySaveResult AppViewModel::saveDisplaySettings(
    const std::uint8_t brightnessPercent,
    const std::uint16_t timeoutSeconds) {
  const models::DisplaySettings candidate{brightnessPercent, timeoutSeconds};
  if (brightnessPercent < 10U || brightnessPercent > 100U) {
    return DisplaySaveResult::InvalidData;
  }
  switch (timeoutSeconds) {
    case 0U:
    case 30U:
    case 60U:
    case 120U:
    case 300U:
      break;
    default:
      return DisplaySaveResult::InvalidData;
  }
  if (storage::DisplaySettingsStore::save(candidate) !=
      storage::DisplaySettingsResult::Ok) {
    return DisplaySaveResult::StorageError;
  }
  state_.display = candidate;
  ++state_.revision;
  return DisplaySaveResult::Ok;
}

TemperatureSetpointResult AppViewModel::setTemperatureSetpointCentiCelsius(
    const std::int16_t setpointCentiCelsius) {
  if (temperatureService_ == nullptr) {
    return TemperatureSetpointResult::ServiceError;
  }
  if (setpointCentiCelsius <
          config::kTemperatureMinimumSetpointCentiCelsius ||
      setpointCentiCelsius >
          config::kTemperatureMaximumSetpointCentiCelsius) {
    return TemperatureSetpointResult::OutOfRange;
  }

  const std::int16_t previousSetpoint =
      temperatureService_->state().setpointCentiCelsius;
  if (storage::TemperatureSettingsStore::saveSetpointCentiCelsius(
          setpointCentiCelsius) != storage::TemperatureSettingsResult::Ok) {
    KEEZER_LOG_ERROR(kTemperatureLogTag,
                     "Could not persist setpoint=%d", setpointCentiCelsius);
    return TemperatureSetpointResult::StorageError;
  }
  if (!temperatureService_->setSetpointCentiCelsius(setpointCentiCelsius)) {
    storage::TemperatureSettingsStore::saveSetpointCentiCelsius(
        previousSetpoint);
    return TemperatureSetpointResult::ServiceError;
  }
  syncTemperature();
  ++state_.revision;
  KEEZER_LOG_INFO(kTemperatureLogTag, "SETPOINT_SAVED value=%d.%02dC",
                  static_cast<int>(setpointCentiCelsius / 100),
                  static_cast<int>(setpointCentiCelsius >= 0
                                       ? setpointCentiCelsius % 100
                                       : -(setpointCentiCelsius % 100)));
  return TemperatureSetpointResult::Ok;
}

void AppViewModel::updateManualWeighing(const std::uint32_t nowMs) {
  ManualWeighingViewData& session = state_.manualWeighing;
  if (session.status != ManualWeighingStatus::WaitingForReading ||
      scaleService_ == nullptr || kegService_ == nullptr) {
    return;
  }
  if (nowMs - manualWeighingStartedAtMs_ >=
      config::kManualWeighingTimeoutMs) {
    session.status = ManualWeighingStatus::TimedOut;
    ++session.revision;
    ++state_.revision;
    return;
  }

  const models::ScaleState& scaleState = scaleService_->state();
  if (scaleState.linkStatus != models::ScaleLinkStatus::Online ||
      !scaleState.stable || !scaleState.hasFilteredWeight ||
      scaleState.acceptedReadingCount == manualWeighingStartReadingCount_) {
    return;
  }

  models::KegMeasurement result{};
  if (!kegService_->calculateMeasurement(
          session.kegId.data(), scaleState.filteredWeightGrams, result) ||
      result.validity == models::MeasurementValidity::InvalidWeight ||
      result.validity ==
          models::MeasurementValidity::InvalidKegConfiguration) {
    session.status = ManualWeighingStatus::Error;
  } else {
    session.status = ManualWeighingStatus::ReadyToConfirm;
    session.weightGrams = scaleState.filteredWeightGrams;
    session.volumeMl = result.volumeMl;
    session.percentageBasisPoints = result.percentageBasisPoints;
    session.validity = result.validity;
    const models::Keg* const nfcKeg =
        scaleState.hasStableNfcUid
            ? kegService_->findByNfcUid(scaleState.stableNfcUid.data())
            : nullptr;
    session.nfcConflict =
        nfcKeg != nullptr &&
        std::strcmp(nfcKeg->id.data(), session.kegId.data()) != 0;
  }
  ++session.revision;
  ++state_.revision;
}

}  // namespace keezer::app
