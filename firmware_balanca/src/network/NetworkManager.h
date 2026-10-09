#pragma once

#include <DNSServer.h>
#include <ESP8266WiFi.h>

#include <cstdint>

namespace balanca::network {

class NetworkManager final {
 public:
  void begin(const char* ssid, const char* password);
  void update(std::uint32_t nowMs);
  void applyCredentials(const char* ssid, const char* password,
                        std::uint32_t nowMs);

  bool connected() const;
  bool provisioning() const;
  std::int32_t rssiDbm() const;
  IPAddress ip() const;
  const char* ssid() const;

 private:
  void connect(std::uint32_t nowMs);
  void startPortal();
  void stopPortal();
  void startMdns();

  DNSServer dns_;
  char ssid_[33]{};
  char password_[65]{};
  bool provisioning_{false};
  bool mdnsStarted_{false};
  wl_status_t previousStatus_{WL_IDLE_STATUS};
  std::uint32_t connectStartedAtMs_{0U};
  std::uint32_t lastAttemptAtMs_{0U};
};

}  // namespace balanca::network
