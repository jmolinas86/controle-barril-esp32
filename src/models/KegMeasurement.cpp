#include "models/KegMeasurement.h"

namespace keezer::models {

const char* measurementValidityName(const MeasurementValidity validity) {
  switch (validity) {
    case MeasurementValidity::Valid:
      return "VALID";
    case MeasurementValidity::BelowTare:
      return "BELOW_TARE";
    case MeasurementValidity::AboveExpectedMaximum:
      return "ABOVE_EXPECTED_MAXIMUM";
    case MeasurementValidity::InvalidWeight:
      return "INVALID_WEIGHT";
    case MeasurementValidity::InvalidKegConfiguration:
      return "INVALID_KEG_CONFIGURATION";
  }
  return "INVALID_WEIGHT";
}

}  // namespace keezer::models
