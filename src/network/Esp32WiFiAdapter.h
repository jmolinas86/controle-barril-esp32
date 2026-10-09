#pragma once

#include "network/INetworkAdapter.h"

namespace keezer::network {

class Esp32WiFiAdapter final : public INetworkAdapter {
 public:
  bool connect(const models::NetworkSettings& settings) override;
  void disconnect() override;
  bool connected() const override;
  bool localIp(char* destination, std::size_t size) const override;
  bool connectedSsid(char* destination, std::size_t size) const override;
  std::int16_t rssiDbm() const override;
};

}  // namespace keezer::network
