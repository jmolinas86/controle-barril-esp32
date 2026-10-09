#include "services/KegService.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <ctime>

#include "diagnostics/Logger.h"
#include "domain/KegCalculator.h"

namespace keezer::services {
namespace {

constexpr char kLogTag[] = "KEG_SERVICE";
constexpr std::int64_t kMinimumValidUtcSeconds = 1'577'836'800LL;

std::int64_t currentUtcSeconds() {
  const std::time_t now = std::time(nullptr);
  return static_cast<std::int64_t>(now) >= kMinimumValidUtcSeconds
             ? static_cast<std::int64_t>(now)
             : models::kUnknownTimestamp;
}

template <std::size_t Size>
void copyText(std::array<char, Size>& destination, const char* source) {
  std::snprintf(destination.data(), destination.size(), "%s",
                source == nullptr ? "" : source);
}

bool sameText(const char* lhs, const char* rhs) {
  return lhs != nullptr && rhs != nullptr && std::strcmp(lhs, rhs) == 0;
}

bool validOptionalText(const char* text, const std::size_t maximumBytes) {
  return text == nullptr || std::strlen(text) < maximumBytes;
}

bool validId(const char* id) {
  if (id == nullptr || id[0] == '\0' ||
      std::strlen(id) >= models::kKegIdBytes) {
    return false;
  }
  for (std::size_t index = 0U; id[index] != '\0'; ++index) {
    const unsigned char character = static_cast<unsigned char>(id[index]);
    if (!std::isalnum(character) && character != '_' && character != '-') {
      return false;
    }
  }
  return true;
}

const char* activationSourceName(const ActivationSource source) {
  switch (source) {
    case ActivationSource::Manual:
      return "MANUAL";
    case ActivationSource::Nfc:
      return "NFC";
    case ActivationSource::AutomaticTracking:
      return "AUTO_TRACKING";
  }
  return "UNKNOWN";
}

}  // namespace

KegService::KegService(storage::IKegRepository& repository)
    : repository_(repository) {}

bool KegService::begin() {
  catalogAvailable_ = false;
  storageReady_ = repository_.begin();
  if (!storageReady_) {
    catalog_ = {};
    ++revision_;
    return false;
  }
  const storage::RepositoryResult result = repository_.load(catalog_);
  if (result == storage::RepositoryResult::NotFound) {
    catalog_ = {};
    catalogAvailable_ = true;
    ++revision_;
    return true;
  }
  if (result != storage::RepositoryResult::Ok) {
    KEEZER_LOG_ERROR(kLogTag, "Catalog load failed: %u",
                     static_cast<unsigned int>(result));
    catalog_ = {};
    ++revision_;
    return false;
  }
  reconcileActiveKeg();
  catalogAvailable_ = true;
  ++revision_;
  return true;
}

KegError KegService::createKeg(const models::KegDraft& draft) {
  if (!validDraft(draft)) {
    return KegError::InvalidData;
  }
  if (catalog_.count >= models::kMaximumKegs) {
    return KegError::CatalogFull;
  }
  char normalizedUid[models::kNfcUidBytes]{};
  if (!normalizeUid(draft.nfcUid, normalizedUid)) {
    return KegError::InvalidData;
  }
  if (indexById(draft.id) >= 0) {
    return KegError::DuplicateId;
  }
  if (normalizedUid[0] != '\0' && indexByUid(normalizedUid) >= 0) {
    return KegError::DuplicateNfc;
  }

  models::Keg& keg = catalog_.kegs[catalog_.count];
  keg = {};
  applyDraft(keg, draft, normalizedUid);
  keg.status = models::KegStatus::Available;
  ++catalog_.count;
  const KegError saved = persist();
  if (saved != KegError::None) {
    --catalog_.count;
    catalog_.kegs[catalog_.count] = {};
    return saved;
  }
  ++revision_;
  KEEZER_LOG_INFO(kLogTag, "Created %s (%s)", keg.id.data(),
                  keg.name.data());
  return KegError::None;
}

KegError KegService::updateKeg(const char* id,
                               const models::KegDraft& draft) {
  const int index = indexById(id);
  if (index < 0) {
    return KegError::NotFound;
  }
  if (!validDraft(draft)) {
    return KegError::InvalidData;
  }
  char normalizedUid[models::kNfcUidBytes]{};
  if (!normalizeUid(draft.nfcUid, normalizedUid)) {
    return KegError::InvalidData;
  }
  const int duplicateId = indexById(draft.id);
  if (duplicateId >= 0 && duplicateId != index) {
    return KegError::DuplicateId;
  }
  const int duplicateUid = indexByUid(normalizedUid);
  if (normalizedUid[0] != '\0' && duplicateUid >= 0 &&
      duplicateUid != index) {
    return KegError::DuplicateNfc;
  }

  models::Keg& keg = catalog_.kegs[static_cast<std::size_t>(index)];
  const models::Keg previous = keg;
  const bool active = keg.status == models::KegStatus::ActiveOnScale;
  applyDraft(keg, draft, normalizedUid);
  keg.status = previous.status;
  keg.createdAtUtc = previous.createdAtUtc;
  keg.activatedAtUtc = previous.activatedAtUtc;
  keg.lastSeenAtUtc = previous.lastSeenAtUtc;
  if (active) {
    copyText(catalog_.activeKegId, keg.id.data());
  }
  const KegError saved = persist();
  if (saved != KegError::None) {
    keg = previous;
    if (active) {
      copyText(catalog_.activeKegId, previous.id.data());
    }
    return saved;
  }
  ++revision_;
  return KegError::None;
}

KegError KegService::archiveKeg(const char* id) {
  const int index = indexById(id);
  if (index < 0) {
    return KegError::NotFound;
  }
  models::Keg& keg = catalog_.kegs[static_cast<std::size_t>(index)];
  const models::KegStatus previous = keg.status;
  const auto previousActive = catalog_.activeKegId;
  keg.status = models::KegStatus::Archived;
  if (sameText(catalog_.activeKegId.data(), keg.id.data())) {
    catalog_.activeKegId = {};
  }
  const KegError saved = persist();
  if (saved != KegError::None) {
    keg.status = previous;
    catalog_.activeKegId = previousActive;
    return saved;
  }
  ++revision_;
  return KegError::None;
}

KegError KegService::deleteKeg(const char* id, const bool authorized) {
  if (!authorized) {
    return KegError::Unauthorized;
  }
  const int index = indexById(id);
  if (index < 0) {
    return KegError::NotFound;
  }
  const std::size_t removedIndex = static_cast<std::size_t>(index);
  const bool wasActive =
      sameText(catalog_.activeKegId.data(), catalog_.kegs[removedIndex].id.data());
  const models::Keg removed = catalog_.kegs[removedIndex];
  for (std::size_t current = removedIndex; current + 1U < catalog_.count;
       ++current) {
    catalog_.kegs[current] = catalog_.kegs[current + 1U];
  }
  --catalog_.count;
  catalog_.kegs[catalog_.count] = {};
  if (wasActive) {
    catalog_.activeKegId = {};
  }
  const KegError saved = persist();
  if (saved != KegError::None) {
    for (std::size_t current = catalog_.count; current > removedIndex;
         --current) {
      catalog_.kegs[current] = catalog_.kegs[current - 1U];
    }
    catalog_.kegs[removedIndex] = removed;
    ++catalog_.count;
    if (wasActive) {
      copyText(catalog_.activeKegId, removed.id.data());
    }
    return saved;
  }
  ++revision_;
  return KegError::None;
}

KegError KegService::finishKeg(const char* id) {
  const int index = indexById(id);
  if (index < 0) {
    return KegError::NotFound;
  }
  models::Keg& keg = catalog_.kegs[static_cast<std::size_t>(index)];
  const models::KegStatus previous = keg.status;
  const auto previousActive = catalog_.activeKegId;
  keg.status = models::KegStatus::Finished;
  if (sameText(catalog_.activeKegId.data(), keg.id.data())) {
    catalog_.activeKegId = {};
  }
  const KegError saved = persist();
  if (saved != KegError::None) {
    keg.status = previous;
    catalog_.activeKegId = previousActive;
    return saved;
  }
  ++revision_;
  return KegError::None;
}

KegError KegService::markEmpty(const char* id) {
  const int index = indexById(id);
  if (index < 0) {
    return KegError::NotFound;
  }
  models::Keg& keg = catalog_.kegs[static_cast<std::size_t>(index)];
  const models::KegStatus previous = keg.status;
  const auto previousActive = catalog_.activeKegId;
  keg.status = models::KegStatus::Empty;
  if (sameText(catalog_.activeKegId.data(), keg.id.data())) {
    catalog_.activeKegId = {};
  }
  const KegError saved = persist();
  if (saved != KegError::None) {
    keg.status = previous;
    catalog_.activeKegId = previousActive;
    return saved;
  }
  ++revision_;
  return KegError::None;
}

KegError KegService::attachNfcTag(const char* id, const char* uid) {
  const int index = indexById(id);
  if (index < 0) {
    return KegError::NotFound;
  }
  char normalizedUid[models::kNfcUidBytes]{};
  if (!normalizeUid(uid, normalizedUid) || normalizedUid[0] == '\0') {
    return KegError::InvalidData;
  }
  const int duplicate = indexByUid(normalizedUid);
  if (duplicate >= 0 && duplicate != index) {
    return KegError::DuplicateNfc;
  }
  models::Keg& keg = catalog_.kegs[static_cast<std::size_t>(index)];
  const auto previous = keg.nfcUid;
  copyText(keg.nfcUid, normalizedUid);
  const KegError saved = persist();
  if (saved != KegError::None) {
    keg.nfcUid = previous;
    return saved;
  }
  ++revision_;
  return KegError::None;
}

KegError KegService::detachNfcTag(const char* id) {
  const int index = indexById(id);
  if (index < 0) {
    return KegError::NotFound;
  }
  models::Keg& keg = catalog_.kegs[static_cast<std::size_t>(index)];
  const auto previous = keg.nfcUid;
  keg.nfcUid = {};
  const KegError saved = persist();
  if (saved != KegError::None) {
    keg.nfcUid = previous;
    return saved;
  }
  ++revision_;
  return KegError::None;
}

KegError KegService::setActiveKeg(const char* id,
                                  const ActivationSource source,
                                  const bool confirmed) {
  (void)source;
  const int targetIndex = indexById(id);
  if (targetIndex < 0) {
    return KegError::NotFound;
  }
  models::Keg& target = catalog_.kegs[static_cast<std::size_t>(targetIndex)];
  if (!models::canActivateAutomatically(target.status) && !confirmed) {
    return KegError::ActivationRequiresConfirmation;
  }
  if (target.status == models::KegStatus::ActiveOnScale &&
      sameText(catalog_.activeKegId.data(), target.id.data())) {
    return KegError::None;
  }

  const auto previousActiveId = catalog_.activeKegId;
  const models::KegStatus previousTargetStatus = target.status;
  int oldIndex = indexById(catalog_.activeKegId.data());
  models::KegStatus previousOldStatus = models::KegStatus::Stored;
  if (oldIndex >= 0 && oldIndex != targetIndex) {
    models::Keg& old = catalog_.kegs[static_cast<std::size_t>(oldIndex)];
    previousOldStatus = old.status;
    old.status = models::KegStatus::Stored;
  }
  target.status = models::KegStatus::ActiveOnScale;
  copyText(catalog_.activeKegId, target.id.data());
  const KegError saved = persist();
  if (saved != KegError::None) {
    target.status = previousTargetStatus;
    if (oldIndex >= 0 && oldIndex != targetIndex) {
      catalog_.kegs[static_cast<std::size_t>(oldIndex)].status =
          previousOldStatus;
    }
    catalog_.activeKegId = previousActiveId;
    return saved;
  }
  ++revision_;
  return KegError::None;
}

KegError KegService::removeActiveKeg(const RemovalReason reason) {
  const int index = indexById(catalog_.activeKegId.data());
  if (index < 0) {
    catalog_.activeKegId = {};
    return KegError::None;
  }
  models::Keg& keg = catalog_.kegs[static_cast<std::size_t>(index)];
  const models::KegStatus previous = keg.status;
  const auto previousActive = catalog_.activeKegId;
  keg.status = models::KegStatus::Stored;
  catalog_.activeKegId = {};
  const KegError saved = persist();
  if (saved != KegError::None) {
    keg.status = previous;
    catalog_.activeKegId = previousActive;
    return saved;
  }
  ++revision_;
  const char* reasonText = "MANUAL";
  if (reason == RemovalReason::ConfirmedSignals) {
    reasonText = "CONFIRMED_SIGNALS";
  } else if (reason == RemovalReason::ReplacedByAnotherKeg) {
    reasonText = "REPLACED";
  }
  KEEZER_LOG_INFO(kLogTag, "Removed from scale: keg=%s reason=%s",
                  keg.id.data(), reasonText);
  return KegError::None;
}

const models::Keg* KegService::findById(const char* id) const {
  const int index = indexById(id);
  return index < 0 ? nullptr : &catalog_.kegs[static_cast<std::size_t>(index)];
}

const models::Keg* KegService::findByNfcUid(const char* uid) const {
  char normalizedUid[models::kNfcUidBytes]{};
  if (!normalizeUid(uid, normalizedUid) || normalizedUid[0] == '\0') {
    return nullptr;
  }
  const int index = indexByUid(normalizedUid);
  return index < 0 ? nullptr : &catalog_.kegs[static_cast<std::size_t>(index)];
}

const models::Keg* KegService::activeKeg() const {
  return findById(catalog_.activeKegId.data());
}

bool KegService::calculateMeasurement(
    const char* id, const std::int32_t filteredWeightGrams,
    models::KegMeasurement& result) const {
  const models::Keg* keg = findById(id);
  if (keg == nullptr) {
    return false;
  }
  result = domain::KegCalculator::calculate(*keg, filteredWeightGrams);
  return true;
}

KegError KegService::recordMeasurement(
    const char* const id, const std::int32_t filteredWeightGrams,
    const ActivationSource source, const bool confirmed,
    models::KegMeasurement& result) {
  const int targetIndex = indexById(id);
  if (targetIndex < 0 ||
      !calculateMeasurement(id, filteredWeightGrams, result)) {
    return KegError::NotFound;
  }
  if (result.validity == models::MeasurementValidity::InvalidWeight ||
      result.validity ==
          models::MeasurementValidity::InvalidKegConfiguration) {
    KEEZER_LOG_WARN(kLogTag,
                    "Measurement rejected: keg=%s weight=%ldg validity=%s",
                    id == nullptr ? "NONE" : id,
                    static_cast<long>(filteredWeightGrams),
                    models::measurementValidityName(result.validity));
    return KegError::InvalidData;
  }

  models::Keg& target =
      catalog_.kegs[static_cast<std::size_t>(targetIndex)];
  if (!models::canActivateAutomatically(target.status) && !confirmed) {
    return KegError::ActivationRequiresConfirmation;
  }

  const auto previousActiveId = catalog_.activeKegId;
  const models::Keg previousTarget = target;
  const int oldIndex = indexById(catalog_.activeKegId.data());
  models::KegStatus previousOldStatus = models::KegStatus::Stored;
  if (oldIndex >= 0 && oldIndex != targetIndex) {
    models::Keg& old = catalog_.kegs[static_cast<std::size_t>(oldIndex)];
    previousOldStatus = old.status;
    old.status = models::KegStatus::Stored;
  }

  target.status = models::KegStatus::ActiveOnScale;
  target.lastWeightGrams = filteredWeightGrams;
  target.lastVolumeMl = result.volumeMl;
  const std::int64_t observedAtUtc = currentUtcSeconds();
  if (observedAtUtc != models::kUnknownTimestamp) {
    target.lastSeenAtUtc = observedAtUtc;
  }
  copyText(catalog_.activeKegId, target.id.data());
  const KegError saved = persist();
  if (saved != KegError::None) {
    target = previousTarget;
    if (oldIndex >= 0 && oldIndex != targetIndex) {
      catalog_.kegs[static_cast<std::size_t>(oldIndex)].status =
          previousOldStatus;
    }
    catalog_.activeKegId = previousActiveId;
    return saved;
  }

  ++revision_;
  if (result.validity != models::MeasurementValidity::Valid) {
    KEEZER_LOG_WARN(
        kLogTag,
        "Measurement bounded: keg=%s raw=%ldg raw_volume=%luml validity=%s",
        target.id.data(), static_cast<long>(filteredWeightGrams),
        static_cast<unsigned long>(result.unclampedVolumeMl),
        models::measurementValidityName(result.validity));
  }
  KEEZER_LOG_INFO(
      kLogTag, "Measurement saved: keg=%s weight=%ldg volume=%luml source=%s",
      target.id.data(), static_cast<long>(filteredWeightGrams),
      static_cast<unsigned long>(result.volumeMl),
      activationSourceName(source));
  return KegError::None;
}

const models::Keg* KegService::at(const std::size_t index) const {
  return index < catalog_.count ? &catalog_.kegs[index] : nullptr;
}

std::size_t KegService::count() const { return catalog_.count; }

std::uint32_t KegService::revision() const { return revision_; }

bool KegService::storageReady() const { return storageReady_; }

bool KegService::catalogAvailable() const { return catalogAvailable_; }

bool KegService::validDraft(const models::KegDraft& draft) {
  return validId(draft.id) &&
         draft.name != nullptr && draft.name[0] != '\0' &&
         std::strlen(draft.name) < models::kKegTextBytes &&
         validOptionalText(draft.nfcUid, 4U * models::kNfcUidBytes) &&
         validOptionalText(draft.beerStyle, models::kKegTextBytes) &&
         validOptionalText(draft.batch, models::kKegTextBytes) &&
         validOptionalText(draft.notes, models::kKegNotesBytes) &&
         domain::KegCalculator::validateConfiguration(
             draft.capacityMl, draft.tareGrams,
             draft.densityGramsPerLiter);
}

bool KegService::normalizeUid(
    const char* source, char (&destination)[models::kNfcUidBytes]) {
  destination[0] = '\0';
  if (source == nullptr || source[0] == '\0') {
    return true;
  }
  std::size_t output = 0U;
  for (std::size_t input = 0U; source[input] != '\0'; ++input) {
    const unsigned char character = static_cast<unsigned char>(source[input]);
    if (character == ':' || character == '-' || std::isspace(character)) {
      continue;
    }
    if (!std::isxdigit(character) || output >= models::kNfcUidBytes - 1U) {
      destination[0] = '\0';
      return false;
    }
    destination[output++] =
        static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
  }
  destination[output] = '\0';
  return output > 0U && (output % 2U) == 0U;
}

void KegService::applyDraft(models::Keg& keg,
                            const models::KegDraft& draft,
                            const char* normalizedUid) {
  copyText(keg.id, draft.id);
  copyText(keg.nfcUid, normalizedUid);
  copyText(keg.name, draft.name);
  copyText(keg.beerStyle, draft.beerStyle);
  copyText(keg.batch, draft.batch);
  copyText(keg.notes, draft.notes);
  keg.capacityMl = draft.capacityMl;
  keg.tareGrams = draft.tareGrams;
  keg.densityGramsPerLiter = draft.densityGramsPerLiter;
  keg.lastWeightGrams = draft.lastWeightGrams;
  keg.lastVolumeMl = std::min(draft.lastVolumeMl, draft.capacityMl);
  keg.filledAtUtc = draft.filledAtUtc;
  keg.schemaVersion = 1U;
}

int KegService::indexById(const char* id) const {
  if (id == nullptr || id[0] == '\0') {
    return -1;
  }
  for (std::size_t index = 0U; index < catalog_.count; ++index) {
    if (sameText(catalog_.kegs[index].id.data(), id)) {
      return static_cast<int>(index);
    }
  }
  return -1;
}

int KegService::indexByUid(const char* normalizedUid) const {
  if (normalizedUid == nullptr || normalizedUid[0] == '\0') {
    return -1;
  }
  for (std::size_t index = 0U; index < catalog_.count; ++index) {
    if (sameText(catalog_.kegs[index].nfcUid.data(), normalizedUid)) {
      return static_cast<int>(index);
    }
  }
  return -1;
}

KegError KegService::persist() {
  if (!storageReady_) {
    return KegError::StorageError;
  }
  return repository_.save(catalog_) == storage::RepositoryResult::Ok
             ? KegError::None
             : KegError::StorageError;
}

void KegService::reconcileActiveKeg() {
  int selected = indexById(catalog_.activeKegId.data());
  if (selected >= 0 && catalog_.kegs[static_cast<std::size_t>(selected)].status !=
                           models::KegStatus::ActiveOnScale) {
    selected = -1;
  }
  for (std::size_t index = 0U; index < catalog_.count; ++index) {
    models::Keg& keg = catalog_.kegs[index];
    if (keg.status != models::KegStatus::ActiveOnScale) {
      continue;
    }
    if (selected < 0) {
      selected = static_cast<int>(index);
      copyText(catalog_.activeKegId, keg.id.data());
    } else if (static_cast<int>(index) != selected) {
      keg.status = models::KegStatus::Stored;
    }
  }
  if (selected < 0) {
    catalog_.activeKegId = {};
  }
}

const char* kegErrorName(const KegError error) {
  switch (error) {
    case KegError::None:
      return "NONE";
    case KegError::InvalidData:
      return "INVALID_DATA";
    case KegError::CatalogFull:
      return "CATALOG_FULL";
    case KegError::NotFound:
      return "NOT_FOUND";
    case KegError::DuplicateId:
      return "DUPLICATE_ID";
    case KegError::DuplicateNfc:
      return "DUPLICATE_NFC";
    case KegError::Unauthorized:
      return "UNAUTHORIZED";
    case KegError::ActivationRequiresConfirmation:
      return "ACTIVATION_REQUIRES_CONFIRMATION";
    case KegError::StorageError:
      return "STORAGE_ERROR";
  }
  return "UNKNOWN";
}

}  // namespace keezer::services
