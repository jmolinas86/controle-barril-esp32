#pragma once

#include <cstddef>
#include <cstdint>

#include "models/Keg.h"
#include "models/KegMeasurement.h"
#include "storage/IKegRepository.h"

namespace keezer::services {

enum class KegError {
  None,
  InvalidData,
  CatalogFull,
  NotFound,
  DuplicateId,
  DuplicateNfc,
  Unauthorized,
  ActivationRequiresConfirmation,
  StorageError,
};

enum class ActivationSource { Manual, Nfc, AutomaticTracking };
enum class RemovalReason { Manual, ConfirmedSignals, ReplacedByAnotherKeg };

class KegService final {
 public:
  explicit KegService(storage::IKegRepository& repository);

  bool begin();
  KegError createKeg(const models::KegDraft& draft);
  KegError updateKeg(const char* id, const models::KegDraft& draft);
  KegError archiveKeg(const char* id);
  KegError deleteKeg(const char* id, bool authorized);
  KegError finishKeg(const char* id);
  KegError markEmpty(const char* id);
  KegError attachNfcTag(const char* id, const char* uid);
  KegError detachNfcTag(const char* id);
  KegError setActiveKeg(const char* id, ActivationSource source,
                        bool confirmed = false);
  KegError removeActiveKeg(RemovalReason reason = RemovalReason::Manual);

  const models::Keg* findById(const char* id) const;
  const models::Keg* findByNfcUid(const char* uid) const;
  const models::Keg* activeKeg() const;
  bool calculateMeasurement(const char* id, std::int32_t filteredWeightGrams,
                            models::KegMeasurement& result) const;
  KegError recordMeasurement(const char* id,
                             std::int32_t filteredWeightGrams,
                             ActivationSource source, bool confirmed,
                             models::KegMeasurement& result);
  const models::Keg* at(std::size_t index) const;
  std::size_t count() const;
  std::uint32_t revision() const;
  bool storageReady() const;
  bool catalogAvailable() const;

 private:
  static bool validDraft(const models::KegDraft& draft);
  static bool normalizeUid(const char* source,
                           char (&destination)[models::kNfcUidBytes]);
  static void applyDraft(models::Keg& keg, const models::KegDraft& draft,
                         const char* normalizedUid);
  int indexById(const char* id) const;
  int indexByUid(const char* normalizedUid) const;
  KegError persist();
  void reconcileActiveKeg();

  storage::IKegRepository& repository_;
  models::KegCatalog catalog_{};
  std::uint32_t revision_{0U};
  bool storageReady_{false};
  bool catalogAvailable_{false};
};

const char* kegErrorName(KegError error);

}  // namespace keezer::services
