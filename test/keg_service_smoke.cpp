#include <cassert>
#include <cstdarg>
#include <cstdio>

#include "diagnostics/Logger.h"
#include "services/KegService.h"
#include "storage/MemoryKegRepository.h"

namespace keezer::diagnostics {
LogLevel Logger::currentLevel_ = LogLevel::Error;
void Logger::begin(std::uint32_t) {}
void Logger::setLevel(const LogLevel level) { currentLevel_ = level; }
LogLevel Logger::level() { return currentLevel_; }
void Logger::log(LogLevel, const char*, const char*, ...) {}
const char* Logger::levelName(LogLevel) { return "TEST"; }
}  // namespace keezer::diagnostics

namespace {

keezer::models::KegDraft draft(const char* id, const char* uid,
                               const char* name) {
  return {id, uid, name, "PILSNER", "L001", "", 20'000U, 4'350,
          1'000U, 18'550, 14'200U};
}

std::size_t activeCount(const keezer::services::KegService& service) {
  std::size_t count = 0U;
  for (std::size_t index = 0U; index < service.count(); ++index) {
    const keezer::models::Keg* const keg = service.at(index);
    if (keg != nullptr &&
        keg->status == keezer::models::KegStatus::ActiveOnScale) {
      ++count;
    }
  }
  return count;
}

}  // namespace

int main() {
  using keezer::models::KegStatus;
  using keezer::services::ActivationSource;
  using keezer::services::KegError;

  keezer::storage::MemoryKegRepository repository;
  keezer::services::KegService service(repository);
  assert(service.begin());
  assert(service.createKeg(draft("KEG_001", "04:a2-3f 891c", "ONE")) ==
         KegError::None);
  assert(service.createKeg(draft("KEG_002", "04B17D2210", "TWO")) ==
         KegError::None);
  assert(service.createKeg(draft("KEG_001", "04C09E118A", "DUP")) ==
         KegError::DuplicateId);
  assert(service.createKeg(draft("KEG_003", "04A23F891C", "DUP NFC")) ==
         KegError::DuplicateNfc);
  assert(service.findByNfcUid("04-A2-3F-89-1C") != nullptr);

  assert(service.setActiveKeg("KEG_001", ActivationSource::Manual) ==
         KegError::None);
  assert(service.setActiveKeg("KEG_002", ActivationSource::Nfc) ==
         KegError::None);
  assert(service.activeKeg() != nullptr);
  assert(service.findById("KEG_001")->status == KegStatus::Stored);
  assert(service.findById("KEG_002")->status == KegStatus::ActiveOnScale);
  assert(activeCount(service) == 1U);
  keezer::models::KegMeasurement measurement{};
  assert(service.calculateMeasurement("KEG_001", 18'550, measurement));
  assert(measurement.volumeMl == 14'200U);
  assert(measurement.percentageBasisPoints == 7'100U);
  assert(service.recordMeasurement("KEG_001", 15'550,
                                   ActivationSource::Manual, true,
                                   measurement) == KegError::None);
  assert(measurement.volumeMl == 11'200U);
  assert(service.activeKeg() != nullptr);
  assert(service.findById("KEG_001")->status == KegStatus::ActiveOnScale);
  assert(service.findById("KEG_002")->status == KegStatus::Stored);
  assert(activeCount(service) == 1U);

  assert(service.archiveKeg("KEG_002") == KegError::None);
  assert(service.activeKeg() != nullptr);
  assert(service.findById("KEG_001")->status == KegStatus::ActiveOnScale);
  assert(service.removeActiveKeg(
             keezer::services::RemovalReason::ConfirmedSignals) ==
         KegError::None);
  assert(service.activeKeg() == nullptr);
  assert(service.findById("KEG_001")->status == KegStatus::Stored);
  assert(activeCount(service) == 0U);
  assert(service.setActiveKeg("KEG_002", ActivationSource::Nfc) ==
         KegError::ActivationRequiresConfirmation);
  assert(service.setActiveKeg("KEG_002", ActivationSource::Manual, true) ==
         KegError::None);
  assert(activeCount(service) == 1U);

  keezer::services::KegService restored(repository);
  assert(restored.begin());
  assert(restored.count() == 2U);
  assert(restored.activeKeg() != nullptr);
  assert(restored.deleteKeg("KEG_001", false) ==
         KegError::Unauthorized);
  assert(restored.deleteKeg("KEG_001", true) == KegError::None);
  assert(restored.count() == 1U);

  keezer::storage::MemoryKegRepository inconsistentRepository;
  assert(inconsistentRepository.begin());
  keezer::models::KegCatalog inconsistent{};
  inconsistent.count = 2U;
  std::snprintf(inconsistent.kegs[0].id.data(),
                inconsistent.kegs[0].id.size(), "%s", "KEG_A");
  std::snprintf(inconsistent.kegs[1].id.data(),
                inconsistent.kegs[1].id.size(), "%s", "KEG_B");
  inconsistent.kegs[0].status = KegStatus::ActiveOnScale;
  inconsistent.kegs[1].status = KegStatus::ActiveOnScale;
  std::snprintf(inconsistent.activeKegId.data(),
                inconsistent.activeKegId.size(), "%s", "KEG_A");
  assert(inconsistentRepository.save(inconsistent) ==
         keezer::storage::RepositoryResult::Ok);
  keezer::services::KegService reconciled(inconsistentRepository);
  assert(reconciled.begin());
  assert(reconciled.activeKeg() != nullptr);
  assert(activeCount(reconciled) == 1U);
  assert(reconciled.findById("KEG_A")->status == KegStatus::ActiveOnScale);
  assert(reconciled.findById("KEG_B")->status == KegStatus::Stored);

  std::puts("KegService smoke test: PASS");
  return 0;
}
