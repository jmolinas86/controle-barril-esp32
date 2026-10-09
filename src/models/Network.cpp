#include "models/Network.h"

namespace keezer::models {

const char* networkStatusName(const NetworkStatus status) {
  switch (status) {
    case NetworkStatus::Disabled:
      return "DESATIVADA";
    case NetworkStatus::Connecting:
      return "CONECTANDO";
    case NetworkStatus::Connected:
      return "CONECTADA";
    case NetworkStatus::Reconnecting:
      return "RECONECTANDO";
    case NetworkStatus::ConfigurationError:
      return "CONFIG. INVALIDA";
  }
  return "DESATIVADA";
}

}  // namespace keezer::models
