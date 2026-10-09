#include "models/Scale.h"

namespace keezer::models {

const char* scaleLinkStatusName(const ScaleLinkStatus status) {
  switch (status) {
    case ScaleLinkStatus::Online:
      return "ONLINE";
    case ScaleLinkStatus::Stale:
      return "STALE";
    case ScaleLinkStatus::Offline:
      return "OFFLINE";
  }
  return "OFFLINE";
}

}  // namespace keezer::models
