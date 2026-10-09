#include "storage/MemoryKegRepository.h"

namespace keezer::storage {

bool MemoryKegRepository::begin() {
  ready_ = true;
  return true;
}

RepositoryResult MemoryKegRepository::load(models::KegCatalog& catalog) {
  if (!ready_) {
    return RepositoryResult::NotReady;
  }
  if (!hasSnapshot_) {
    return RepositoryResult::NotFound;
  }
  catalog = catalog_;
  return RepositoryResult::Ok;
}

RepositoryResult MemoryKegRepository::save(
    const models::KegCatalog& catalog) {
  if (!ready_) {
    return RepositoryResult::NotReady;
  }
  catalog_ = catalog;
  hasSnapshot_ = true;
  return RepositoryResult::Ok;
}

}  // namespace keezer::storage
