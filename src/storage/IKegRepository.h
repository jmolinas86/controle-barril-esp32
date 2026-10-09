#pragma once

#include "models/Keg.h"

namespace keezer::storage {

enum class RepositoryResult {
  Ok,
  NotFound,
  NotReady,
  Corrupt,
  IoError,
};

class IKegRepository {
 public:
  virtual ~IKegRepository() = default;
  virtual bool begin() = 0;
  virtual RepositoryResult load(models::KegCatalog& catalog) = 0;
  virtual RepositoryResult save(const models::KegCatalog& catalog) = 0;
};

}  // namespace keezer::storage
