#pragma once

#include "storage/IKegRepository.h"

namespace keezer::storage {

class MemoryKegRepository final : public IKegRepository {
 public:
  bool begin() override;
  RepositoryResult load(models::KegCatalog& catalog) override;
  RepositoryResult save(const models::KegCatalog& catalog) override;

 private:
  models::KegCatalog catalog_{};
  bool ready_{false};
  bool hasSnapshot_{false};
};

}  // namespace keezer::storage
