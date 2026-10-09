#include "ui/Routes.h"

namespace keezer::ui {

const char* routeTitle(const Route route) {
  switch (route) {
    case Route::Home:
      return "HOME";
    case Route::KegDetail:
      return "BARRIL";
    case Route::KegEdit:
      return "EDITAR KEG";
    case Route::Temperature:
      return "TEMPERATURA";
    case Route::Settings:
      return "CONFIG.";
    case Route::NetworkSettings:
      return "REDE";
    case Route::ArchivedKegs:
      return "ARQUIVADOS";
    case Route::ArchivedKegDetail:
      return "HISTORICO";
    case Route::DisplaySettings:
      return "DISPLAY";
  }
  return "KEEZER";
}

}  // namespace keezer::ui
