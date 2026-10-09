#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "diagnostics/Logger.h"
#include "services/HistoryService.h"

namespace keezer::diagnostics {
LogLevel Logger::currentLevel_ = LogLevel::Error;
void Logger::begin(std::uint32_t) {}
void Logger::setLevel(const LogLevel level) { currentLevel_ = level; }
LogLevel Logger::level() { return currentLevel_; }
void Logger::log(LogLevel, const char*, const char*, ...) {}
const char* Logger::levelName(LogLevel) { return "TEST"; }
}  // namespace keezer::diagnostics

namespace {

class FakeHistoryRepository final
    : public keezer::storage::IHistoryRepository {
 public:
  bool begin(std::uint32_t) override {
    ready_ = true;
    return true;
  }

  keezer::storage::HistoryRepositoryResult append(
      const keezer::models::HistoryRecord& record) override {
    if (!ready_) {
      return keezer::storage::HistoryRepositoryResult::NotReady;
    }
    assert(count_ < records_.size());
    records_[count_++] = record;
    return keezer::storage::HistoryRepositoryResult::Ok;
  }

  keezer::storage::HistoryRepositoryResult loadRecent(
      const char* kegId, keezer::models::HistoryRecord* records,
      std::size_t capacity, std::size_t& count) override {
    count = 0U;
    if (!ready_) {
      return keezer::storage::HistoryRepositoryResult::NotReady;
    }
    for (std::size_t index = 0U; index < count_; ++index) {
      if (std::strcmp(records_[index].kegId.data(), kegId) != 0) {
        continue;
      }
      if (count == capacity) {
        for (std::size_t point = 1U; point < count; ++point) {
          records[point - 1U] = records[point];
        }
        --count;
      }
      records[count++] = records_[index];
    }
    return keezer::storage::HistoryRepositoryResult::Ok;
  }

  bool ready() const override { return ready_; }
  void fail() { ready_ = false; }
  std::size_t count() const { return count_; }

 private:
  std::array<keezer::models::HistoryRecord, 16U> records_{};
  std::size_t count_{0U};
  bool ready_{false};
};

keezer::models::KegMeasurement measurement(const std::uint32_t volumeMl) {
  keezer::models::KegMeasurement result{};
  result.filteredWeightGrams = static_cast<std::int32_t>(volumeMl + 4'350U);
  result.beerWeightGrams = volumeMl;
  result.unclampedVolumeMl = volumeMl;
  result.volumeMl = volumeMl;
  result.percentageBasisPoints =
      static_cast<std::uint16_t>(volumeMl / 2U);
  result.validity = keezer::models::MeasurementValidity::Valid;
  return result;
}

}  // namespace

int main() {
  FakeHistoryRepository repository;
  keezer::services::HistoryService service(repository);
  assert(service.begin(0U, 0x12345678U));

  assert(service.recordMeasurement("KEG_001", measurement(14'200U), 18'550,
                                   keezer::models::HistorySource::Nfc,
                                   1'000U));
  assert(repository.count() == 1U);
  assert(service.pendingCount() == 0U);

  // Same automatic result inside the one-minute window is deduplicated.
  assert(service.recordMeasurement("KEG_001", measurement(14'200U), 18'550,
                                   keezer::models::HistorySource::Nfc,
                                   2'000U));
  assert(repository.count() == 1U);

  // A 50 mL change and an explicit manual confirmation are both kept.
  assert(service.recordMeasurement("KEG_001", measurement(14'150U), 18'500,
                                   keezer::models::HistorySource::Nfc,
                                   3'000U));
  assert(service.recordMeasurement("KEG_001", measurement(14'150U), 18'500,
                                   keezer::models::HistorySource::Manual,
                                   4'000U));
  assert(repository.count() == 3U);

  assert(service.recordMeasurement(
      "KEG_001", measurement(14'000U), 18'350,
      keezer::models::HistorySource::AutomaticTracking, 35'000U));
  assert(repository.count() == 4U);

  repository.fail();
  assert(service.recordMeasurement("KEG_001", measurement(14'000U), 18'350,
                                   keezer::models::HistorySource::Manual,
                                   36'000U));
  assert(service.pendingCount() == 1U);
  assert(!service.storageReady());
  assert(!service.recoveryDue(50'999U));
  assert(service.recoveryDue(51'000U));
  assert(service.retryStorage(51'000U));
  assert(service.storageReady());
  assert(service.pendingCount() == 0U);
  assert(repository.count() == 5U);

  std::array<std::uint32_t,
             keezer::services::HistoryService::kGraphPointCount>
      volumes{};
  std::size_t count = 0U;
  assert(service.loadRecentVolumes("KEG_001", volumes, count));
  assert(count == 5U);
  assert(volumes[count - 1U] == 14'000U);

  std::puts("HistoryService smoke test: PASS");
  return 0;
}
