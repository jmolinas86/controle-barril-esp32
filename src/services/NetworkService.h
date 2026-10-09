#pragma once

#include <cstdint>

#include "models/Network.h"
#include "network/INetworkAdapter.h"

namespace keezer::services {

struct NetworkServiceSettings {
  std::uint32_t connectionTimeoutMs{20'000U};
  std::uint32_t initialRetryMs{5'000U};
  std::uint32_t maximumRetryMs{60'000U};
  std::uint32_t telemetryRefreshMs{2'000U};
};

enum class NetworkError : std::uint8_t {
  None = 0U,
  InvalidSettings,
  AdapterError,
  StorageError,
};

class NetworkService final {
 public:
  explicit NetworkService(network::INetworkAdapter& adapter);

  bool begin(const models::NetworkSettings& settings, std::uint32_t nowMs,
             const NetworkServiceSettings& serviceSettings = {});
  void update(std::uint32_t nowMs);
  NetworkError applySettings(const models::NetworkSettings& settings,
                             std::uint32_t nowMs);

  const models::NetworkState& state() const;
  const models::NetworkSettings& settings() const;
  bool connected() const;
  const char* scaleHost() const;
  std::uint16_t scalePort() const;
  const char* hostname() const;
  std::uint32_t configurationRevision() const;
  static bool validSettings(const models::NetworkSettings& settings);

 private:
  bool startAttempt(std::uint32_t nowMs);
  void updateConnectedTelemetry(std::uint32_t nowMs);
  void setStatus(models::NetworkStatus status);
  void scheduleRetry(std::uint32_t nowMs);

  network::INetworkAdapter& adapter_;
  models::NetworkSettings settings_{};
  models::NetworkState state_{};
  NetworkServiceSettings serviceSettings_{};
  std::uint32_t attemptStartedAtMs_{0U};
  std::uint32_t nextRetryAtMs_{0U};
  std::uint32_t currentRetryMs_{0U};
  std::uint32_t nextTelemetryAtMs_{0U};
  std::uint32_t configurationRevision_{0U};
  bool attemptActive_{false};
  bool started_{false};
};

const char* networkErrorName(NetworkError error);

}  // namespace keezer::services
