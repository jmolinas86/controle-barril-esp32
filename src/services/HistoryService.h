#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "models/History.h"
#include "storage/IHistoryRepository.h"

namespace keezer::services {

enum class HistoryStorageStatus : std::uint8_t {
  Ready = 0U,
  Missing,
  Full,
  IoError,
};

const char* historyStorageStatusName(HistoryStorageStatus status);

class HistoryService final {
 public:
  static constexpr std::size_t kBufferCapacity = 64U;
  static constexpr std::size_t kGraphPointCount = 7U;

  explicit HistoryService(storage::IHistoryRepository& repository);
  bool begin(std::uint32_t nowMs, std::uint32_t bootId);
  bool recordMeasurement(const char* kegId,
                         const models::KegMeasurement& measurement,
                         std::int32_t rawWeightGrams,
                         models::HistorySource source,
                         std::uint32_t nowMs);
  void update(std::uint32_t nowMs);
  bool recoveryDue(std::uint32_t nowMs) const;
  bool retryStorage(std::uint32_t nowMs);
  bool loadRecentVolumes(
      const char* kegId,
      std::array<std::uint32_t, kGraphPointCount>& volumesMl,
      std::size_t& count);
  std::uint32_t revision() const;
  bool storageReady() const;
  HistoryStorageStatus storageStatus() const;
  std::size_t pendingCount() const;
  std::uint32_t droppedCount() const;

 private:
  bool queue(const models::HistoryRecord& record);
  bool flush(std::uint32_t nowMs);
  void setStorageResult(storage::HistoryRepositoryResult result,
                        std::uint32_t nowMs);

  storage::IHistoryRepository& repository_;
  models::HistoryRecord* buffer_{nullptr};
  std::size_t head_{0U};
  std::size_t count_{0U};
  std::uint32_t bootId_{0U};
  std::uint32_t sequence_{0U};
  std::uint32_t revision_{0U};
  std::uint32_t droppedCount_{0U};
  std::uint32_t nextFlushAtMs_{0U};
  std::uint32_t nextRecoveryAtMs_{0U};
  HistoryStorageStatus storageStatus_{HistoryStorageStatus::Missing};
  std::array<char, models::kKegIdBytes> lastKegId_{};
  std::uint32_t lastVolumeMl_{0U};
  std::uint32_t lastAcceptedAtMs_{0U};
  bool hasLastRecord_{false};
};

}  // namespace keezer::services
