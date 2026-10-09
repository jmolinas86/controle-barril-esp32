#include <cassert>
#include <cstdio>
#include <cstring>

#include "diagnostics/Logger.h"
#include "scale/IScaleTransport.h"
#include "services/ScaleService.h"

namespace keezer::diagnostics {
LogLevel Logger::currentLevel_ = LogLevel::Error;
void Logger::begin(std::uint32_t) {}
void Logger::setLevel(const LogLevel level) { currentLevel_ = level; }
LogLevel Logger::level() { return currentLevel_; }
void Logger::log(LogLevel, const char*, const char*, ...) {}
const char* Logger::levelName(LogLevel) { return "TEST"; }
}  // namespace keezer::diagnostics

namespace {

class FakeScaleTransport final : public keezer::scale::IScaleTransport {
 public:
  bool begin(std::uint32_t) override {
    connected_ = true;
    return true;
  }
  void update(std::uint32_t) override {}
  bool isConnected() const override { return connected_; }
  bool tryRead(keezer::models::ScalePacket& packet) override {
    if (!pending_) {
      return false;
    }
    packet = packet_;
    pending_ = false;
    return true;
  }
  void push(const keezer::models::ScalePacket& packet) {
    packet_ = packet;
    pending_ = true;
  }

 private:
  keezer::models::ScalePacket packet_{};
  bool pending_{false};
  bool connected_{false};
};

keezer::models::ScalePacket packet(const std::uint32_t sequence,
                                   const std::uint64_t uptimeMs,
                                   const char* const uid = "04:a2-3f 891c") {
  keezer::models::ScalePacket value{};
  value.sequence = sequence;
  std::snprintf(value.scaleId.data(), value.scaleId.size(), "%s",
                keezer::models::kMainScaleId);
  value.hasNfcUid = uid != nullptr;
  if (uid != nullptr) {
    std::snprintf(value.nfcUid.data(), value.nfcUid.size(), "%s", uid);
  }
  value.weightGrams = 18'550;
  value.stable = true;
  value.hasBatteryPercentage = true;
  value.batteryPercentage = 92U;
  value.hasRssiDbm = true;
  value.rssiDbm = -58;
  value.senderUptimeMs = uptimeMs;
  return value;
}

}  // namespace

int main() {
  using keezer::models::ScaleLinkStatus;

  FakeScaleTransport transport;
  keezer::services::ScaleService service(transport);
  const keezer::services::ScaleSettings settings{30'000U, 300'000U,
                                                  100'000};
  assert(service.begin(0U, settings));
  assert(service.state().linkStatus == ScaleLinkStatus::Offline);

  transport.push(packet(1U, 1'000U));
  service.update(1'000U);
  assert(service.state().linkStatus == ScaleLinkStatus::Online);
  assert(service.state().filteredWeightGrams == 18'550);
  assert(service.state().hasDetectedNfcUid);
  assert(!service.state().hasStableNfcUid);
  assert(std::strcmp(service.state().detectedNfcUid.data(), "04A23F891C") ==
         0);

  transport.push(packet(2U, 2'000U));
  service.update(2'000U);
  transport.push(packet(3U, 3'000U));
  service.update(3'000U);
  assert(service.state().hasStableNfcUid);
  assert(std::strcmp(service.state().stableNfcUid.data(), "04A23F891C") ==
         0);
  assert(service.state().nfcEventRevision == 1U);

  service.update(33'000U);
  assert(service.state().linkStatus == ScaleLinkStatus::Stale);
  service.update(303'000U);
  assert(service.state().linkStatus == ScaleLinkStatus::Offline);

  transport.push(packet(3U, 4'000U));
  service.update(304'000U);
  assert(service.state().duplicateReadingCount == 1U);

  transport.push(packet(1U, 10U));
  service.update(304'001U);
  assert(service.state().linkStatus == ScaleLinkStatus::Online);
  assert(service.state().acceptedReadingCount == 4U);

  keezer::models::ScalePacket invalid = packet(2U, 20U);
  std::snprintf(invalid.scaleId.data(), invalid.scaleId.size(), "%s",
                "SCALE_OTHER");
  transport.push(invalid);
  service.update(304'002U);
  assert(service.state().validationErrorCount == 1U);

  // Negative weight is rejected and cannot replace the last valid value.
  invalid = packet(2U, 20U);
  invalid.weightGrams = -200;
  transport.push(invalid);
  service.update(304'003U);
  assert(service.state().validationErrorCount == 2U);
  assert(service.state().filteredWeightGrams == 18'550);

  transport.push(packet(2U, 20U, nullptr));
  service.update(305'000U);
  transport.push(packet(3U, 1'020U, nullptr));
  service.update(306'000U);
  transport.push(packet(4U, 2'020U, nullptr));
  service.update(307'000U);
  assert(!service.state().hasStableNfcUid);
  assert(service.state().nfcEventRevision == 2U);

  transport.push(packet(5U, 3'020U, "04B17D2210"));
  service.update(308'000U);
  transport.push(packet(6U, 4'020U, "04B17D2210"));
  service.update(309'000U);
  transport.push(packet(7U, 5'020U, "04B17D2210"));
  service.update(310'000U);
  assert(service.state().hasStableNfcUid);
  assert(std::strcmp(service.state().stableNfcUid.data(), "04B17D2210") ==
         0);
  assert(service.state().nfcEventRevision == 3U);

  transport.push(packet(8U, 6'020U, "04C09E118A"));
  service.update(310'200U);
  transport.push(packet(9U, 6'220U, "04D09E118B"));
  service.update(310'400U);
  transport.push(packet(10U, 6'420U, "04C09E118A"));
  service.update(310'600U);
  transport.push(packet(11U, 6'620U, "04D09E118B"));
  service.update(310'800U);
  assert(service.state().nfcAmbiguous);
  assert(std::strcmp(service.state().stableNfcUid.data(), "04B17D2210") ==
         0);

  keezer::models::ScalePacket unstable =
      packet(12U, 6'820U, "04B17D2210");
  unstable.weightGrams = 20'000;
  unstable.stable = false;
  transport.push(unstable);
  service.update(311'000U);
  assert(service.state().rawWeightGrams == 20'000);
  assert(service.state().filteredWeightGrams == 18'550);
  assert(!service.state().stable);

  std::puts("ScaleService smoke test: PASS");
  return 0;
}
