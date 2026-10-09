#include "scale/SimulatedScaleTransport.h"

#include <cstdio>

#include "BuildConfig.h"

namespace keezer::scale {

bool SimulatedScaleTransport::begin(const std::uint32_t nowMs) {
  pendingPacket_ = {};
  startedAtMs_ = nowMs;
  lastEmissionAtMs_ = nowMs;
  sequence_ = 0U;
  currentScenario_ = 3U;
  scenarioSampleIndex_ = 0U;
  pending_ = false;
  emittedAtLeastOnce_ = false;
  connected_ = true;
  return true;
}

void SimulatedScaleTransport::update(const std::uint32_t nowMs) {
  const std::uint32_t elapsedMs = nowMs - startedAtMs_;
  const std::uint32_t cyclePositionMs =
      elapsedMs % config::kScaleSimulatorCycleMs;
  connected_ = cyclePositionMs < config::kScaleSimulatorSendingMs;
  if (!connected_ || pending_) {
    return;
  }
  if (emittedAtLeastOnce_ &&
      nowMs - lastEmissionAtMs_ < config::kScaleSimulatorReportPeriodMs) {
    return;
  }
  preparePacket(nowMs);
}

bool SimulatedScaleTransport::isConnected() const { return connected_; }

bool SimulatedScaleTransport::tryRead(models::ScalePacket& packet) {
  if (!pending_) {
    return false;
  }
  packet = pendingPacket_;
  pending_ = false;
  return true;
}

void SimulatedScaleTransport::preparePacket(const std::uint32_t nowMs) {
  static constexpr std::int32_t kWeightJitter[]{
      0, -8, 8, -5, 12, -10, 1'500, 6, -4, 9,
  };
  const std::uint32_t scenario =
      ((nowMs - startedAtMs_) / config::kScaleSimulatorCycleMs) % 3U;
  if (scenario != currentScenario_) {
    currentScenario_ = scenario;
    scenarioSampleIndex_ = 0U;
  }
  pendingPacket_ = {};
  pendingPacket_.protocolVersion = models::kScaleProtocolVersion;
  std::snprintf(pendingPacket_.scaleId.data(),
                pendingPacket_.scaleId.size(), "%s", models::kMainScaleId);
  pendingPacket_.sequence = ++sequence_;
  pendingPacket_.hasNfcUid = scenario < 2U;
  if (scenario == 0U) {
    std::snprintf(pendingPacket_.nfcUid.data(),
                  pendingPacket_.nfcUid.size(), "%s", "04:A2-3F 891C");
  } else if (scenario == 1U) {
    std::snprintf(pendingPacket_.nfcUid.data(),
                  pendingPacket_.nfcUid.size(), "%s", "04:B1-7D 2210");
  }
  const std::int32_t baseWeight =
      scenario == 0U ? 18'550 : (scenario == 1U ? 10'650 : 450);
  pendingPacket_.weightGrams =
      baseWeight +
      kWeightJitter[scenarioSampleIndex_ %
                    (sizeof(kWeightJitter) / sizeof(kWeightJitter[0]))];
  ++scenarioSampleIndex_;
  pendingPacket_.stable = true;
  pendingPacket_.hasBatteryPercentage = true;
  pendingPacket_.batteryPercentage = 92U;
  pendingPacket_.hasRssiDbm = true;
  pendingPacket_.rssiDbm = -58;
  pendingPacket_.senderUptimeMs = nowMs - startedAtMs_;
  lastEmissionAtMs_ = nowMs;
  emittedAtLeastOnce_ = true;
  pending_ = true;
}

}  // namespace keezer::scale
