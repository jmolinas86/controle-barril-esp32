#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "models/KegStatus.h"

namespace keezer::models {

inline constexpr std::size_t kMaximumKegs = 32U;
inline constexpr std::size_t kKegIdBytes = 17U;
inline constexpr std::size_t kNfcUidBytes = 21U;
inline constexpr std::size_t kKegTextBytes = 33U;
inline constexpr std::size_t kKegNotesBytes = 161U;
inline constexpr std::int64_t kUnknownTimestamp = -1;

struct Keg final {
  std::array<char, kKegIdBytes> id{};
  std::array<char, kNfcUidBytes> nfcUid{};
  std::array<char, kKegTextBytes> name{};
  std::array<char, kKegTextBytes> beerStyle{};
  std::array<char, kKegTextBytes> batch{};
  std::array<char, kKegNotesBytes> notes{};
  std::uint32_t capacityMl{20'000U};
  std::int32_t tareGrams{0};
  std::uint16_t densityGramsPerLiter{1'000U};
  std::int32_t lastWeightGrams{0};
  std::uint32_t lastVolumeMl{0U};
  KegStatus status{KegStatus::Available};
  std::int64_t createdAtUtc{kUnknownTimestamp};
  std::int64_t filledAtUtc{kUnknownTimestamp};
  std::int64_t activatedAtUtc{kUnknownTimestamp};
  std::int64_t lastSeenAtUtc{kUnknownTimestamp};
  std::uint16_t schemaVersion{1U};
};

struct KegCatalog final {
  std::array<Keg, kMaximumKegs> kegs{};
  std::size_t count{0U};
  std::array<char, kKegIdBytes> activeKegId{};
};

struct KegDraft final {
  const char* id{nullptr};
  const char* nfcUid{nullptr};
  const char* name{nullptr};
  const char* beerStyle{nullptr};
  const char* batch{nullptr};
  const char* notes{nullptr};
  std::uint32_t capacityMl{20'000U};
  std::int32_t tareGrams{0};
  std::uint16_t densityGramsPerLiter{1'000U};
  std::int32_t lastWeightGrams{0};
  std::uint32_t lastVolumeMl{0U};
  std::int64_t filledAtUtc{kUnknownTimestamp};
};

}  // namespace keezer::models
