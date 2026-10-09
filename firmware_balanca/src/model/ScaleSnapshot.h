#pragma once

#include <cstddef>
#include <cstdint>

namespace balanca::model {

inline constexpr std::size_t kNfcUidBytes = 21U;

struct ScaleSnapshot {
  std::uint32_t sequence{0U};
  std::int32_t weightGrams{0};
  bool weightValid{false};
  bool stable{false};
  bool hasNfcUid{false};
  char nfcUid[kNfcUidBytes]{};
  std::uint32_t lastWeightAtMs{0U};
};

}  // namespace balanca::model
