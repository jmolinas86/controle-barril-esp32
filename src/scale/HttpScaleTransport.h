#pragma once

#include <WiFi.h>

#include <cstddef>
#include <cstdint>

#include "scale/IScaleTransport.h"
#include "services/NetworkService.h"

namespace keezer::scale {

class HttpScaleTransport final : public IScaleTransport {
 public:
  explicit HttpScaleTransport(services::NetworkService& networkService);
  bool begin(std::uint32_t nowMs) override;
  void update(std::uint32_t nowMs) override;
  bool isConnected() const override;
  bool tryRead(models::ScalePacket& packet) override;

 private:
  enum class RequestState : std::uint8_t { Idle = 0U, Reading };
  enum class ResponseResult : std::uint8_t {
    Invalid = 0U,
    NoReading,
    Reading,
  };
  enum class PollMode : std::uint8_t { Fast = 0U, Stable, Idle };

  void resetNetworkBinding();
  bool resolveScaleAddress(std::uint32_t nowMs);
  void startRequest(std::uint32_t nowMs);
  void consumeResponse(std::uint32_t nowMs);
  void finishRequest(bool serverResponded);
  ResponseResult parseBufferedResponse(std::uint32_t nowMs);
  void updatePollMode(const models::ScalePacket& packet,
                      std::uint32_t nowMs);
  std::uint32_t pollPeriodMs(std::uint32_t nowMs) const;
  void setPollMode(PollMode mode);
  void recordFailure(std::uint32_t nowMs, const char* reason);

  services::NetworkService& networkService_;
  WiFiClient client_;
  models::ScalePacket pendingPacket_{};
  char response_[768]{};
  std::size_t responseSize_{0U};
  RequestState requestState_{RequestState::Idle};
  std::uint32_t lastRequestAtMs_{0U};
  std::uint32_t requestStartedAtMs_{0U};
  std::uint32_t lastResolveAttemptAtMs_{0U};
  std::uint32_t lastFailureLogAtMs_{0U};
  std::uint32_t networkConfigurationRevision_{0U};
  std::uint32_t fastModeUntilMs_{0U};
  std::int32_t lastObservedWeightGrams_{0};
  IPAddress scaleAddress_{};
  std::uint8_t consecutiveFailures_{0U};
  bool addressResolved_{false};
  bool mdnsStarted_{false};
  bool pending_{false};
  bool linkUp_{false};
  bool networkWasConnected_{false};
  bool hasObservedWeight_{false};
  PollMode pollMode_{PollMode::Fast};
};

}  // namespace keezer::scale
