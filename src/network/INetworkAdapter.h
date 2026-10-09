#pragma once

#include <cstddef>

#include "models/Network.h"

namespace keezer::network {

class INetworkAdapter {
 public:
  virtual ~INetworkAdapter() = default;
  virtual bool connect(const models::NetworkSettings& settings) = 0;
  virtual void disconnect() = 0;
  virtual bool connected() const = 0;
  virtual bool localIp(char* destination, std::size_t size) const = 0;
  virtual bool connectedSsid(char* destination, std::size_t size) const = 0;
  virtual std::int16_t rssiDbm() const = 0;
};

}  // namespace keezer::network
