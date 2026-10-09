#pragma once

#include <cstddef>
#include <cstdint>

#include "models/History.h"

namespace keezer::storage {

enum class HistoryRepositoryResult : std::uint8_t {
  Ok = 0U,
  NotReady,
  Full,
  Corrupt,
  InvalidData,
  IoError,
};

const char* historyRepositoryResultName(HistoryRepositoryResult result);

class IHistoryRepository {
 public:
  virtual ~IHistoryRepository() = default;
  virtual bool begin(std::uint32_t bootId) = 0;
  virtual HistoryRepositoryResult append(
      const models::HistoryRecord& record) = 0;
  virtual HistoryRepositoryResult loadRecent(
      const char* kegId, models::HistoryRecord* records,
      std::size_t capacity, std::size_t& count) = 0;
  virtual bool ready() const = 0;
  virtual HistoryRepositoryResult lastResult() const {
    return ready() ? HistoryRepositoryResult::Ok
                   : HistoryRepositoryResult::NotReady;
  }
};

}  // namespace keezer::storage
