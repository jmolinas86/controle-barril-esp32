#include "models/KegStatus.h"

namespace keezer::models {

const char* kegStatusName(const KegStatus status) {
  switch (status) {
    case KegStatus::Available:
      return "DISPONIVEL";
    case KegStatus::ActiveOnScale:
      return "ATIVO";
    case KegStatus::Stored:
      return "ARMAZENADO";
    case KegStatus::Empty:
      return "VAZIO";
    case KegStatus::Finished:
      return "FINALIZADO";
    case KegStatus::Cleaning:
      return "LIMPEZA";
    case KegStatus::Archived:
      return "ARQUIVADO";
  }
  return "DESCONHECIDO";
}

bool canActivateAutomatically(const KegStatus status) {
  return status == KegStatus::Available || status == KegStatus::Stored ||
         status == KegStatus::Empty || status == KegStatus::ActiveOnScale;
}

}  // namespace keezer::models
