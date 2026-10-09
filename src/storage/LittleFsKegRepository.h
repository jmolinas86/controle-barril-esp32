#pragma once

#include <cstdint>

#include "storage/IKegRepository.h"

namespace keezer::storage {

class LittleFsKegRepository final : public IKegRepository {
 public:
  bool begin() override;
  RepositoryResult load(models::KegCatalog& catalog) override;
  RepositoryResult save(const models::KegCatalog& catalog) override;

  std::uint32_t generation() const;

 private:
  bool ready_{false};
  std::uint32_t generation_{0U};
};

}  // namespace keezer::storage
