#include <cassert>
#include <cstdio>
#include <cstring>

#include "services/NetworkService.h"

namespace {

class FakeNetworkAdapter final : public keezer::network::INetworkAdapter {
 public:
  bool connect(const keezer::models::NetworkSettings&) override {
    ++connectCalls;
    return allowConnect;
  }
  void disconnect() override {
    ++disconnectCalls;
    isConnected = false;
  }
  bool connected() const override { return isConnected; }
  bool localIp(char* destination, std::size_t size) const override {
    std::snprintf(destination, size, "%s", "10.0.0.42");
    return isConnected;
  }
  bool connectedSsid(char* destination, std::size_t size) const override {
    std::snprintf(destination, size, "%s", "LAB");
    return isConnected;
  }
  std::int16_t rssiDbm() const override { return -57; }

  int connectCalls{0};
  int disconnectCalls{0};
  bool allowConnect{true};
  bool isConnected{false};
};

keezer::models::NetworkSettings settings() {
  keezer::models::NetworkSettings result{};
  std::snprintf(result.ssid.data(), result.ssid.size(), "%s", "LAB");
  std::snprintf(result.password.data(), result.password.size(), "%s",
                "password");
  std::snprintf(result.hostname.data(), result.hostname.size(), "%s",
                "keezer-test");
  std::snprintf(result.scaleHost.data(), result.scaleHost.size(), "%s",
                "10.0.0.188");
  result.scalePort = 80U;
  return result;
}

}  // namespace

int main() {
  using keezer::models::NetworkStatus;
  using keezer::services::NetworkError;
  using keezer::services::NetworkService;

  FakeNetworkAdapter adapter;
  NetworkService service(adapter);
  assert(service.begin(settings(), 0U));
  assert(adapter.connectCalls == 1);
  assert(service.state().status == NetworkStatus::Connecting);

  adapter.isConnected = true;
  service.update(100U);
  assert(service.connected());
  assert(service.state().status == NetworkStatus::Connected);
  assert(std::strcmp(service.state().ipAddress.data(), "10.0.0.42") == 0);
  assert(service.state().rssiDbm == -57);

  adapter.isConnected = false;
  service.update(1'000U);
  assert(adapter.connectCalls == 2);
  assert(service.state().status == NetworkStatus::Connecting);
  service.update(21'000U);
  assert(service.state().status == NetworkStatus::Reconnecting);
  assert(service.state().retryInMs == 5'000U);
  service.update(26'000U);
  assert(adapter.connectCalls == 3);

  auto changed = settings();
  std::snprintf(changed.scaleHost.data(), changed.scaleHost.size(), "%s",
                "balanca.local");
  const std::uint32_t before = service.configurationRevision();
  assert(service.applySettings(changed, 30'000U) == NetworkError::None);
  assert(service.configurationRevision() == before + 1U);
  assert(std::strcmp(service.scaleHost(), "balanca.local") == 0);

  changed.scalePort = 0U;
  assert(service.applySettings(changed, 31'000U) ==
         NetworkError::InvalidSettings);
  std::puts("NetworkService smoke test: PASS");
  return 0;
}
