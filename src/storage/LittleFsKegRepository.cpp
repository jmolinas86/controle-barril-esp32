#include "storage/LittleFsKegRepository.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <Preferences.h>

#include <algorithm>
#include <array>
#include <cstring>

#include "diagnostics/Logger.h"

namespace keezer::storage {
namespace {

constexpr char kLogTag[] = "KEG_STORE";
constexpr char kSlotA[] = "/kegs_a.bin";
constexpr char kSlotB[] = "/kegs_b.bin";
constexpr std::uint32_t kMagic = 0x4B454753U;  // KEGS
constexpr std::uint16_t kFormatVersion = 1U;
constexpr char kPreferencesNamespace[] = "keg_store";
constexpr char kInitializedKey[] = "fs_init";
constexpr char kMountPath[] = "/littlefs";
constexpr char kPartitionLabel[] = "littlefs";
constexpr std::uint8_t kMaximumOpenFiles = 10U;

#pragma pack(push, 1)
struct SnapshotHeader final {
  std::uint32_t magic;
  std::uint16_t formatVersion;
  std::uint16_t recordSize;
  std::uint32_t generation;
  std::uint16_t count;
  std::uint16_t reserved;
  std::uint32_t payloadCrc;
};

struct PersistedKeg final {
  char id[models::kKegIdBytes];
  char nfcUid[models::kNfcUidBytes];
  char name[models::kKegTextBytes];
  char beerStyle[models::kKegTextBytes];
  char batch[models::kKegTextBytes];
  char notes[models::kKegNotesBytes];
  std::uint32_t capacityMl;
  std::int32_t tareGrams;
  std::uint16_t densityGramsPerLiter;
  std::int32_t lastWeightGrams;
  std::uint32_t lastVolumeMl;
  std::uint8_t status;
  std::int64_t createdAtUtc;
  std::int64_t filledAtUtc;
  std::int64_t activatedAtUtc;
  std::int64_t lastSeenAtUtc;
  std::uint16_t schemaVersion;
};

struct SnapshotPrefix final {
  char activeKegId[models::kKegIdBytes];
};
#pragma pack(pop)

struct SlotInfo final {
  bool exists{false};
  bool valid{false};
  SnapshotHeader header{};
};

std::uint32_t crc32Update(std::uint32_t crc, const std::uint8_t* data,
                          const std::size_t length) {
  for (std::size_t index = 0U; index < length; ++index) {
    crc ^= data[index];
    for (std::uint8_t bit = 0U; bit < 8U; ++bit) {
      crc = (crc >> 1U) ^ (0xEDB88320U & (0U - (crc & 1U)));
    }
  }
  return crc;
}

template <typename T>
std::uint32_t crcObject(std::uint32_t crc, const T& value) {
  return crc32Update(crc, reinterpret_cast<const std::uint8_t*>(&value),
                     sizeof(T));
}

template <std::size_t Size>
void copyText(char (&destination)[Size], const std::array<char, Size>& source) {
  std::memcpy(destination, source.data(), Size);
  destination[Size - 1U] = '\0';
}

template <std::size_t Size>
void copyText(std::array<char, Size>& destination, const char (&source)[Size]) {
  std::memcpy(destination.data(), source, Size);
  destination.back() = '\0';
}

PersistedKeg encode(const models::Keg& keg) {
  PersistedKeg record{};
  copyText(record.id, keg.id);
  copyText(record.nfcUid, keg.nfcUid);
  copyText(record.name, keg.name);
  copyText(record.beerStyle, keg.beerStyle);
  copyText(record.batch, keg.batch);
  copyText(record.notes, keg.notes);
  record.capacityMl = keg.capacityMl;
  record.tareGrams = keg.tareGrams;
  record.densityGramsPerLiter = keg.densityGramsPerLiter;
  record.lastWeightGrams = keg.lastWeightGrams;
  record.lastVolumeMl = keg.lastVolumeMl;
  record.status = static_cast<std::uint8_t>(keg.status);
  record.createdAtUtc = keg.createdAtUtc;
  record.filledAtUtc = keg.filledAtUtc;
  record.activatedAtUtc = keg.activatedAtUtc;
  record.lastSeenAtUtc = keg.lastSeenAtUtc;
  record.schemaVersion = keg.schemaVersion;
  return record;
}

bool decode(const PersistedKeg& record, models::Keg& keg) {
  if (record.status > static_cast<std::uint8_t>(models::KegStatus::Archived) ||
      record.schemaVersion == 0U) {
    return false;
  }
  keg = {};
  copyText(keg.id, record.id);
  copyText(keg.nfcUid, record.nfcUid);
  copyText(keg.name, record.name);
  copyText(keg.beerStyle, record.beerStyle);
  copyText(keg.batch, record.batch);
  copyText(keg.notes, record.notes);
  keg.capacityMl = record.capacityMl;
  keg.tareGrams = record.tareGrams;
  keg.densityGramsPerLiter = record.densityGramsPerLiter;
  keg.lastWeightGrams = record.lastWeightGrams;
  keg.lastVolumeMl = record.lastVolumeMl;
  keg.status = static_cast<models::KegStatus>(record.status);
  keg.createdAtUtc = record.createdAtUtc;
  keg.filledAtUtc = record.filledAtUtc;
  keg.activatedAtUtc = record.activatedAtUtc;
  keg.lastSeenAtUtc = record.lastSeenAtUtc;
  keg.schemaVersion = record.schemaVersion;
  return keg.id[0] != '\0';
}

SlotInfo inspectSlot(const char* path) {
  SlotInfo info{};
  File file = LittleFS.open(path, FILE_READ);
  if (!file) {
    return info;
  }
  info.exists = true;
  if (file.read(reinterpret_cast<std::uint8_t*>(&info.header),
                sizeof(info.header)) != sizeof(info.header) ||
      info.header.magic != kMagic ||
      info.header.formatVersion != kFormatVersion ||
      info.header.recordSize != sizeof(PersistedKeg) ||
      info.header.count > models::kMaximumKegs) {
    file.close();
    return info;
  }

  const std::size_t expectedSize = sizeof(SnapshotHeader) +
      sizeof(SnapshotPrefix) +
      (static_cast<std::size_t>(info.header.count) * sizeof(PersistedKeg));
  if (file.size() != expectedSize) {
    file.close();
    return info;
  }

  std::uint32_t crc = 0xFFFFFFFFU;
  std::array<std::uint8_t, 128U> buffer{};
  while (file.available()) {
    const std::size_t read = file.read(buffer.data(), buffer.size());
    if (read == 0U) {
      file.close();
      return info;
    }
    crc = crc32Update(crc, buffer.data(), read);
  }
  file.close();
  info.valid = (crc ^ 0xFFFFFFFFU) == info.header.payloadCrc;
  return info;
}

bool newer(const std::uint32_t lhs, const std::uint32_t rhs) {
  return static_cast<std::int32_t>(lhs - rhs) > 0;
}

RepositoryResult readSlot(const char* path, models::KegCatalog& catalog) {
  File file = LittleFS.open(path, FILE_READ);
  if (!file) {
    return RepositoryResult::IoError;
  }
  SnapshotHeader header{};
  SnapshotPrefix prefix{};
  if (file.read(reinterpret_cast<std::uint8_t*>(&header), sizeof(header)) !=
          sizeof(header) ||
      file.read(reinterpret_cast<std::uint8_t*>(&prefix), sizeof(prefix)) !=
          sizeof(prefix)) {
    file.close();
    return RepositoryResult::Corrupt;
  }
  catalog = {};
  copyText(catalog.activeKegId, prefix.activeKegId);
  for (std::size_t index = 0U; index < header.count; ++index) {
    PersistedKeg record{};
    if (file.read(reinterpret_cast<std::uint8_t*>(&record), sizeof(record)) !=
            sizeof(record) ||
        !decode(record, catalog.kegs[index])) {
      catalog = {};
      file.close();
      return RepositoryResult::Corrupt;
    }
    ++catalog.count;
  }
  file.close();
  return RepositoryResult::Ok;
}

}  // namespace

bool LittleFsKegRepository::begin() {
  ready_ = LittleFS.begin(false, kMountPath, kMaximumOpenFiles,
                          kPartitionLabel);
  Preferences preferences;
  if (!ready_) {
    preferences.begin(kPreferencesNamespace, false);
    const bool wasInitialized = preferences.getBool(kInitializedKey, false);
    preferences.end();
    if (!wasInitialized) {
      KEEZER_LOG_WARN(kLogTag, "First mount; formatting LittleFS once");
      ready_ = LittleFS.begin(true, kMountPath, kMaximumOpenFiles,
                              kPartitionLabel);
    }
  }
  if (!ready_) {
    KEEZER_LOG_ERROR(kLogTag,
                     "LittleFS mount failed; refusing to format existing data");
  } else {
    preferences.begin(kPreferencesNamespace, false);
    preferences.putBool(kInitializedKey, true);
    preferences.end();
    KEEZER_LOG_INFO(kLogTag, "LittleFS mounted: total=%lu used=%lu",
                    static_cast<unsigned long>(LittleFS.totalBytes()),
                    static_cast<unsigned long>(LittleFS.usedBytes()));
  }
  return ready_;
}

RepositoryResult LittleFsKegRepository::load(models::KegCatalog& catalog) {
  if (!ready_) {
    return RepositoryResult::NotReady;
  }
  const SlotInfo slotA = inspectSlot(kSlotA);
  const SlotInfo slotB = inspectSlot(kSlotB);
  if (!slotA.valid && !slotB.valid) {
    if (slotA.exists || slotB.exists) {
      KEEZER_LOG_ERROR(
          kLogTag,
          "CATALOG_CORRUPT slot_a=%s slot_b=%s action=PRESERVE_AND_STOP",
          slotA.exists ? "INVALID" : "MISSING",
          slotB.exists ? "INVALID" : "MISSING");
    }
    return (slotA.exists || slotB.exists) ? RepositoryResult::Corrupt
                                         : RepositoryResult::NotFound;
  }
  const bool useA = slotA.valid &&
                    (!slotB.valid || newer(slotA.header.generation,
                                           slotB.header.generation));
  const SlotInfo& selected = useA ? slotA : slotB;
  if ((slotA.exists && !slotA.valid) || (slotB.exists && !slotB.valid)) {
    KEEZER_LOG_WARN(
        kLogTag,
        "CATALOG_RECOVERED selected=%s generation=%lu damaged=%s",
        useA ? "A" : "B",
        static_cast<unsigned long>(selected.header.generation),
        slotA.exists && !slotA.valid ? "A" : "B");
  }
  const RepositoryResult result = readSlot(useA ? kSlotA : kSlotB, catalog);
  if (result == RepositoryResult::Ok) {
    generation_ = selected.header.generation;
    KEEZER_LOG_INFO(kLogTag, "Loaded generation=%lu, kegs=%u",
                    static_cast<unsigned long>(generation_),
                    static_cast<unsigned int>(catalog.count));
  }
  return result;
}

RepositoryResult LittleFsKegRepository::save(
    const models::KegCatalog& catalog) {
  if (!ready_) {
    return RepositoryResult::NotReady;
  }
  if (catalog.count > models::kMaximumKegs) {
    return RepositoryResult::IoError;
  }

  const SlotInfo slotA = inspectSlot(kSlotA);
  const SlotInfo slotB = inspectSlot(kSlotB);
  std::uint32_t latestGeneration = generation_;
  if (slotA.valid && newer(slotA.header.generation, latestGeneration)) {
    latestGeneration = slotA.header.generation;
  }
  if (slotB.valid && newer(slotB.header.generation, latestGeneration)) {
    latestGeneration = slotB.header.generation;
  }
  const char* target = nullptr;
  if (!slotA.valid) {
    target = kSlotA;
  } else if (!slotB.valid) {
    target = kSlotB;
  } else {
    target = newer(slotA.header.generation, slotB.header.generation) ? kSlotB
                                                                     : kSlotA;
  }

  SnapshotPrefix prefix{};
  copyText(prefix.activeKegId, catalog.activeKegId);
  std::uint32_t crc = crcObject(0xFFFFFFFFU, prefix);
  for (std::size_t index = 0U; index < catalog.count; ++index) {
    const PersistedKeg record = encode(catalog.kegs[index]);
    crc = crcObject(crc, record);
  }

  SnapshotHeader header{kMagic,
                        kFormatVersion,
                        static_cast<std::uint16_t>(sizeof(PersistedKeg)),
                        latestGeneration + 1U,
                        static_cast<std::uint16_t>(catalog.count),
                        0U,
                        crc ^ 0xFFFFFFFFU};
  LittleFS.remove(target);
  File file = LittleFS.open(target, FILE_WRITE);
  if (!file || file.write(reinterpret_cast<const std::uint8_t*>(&header),
                          sizeof(header)) != sizeof(header) ||
      file.write(reinterpret_cast<const std::uint8_t*>(&prefix),
                 sizeof(prefix)) != sizeof(prefix)) {
    if (file) {
      file.close();
    }
    return RepositoryResult::IoError;
  }
  for (std::size_t index = 0U; index < catalog.count; ++index) {
    const PersistedKeg record = encode(catalog.kegs[index]);
    if (file.write(reinterpret_cast<const std::uint8_t*>(&record),
                   sizeof(record)) != sizeof(record)) {
      file.close();
      return RepositoryResult::IoError;
    }
  }
  file.flush();
  file.close();

  const SlotInfo verified = inspectSlot(target);
  if (!verified.valid || verified.header.generation != header.generation) {
    KEEZER_LOG_ERROR(kLogTag, "Snapshot verification failed");
    return RepositoryResult::Corrupt;
  }
  generation_ = header.generation;
  KEEZER_LOG_INFO(kLogTag, "Saved generation=%lu to %s",
                  static_cast<unsigned long>(generation_), target);
  return RepositoryResult::Ok;
}

std::uint32_t LittleFsKegRepository::generation() const {
  return generation_;
}

}  // namespace keezer::storage
