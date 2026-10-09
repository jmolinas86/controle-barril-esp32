#include "storage/SdHistoryRepository.h"

#include <Arduino.h>
#include <SD.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>

#include "BuildConfig.h"
#include "diagnostics/Logger.h"

namespace keezer::storage {
namespace {

constexpr char kHistoryRoot[] = "/history";
constexpr char kLogTag[] = "HISTORY_SD";

std::uint32_t crc32(const std::uint8_t* const data,
                    const std::size_t size) {
  std::uint32_t crc = 0xFFFFFFFFU;
  for (std::size_t index = 0U; index < size; ++index) {
    crc ^= data[index];
    for (std::uint8_t bit = 0U; bit < 8U; ++bit) {
      crc = (crc >> 1U) ^
            (0xEDB88320U & (0U - static_cast<std::uint32_t>(crc & 1U)));
    }
  }
  return crc ^ 0xFFFFFFFFU;
}

}  // namespace

bool SdHistoryRepository::begin(const std::uint32_t bootId) {
  bootId_ = bootId;
  ready_ = SD.cardType() != CARD_NONE;
  if (!ready_) {
    lastResult_ = HistoryRepositoryResult::NotReady;
    return false;
  }
  const std::uint64_t total = SD.totalBytes();
  const std::uint64_t used = SD.usedBytes();
  if (total <= used || total - used <= config::kSdMinimumFreeBytes) {
    ready_ = false;
    lastResult_ = HistoryRepositoryResult::Full;
    return false;
  }
  if (ready_ && !SD.exists(kHistoryRoot)) {
    ready_ = SD.mkdir(kHistoryRoot);
  }
  lastResult_ = ready_ ? HistoryRepositoryResult::Ok
                       : HistoryRepositoryResult::IoError;
  return ready_;
}

HistoryRepositoryResult SdHistoryRepository::append(
    const models::HistoryRecord& record) {
  const std::uint32_t operationStartedAtUs = micros();
  ++performance_.writeAttempts;
  const auto finish = [this, operationStartedAtUs](
                          const HistoryRepositoryResult result,
                          const std::size_t bytesWritten = 0U) {
    const std::uint32_t durationUs = micros() - operationStartedAtUs;
    performance_.bytesWritten += bytesWritten;
    performance_.operationTotalUs += durationUs;
    if (durationUs > performance_.operationMaxUs) {
      performance_.operationMaxUs = durationUs;
    }
    if (result != HistoryRepositoryResult::Ok) {
      ++performance_.failedOperations;
    }
    return result;
  };
  if (!ready_) {
    return finish(lastResult_);
  }
  if (!validKegId(record.kegId.data()) ||
      !ensureKegDirectory(record.kegId.data())) {
    lastResult_ = HistoryRepositoryResult::InvalidData;
    return finish(lastResult_);
  }

  char path[80]{};
  buildFilePath(record.kegId.data(), path, sizeof(path));
  File file = SD.open(path, FILE_APPEND);
  if (!file) {
    ready_ = false;
    lastResult_ = SD.cardType() == CARD_NONE
                      ? HistoryRepositoryResult::NotReady
                      : HistoryRepositoryResult::IoError;
    return finish(lastResult_);
  }

  char payload[176]{};
  const int payloadLength = std::snprintf(
      payload, sizeof(payload),
      "%u,%lu,%08lX,%lld,%lu,%ld,%ld,%lu,%u,%u,%u,%u",
      static_cast<unsigned int>(models::kHistoryFormatVersion),
      static_cast<unsigned long>(record.sequence),
      static_cast<unsigned long>(record.bootId),
      static_cast<long long>(record.utcSeconds),
      static_cast<unsigned long>(record.monotonicMs),
      static_cast<long>(record.rawWeightGrams),
      static_cast<long>(record.filteredWeightGrams),
      static_cast<unsigned long>(record.volumeMl),
      static_cast<unsigned int>(record.percentageBasisPoints),
      static_cast<unsigned int>(record.source),
      static_cast<unsigned int>(record.validity),
      static_cast<unsigned int>(record.flags));
  char line[192]{};
  const std::uint32_t checksum =
      payloadLength > 0
          ? crc32(reinterpret_cast<const std::uint8_t*>(payload),
                  static_cast<std::size_t>(payloadLength))
          : 0U;
  const int length =
      payloadLength > 0 && static_cast<std::size_t>(payloadLength) <
                                 sizeof(payload)
          ? std::snprintf(line, sizeof(line), "%s,%08lX\n", payload,
                          static_cast<unsigned long>(checksum))
          : -1;
  const bool formatted =
      length > 0 && static_cast<std::size_t>(length) < sizeof(line);
  const std::uint64_t total = SD.totalBytes();
  const std::uint64_t used = SD.usedBytes();
  if (!formatted || total <= used ||
      total - used <= static_cast<std::uint64_t>(length) +
                          config::kSdMinimumFreeBytes) {
    file.close();
    ready_ = false;
    lastResult_ = formatted ? HistoryRepositoryResult::Full
                            : HistoryRepositoryResult::InvalidData;
    return finish(lastResult_);
  }
  const std::size_t written =
      formatted ? file.write(reinterpret_cast<const std::uint8_t*>(line),
                             static_cast<std::size_t>(length))
                : 0U;
  file.flush();
  file.close();
  if (!formatted || written != static_cast<std::size_t>(length)) {
    ready_ = false;
    lastResult_ = SD.cardType() == CARD_NONE
                      ? HistoryRepositoryResult::NotReady
                      : HistoryRepositoryResult::IoError;
    return finish(lastResult_);
  }
  lastResult_ = HistoryRepositoryResult::Ok;
  return finish(lastResult_, written);
}

HistoryRepositoryResult SdHistoryRepository::loadRecent(
    const char* const kegId, models::HistoryRecord* const records,
    const std::size_t capacity, std::size_t& count) {
  const std::uint32_t operationStartedAtUs = micros();
  ++performance_.readAttempts;
  std::uint64_t bytesRead = 0U;
  const auto finish = [this, operationStartedAtUs, &bytesRead](
                          const HistoryRepositoryResult result) {
    const std::uint32_t durationUs = micros() - operationStartedAtUs;
    performance_.bytesRead += bytesRead;
    performance_.operationTotalUs += durationUs;
    if (durationUs > performance_.operationMaxUs) {
      performance_.operationMaxUs = durationUs;
    }
    if (result != HistoryRepositoryResult::Ok &&
        result != HistoryRepositoryResult::Corrupt) {
      ++performance_.failedOperations;
    }
    return result;
  };
  count = 0U;
  if (!ready_) {
    return finish(lastResult_);
  }
  if (SD.cardType() == CARD_NONE) {
    ready_ = false;
    lastResult_ = HistoryRepositoryResult::NotReady;
    return finish(lastResult_);
  }
  if (!validKegId(kegId) || records == nullptr || capacity == 0U) {
    return finish(HistoryRepositoryResult::InvalidData);
  }

  char directoryPath[48]{};
  std::snprintf(directoryPath, sizeof(directoryPath), "%s/%s", kHistoryRoot,
                kegId);
  if (!SD.exists(directoryPath)) {
    return finish(HistoryRepositoryResult::Ok);
  }
  File directory = SD.open(directoryPath, FILE_READ);
  if (!directory || !directory.isDirectory()) {
    ready_ = false;
    lastResult_ = HistoryRepositoryResult::IoError;
    return finish(lastResult_);
  }

  std::size_t total = 0U;
  std::size_t corruptLines = 0U;
  File file = directory.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      bytesRead += file.size();
      char line[176]{};
      while (readLine(file, line, sizeof(line))) {
        models::HistoryRecord parsed{};
        if (!parseRecord(line, kegId, parsed)) {
          ++corruptLines;
          continue;
        }
        records[total % capacity] = parsed;
        ++total;
      }
    }
    file.close();
    file = directory.openNextFile();
  }
  directory.close();

  count = total < capacity ? total : capacity;
  if (total > capacity) {
    const std::size_t first = total % capacity;
    std::rotate(records, records + first, records + capacity);
  }
  if (corruptLines != 0U) {
    KEEZER_LOG_WARN(kLogTag,
                    "HISTORY_CORRUPT keg=%s ignored_lines=%u valid=%u",
                    kegId, static_cast<unsigned int>(corruptLines),
                    static_cast<unsigned int>(count));
    return finish(HistoryRepositoryResult::Corrupt);
  }
  return finish(HistoryRepositoryResult::Ok);
}

bool SdHistoryRepository::ready() const { return ready_; }

HistoryRepositoryResult SdHistoryRepository::lastResult() const {
  return lastResult_;
}

SdPerformanceStats SdHistoryRepository::takePerformanceStats() {
  const SdPerformanceStats result = performance_;
  performance_ = {};
  return result;
}

bool SdHistoryRepository::validKegId(const char* const kegId) {
  if (kegId == nullptr || kegId[0] == '\0') {
    return false;
  }
  for (const char* cursor = kegId; *cursor != '\0'; ++cursor) {
    const unsigned char value = static_cast<unsigned char>(*cursor);
    if (!std::isalnum(value) && *cursor != '_' && *cursor != '-') {
      return false;
    }
  }
  return true;
}

bool SdHistoryRepository::readLine(File& file, char* const destination,
                                   const std::size_t size) {
  if (destination == nullptr || size < 2U || !file.available()) {
    return false;
  }
  std::size_t length = 0U;
  while (file.available()) {
    const int value = file.read();
    if (value < 0 || value == '\n') {
      break;
    }
    if (value != '\r' && length + 1U < size) {
      destination[length++] = static_cast<char>(value);
    }
  }
  destination[length] = '\0';
  return length != 0U;
}

bool SdHistoryRepository::parseRecord(const char* const line,
                                      const char* const kegId,
                                      models::HistoryRecord& record) {
  if (line == nullptr) {
    return false;
  }
  char payload[176]{};
  const char* parseText = line;
  const char* const checksumSeparator = std::strrchr(line, ',');
  if (checksumSeparator != nullptr && std::strlen(checksumSeparator + 1U) == 8U) {
    unsigned long storedChecksum = 0U;
    char trailing = '\0';
    if (std::sscanf(checksumSeparator + 1U, "%8lx%c", &storedChecksum,
                    &trailing) != 1) {
      return false;
    }
    const std::size_t payloadLength =
        static_cast<std::size_t>(checksumSeparator - line);
    if (payloadLength == 0U || payloadLength >= sizeof(payload)) {
      return false;
    }
    std::memcpy(payload, line, payloadLength);
    payload[payloadLength] = '\0';
    if (crc32(reinterpret_cast<const std::uint8_t*>(payload), payloadLength) !=
        static_cast<std::uint32_t>(storedChecksum)) {
      return false;
    }
    parseText = payload;
  }
  unsigned int version = 0U;
  unsigned long sequence = 0U;
  unsigned long bootId = 0U;
  long long utc = -1;
  unsigned long monotonic = 0U;
  long raw = 0;
  long filtered = 0;
  unsigned long volume = 0U;
  unsigned int percentage = 0U;
  unsigned int source = 0U;
  unsigned int validity = 0U;
  unsigned int flags = 0U;
  if (std::sscanf(parseText,
                  "%u,%lu,%lx,%lld,%lu,%ld,%ld,%lu,%u,%u,%u,%u",
                  &version, &sequence, &bootId, &utc, &monotonic, &raw,
                  &filtered, &volume, &percentage, &source, &validity,
                  &flags) != 12 ||
      version != models::kHistoryFormatVersion || percentage > 10'000U ||
      raw < 0 || raw > config::kScaleMaximumWeightGrams || filtered < 0 ||
      filtered > config::kScaleMaximumWeightGrams || volume > 100'000UL ||
      source >
          static_cast<unsigned int>(models::HistorySource::AutomaticTracking) ||
      validity > static_cast<unsigned int>(
                     models::MeasurementValidity::InvalidKegConfiguration)) {
    return false;
  }
  std::snprintf(record.kegId.data(), record.kegId.size(), "%s", kegId);
  record.sequence = static_cast<std::uint32_t>(sequence);
  record.bootId = static_cast<std::uint32_t>(bootId);
  record.utcSeconds = static_cast<std::int64_t>(utc);
  record.monotonicMs = static_cast<std::uint32_t>(monotonic);
  record.rawWeightGrams = static_cast<std::int32_t>(raw);
  record.filteredWeightGrams = static_cast<std::int32_t>(filtered);
  record.volumeMl = static_cast<std::uint32_t>(volume);
  record.percentageBasisPoints = static_cast<std::uint16_t>(percentage);
  record.source = static_cast<models::HistorySource>(source);
  record.validity = static_cast<models::MeasurementValidity>(validity);
  record.flags = static_cast<std::uint8_t>(flags);
  return true;
}

bool SdHistoryRepository::ensureKegDirectory(const char* const kegId) const {
  char path[48]{};
  std::snprintf(path, sizeof(path), "%s/%s", kHistoryRoot, kegId);
  return SD.exists(path) || SD.mkdir(path);
}

void SdHistoryRepository::buildFilePath(const char* const kegId,
                                        char* const destination,
                                        const std::size_t size) const {
  std::snprintf(destination, size, "%s/%s/unsynced-%08lX.csv", kHistoryRoot,
                kegId, static_cast<unsigned long>(bootId_));
}

const char* historyRepositoryResultName(
    const HistoryRepositoryResult result) {
  switch (result) {
    case HistoryRepositoryResult::Ok:
      return "OK";
    case HistoryRepositoryResult::NotReady:
      return "SD_MISSING";
    case HistoryRepositoryResult::Full:
      return "SD_FULL";
    case HistoryRepositoryResult::Corrupt:
      return "CORRUPT_DATA";
    case HistoryRepositoryResult::InvalidData:
      return "INVALID_DATA";
    case HistoryRepositoryResult::IoError:
      return "IO_ERROR";
  }
  return "IO_ERROR";
}

}  // namespace keezer::storage
