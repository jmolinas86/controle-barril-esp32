#include "services/HistoryService.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <new>

#include "diagnostics/Logger.h"

namespace keezer::services {
namespace {

constexpr char kLogTag[] = "HISTORY";
constexpr std::uint32_t kFlushIntervalMs = 60'000U;
constexpr std::uint32_t kStorageRecoveryIntervalMs = 15'000U;
constexpr std::uint32_t kMinimumPeriodicIntervalMs = 60'000U;
constexpr std::uint32_t kSignificantVolumeChangeMl = 50U;
constexpr std::int64_t kMinimumValidUtcSeconds = 1'577'836'800LL;

std::int64_t currentUtcSeconds() {
  const std::time_t now = std::time(nullptr);
  return static_cast<std::int64_t>(now) >= kMinimumValidUtcSeconds
             ? static_cast<std::int64_t>(now)
             : -1;
}

const char* sourceName(const models::HistorySource source) {
  switch (source) {
    case models::HistorySource::Nfc:
      return "NFC";
    case models::HistorySource::Manual:
      return "MANUAL";
    case models::HistorySource::AutomaticTracking:
      return "AUTO_TRACKING";
  }
  return "UNKNOWN";
}

}  // namespace

HistoryService::HistoryService(storage::IHistoryRepository& repository)
    : repository_(repository) {}

bool HistoryService::begin(const std::uint32_t nowMs,
                           const std::uint32_t bootId) {
  if (buffer_ == nullptr) {
    buffer_ = new (std::nothrow) models::HistoryRecord[kBufferCapacity]{};
  }
  bootId_ = bootId;
  nextFlushAtMs_ = nowMs + kFlushIntervalMs;
  const bool ready = repository_.begin(bootId);
  setStorageResult(ready ? storage::HistoryRepositoryResult::Ok
                         : repository_.lastResult(),
                   nowMs);
  if (buffer_ == nullptr) {
    KEEZER_LOG_ERROR(kLogTag, "History RAM buffer allocation failed");
  }
  KEEZER_LOG_INFO(kLogTag, "History repository=%s boot=%08lX",
                  ready ? "SD_READY" : "DEGRADED_RAM",
                  static_cast<unsigned long>(bootId));
  return ready && buffer_ != nullptr;
}

bool HistoryService::recordMeasurement(
    const char* const kegId, const models::KegMeasurement& measurement,
    const std::int32_t rawWeightGrams, const models::HistorySource source,
    const std::uint32_t nowMs) {
  if (kegId == nullptr || kegId[0] == '\0' ||
      measurement.validity == models::MeasurementValidity::InvalidWeight ||
      measurement.validity ==
          models::MeasurementValidity::InvalidKegConfiguration) {
    return false;
  }

  const std::uint32_t delta = measurement.volumeMl > lastVolumeMl_
                                  ? measurement.volumeMl - lastVolumeMl_
                                  : lastVolumeMl_ - measurement.volumeMl;
  const bool duplicateAutomatic =
      source != models::HistorySource::Manual && hasLastRecord_ &&
      std::strcmp(lastKegId_.data(), kegId) == 0 &&
      nowMs - lastAcceptedAtMs_ < kMinimumPeriodicIntervalMs &&
      delta < kSignificantVolumeChangeMl;
  if (duplicateAutomatic) {
    KEEZER_LOG_INFO(kLogTag,
                    "HISTORY_SKIPPED keg=%s reason=DEDUP volume=%luml", kegId,
                    static_cast<unsigned long>(measurement.volumeMl));
    return true;
  }

  models::HistoryRecord record{};
  std::snprintf(record.kegId.data(), record.kegId.size(), "%s", kegId);
  record.sequence = ++sequence_;
  record.bootId = bootId_;
  record.utcSeconds = currentUtcSeconds();
  record.monotonicMs = nowMs;
  record.rawWeightGrams = rawWeightGrams;
  record.filteredWeightGrams = measurement.filteredWeightGrams;
  record.volumeMl = measurement.volumeMl;
  record.percentageBasisPoints = measurement.percentageBasisPoints;
  record.source = source;
  record.validity = measurement.validity;
  if (!queue(record)) {
    return false;
  }

  std::snprintf(lastKegId_.data(), lastKegId_.size(), "%s", kegId);
  lastVolumeMl_ = measurement.volumeMl;
  lastAcceptedAtMs_ = nowMs;
  hasLastRecord_ = true;
  ++revision_;

  KEEZER_LOG_INFO(
      kLogTag,
      "HISTORY_QUEUED keg=%s seq=%lu volume=%luml source=%s pending=%u",
      kegId, static_cast<unsigned long>(record.sequence),
      static_cast<unsigned long>(record.volumeMl),
      sourceName(source),
      static_cast<unsigned int>(count_));

  // A confirmed weighing is a relevant transition. Persist it immediately;
  // the circular buffer remains the safe fallback when the card is absent.
  flush(nowMs);
  return true;
}

void HistoryService::update(const std::uint32_t nowMs) {
  if (count_ == 0U || static_cast<std::int32_t>(nowMs - nextFlushAtMs_) < 0) {
    return;
  }
  flush(nowMs);
  nextFlushAtMs_ = nowMs + kFlushIntervalMs;
}

bool HistoryService::recoveryDue(const std::uint32_t nowMs) const {
  return storageStatus_ != HistoryStorageStatus::Ready &&
         static_cast<std::int32_t>(nowMs - nextRecoveryAtMs_) >= 0;
}

bool HistoryService::retryStorage(const std::uint32_t nowMs) {
  const bool ready = repository_.begin(bootId_);
  setStorageResult(ready ? storage::HistoryRepositoryResult::Ok
                         : repository_.lastResult(),
                   nowMs);
  if (!ready) {
    KEEZER_LOG_WARN(kLogTag, "STORAGE_RECOVERY_WAIT status=%s pending=%u",
                    historyStorageStatusName(storageStatus_),
                    static_cast<unsigned int>(count_));
    return false;
  }
  KEEZER_LOG_INFO(kLogTag, "STORAGE_RECOVERED pending=%u",
                  static_cast<unsigned int>(count_));
  return flush(nowMs);
}

bool HistoryService::loadRecentVolumes(
    const char* const kegId,
    std::array<std::uint32_t, kGraphPointCount>& volumesMl,
    std::size_t& count) {
  volumesMl.fill(0U);
  count = 0U;
  std::array<models::HistoryRecord, kGraphPointCount> persisted{};
  std::size_t persistedCount = 0U;
  const storage::HistoryRepositoryResult result = repository_.loadRecent(
      kegId, persisted.data(), persisted.size(), persistedCount);
  if (result == storage::HistoryRepositoryResult::Ok ||
      result == storage::HistoryRepositoryResult::Corrupt) {
    for (std::size_t index = 0U; index < persistedCount; ++index) {
      volumesMl[count++] = persisted[index].volumeMl;
    }
  }

  for (std::size_t offset = 0U; offset < count_; ++offset) {
    const models::HistoryRecord& record =
        buffer_[(head_ + offset) % kBufferCapacity];
    if (std::strcmp(record.kegId.data(), kegId) != 0) {
      continue;
    }
    if (count == kGraphPointCount) {
      for (std::size_t index = 1U; index < count; ++index) {
        volumesMl[index - 1U] = volumesMl[index];
      }
      --count;
    }
    volumesMl[count++] = record.volumeMl;
  }
  return result == storage::HistoryRepositoryResult::Ok || count != 0U;
}

std::uint32_t HistoryService::revision() const { return revision_; }

bool HistoryService::storageReady() const {
  return storageStatus_ == HistoryStorageStatus::Ready && repository_.ready();
}

HistoryStorageStatus HistoryService::storageStatus() const {
  return storageStatus_;
}

std::size_t HistoryService::pendingCount() const { return count_; }

std::uint32_t HistoryService::droppedCount() const { return droppedCount_; }

bool HistoryService::queue(const models::HistoryRecord& record) {
  if (buffer_ == nullptr) {
    KEEZER_LOG_ERROR(kLogTag, "HISTORY_DROPPED reason=NO_RAM_BUFFER");
    ++droppedCount_;
    return false;
  }
  if (count_ == kBufferCapacity) {
    head_ = (head_ + 1U) % kBufferCapacity;
    --count_;
    ++droppedCount_;
    KEEZER_LOG_WARN(kLogTag, "HISTORY_BUFFER_OVERFLOW dropped=%lu",
                    static_cast<unsigned long>(droppedCount_));
  }
  const std::size_t tail = (head_ + count_) % kBufferCapacity;
  buffer_[tail] = record;
  ++count_;
  return true;
}

bool HistoryService::flush(const std::uint32_t nowMs) {
  bool wroteAny = false;
  while (count_ != 0U) {
    const storage::HistoryRepositoryResult result =
        repository_.append(buffer_[head_]);
    if (result != storage::HistoryRepositoryResult::Ok) {
      if (wroteAny) {
        ++revision_;
      }
      KEEZER_LOG_WARN(kLogTag, "HISTORY_FLUSH_DEFERRED pending=%u error=%u",
                      static_cast<unsigned int>(count_),
                      static_cast<unsigned int>(result));
      setStorageResult(result, nowMs);
      return false;
    }
    head_ = (head_ + 1U) % kBufferCapacity;
    --count_;
    wroteAny = true;
  }
  if (wroteAny) {
    ++revision_;
    KEEZER_LOG_INFO(kLogTag, "HISTORY_FLUSHED storage=SD");
  }
  return true;
}

void HistoryService::setStorageResult(
    const storage::HistoryRepositoryResult result,
    const std::uint32_t nowMs) {
  HistoryStorageStatus status = HistoryStorageStatus::IoError;
  switch (result) {
    case storage::HistoryRepositoryResult::Ok:
      status = HistoryStorageStatus::Ready;
      break;
    case storage::HistoryRepositoryResult::NotReady:
      status = HistoryStorageStatus::Missing;
      break;
    case storage::HistoryRepositoryResult::Full:
      status = HistoryStorageStatus::Full;
      break;
    case storage::HistoryRepositoryResult::Corrupt:
    case storage::HistoryRepositoryResult::InvalidData:
    case storage::HistoryRepositoryResult::IoError:
      status = HistoryStorageStatus::IoError;
      break;
  }
  if (storageStatus_ != status) {
    storageStatus_ = status;
    ++revision_;
    KEEZER_LOG_WARN(kLogTag, "STORAGE_STATE status=%s pending=%u",
                    historyStorageStatusName(storageStatus_),
                    static_cast<unsigned int>(count_));
  }
  if (status != HistoryStorageStatus::Ready) {
    nextRecoveryAtMs_ = nowMs + kStorageRecoveryIntervalMs;
  }
}

const char* historyStorageStatusName(const HistoryStorageStatus status) {
  switch (status) {
    case HistoryStorageStatus::Ready:
      return "SD_OK";
    case HistoryStorageStatus::Missing:
      return "SD_MISSING";
    case HistoryStorageStatus::Full:
      return "SD_FULL";
    case HistoryStorageStatus::IoError:
      return "SD_ERROR";
  }
  return "SD_ERROR";
}

}  // namespace keezer::services
