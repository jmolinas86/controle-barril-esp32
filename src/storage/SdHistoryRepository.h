#pragma once

#include <FS.h>

#include <cstdint>

#include "storage/IHistoryRepository.h"

namespace keezer::storage {

struct SdPerformanceStats final {
  std::uint32_t writeAttempts{0U};
  std::uint32_t readAttempts{0U};
  std::uint32_t failedOperations{0U};
  std::uint64_t bytesWritten{0U};
  std::uint64_t bytesRead{0U};
  std::uint64_t operationTotalUs{0U};
  std::uint32_t operationMaxUs{0U};
};

class SdHistoryRepository final : public IHistoryRepository {
 public:
  bool begin(std::uint32_t bootId) override;
  HistoryRepositoryResult append(
      const models::HistoryRecord& record) override;
  HistoryRepositoryResult loadRecent(
      const char* kegId, models::HistoryRecord* records,
      std::size_t capacity, std::size_t& count) override;
  bool ready() const override;
  HistoryRepositoryResult lastResult() const override;
  SdPerformanceStats takePerformanceStats();

 private:
  static bool validKegId(const char* kegId);
  static bool readLine(File& file, char* destination, std::size_t size);
  static bool parseRecord(const char* line, const char* kegId,
                          models::HistoryRecord& record);
  bool ensureKegDirectory(const char* kegId) const;
  void buildFilePath(const char* kegId, char* destination,
                     std::size_t size) const;

  std::uint32_t bootId_{0U};
  bool ready_{false};
  HistoryRepositoryResult lastResult_{HistoryRepositoryResult::NotReady};
  SdPerformanceStats performance_{};
};

}  // namespace keezer::storage
