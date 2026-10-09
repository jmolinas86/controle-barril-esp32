#include "network/Esp32WiFiAdapter.h"

#include <WiFi.h>

#include <cstdio>
#include <cstring>

namespace keezer::network {

bool Esp32WiFiAdapter::connect(const models::NetworkSettings& settings) {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);
  if (settings.hostname[0] != '\0') {
    WiFi.setHostname(settings.hostname.data());
  }
  if (settings.ssid[0] == '\0') {
    WiFi.begin();
  } else {
    WiFi.begin(settings.ssid.data(), settings.password.data());
  }
  return true;
}

void Esp32WiFiAdapter::disconnect() {
  WiFi.disconnect(false, false);
}

bool Esp32WiFiAdapter::connected() const {
  return WiFi.status() == WL_CONNECTED;
}

bool Esp32WiFiAdapter::localIp(char* const destination,
                               const std::size_t size) const {
  if (destination == nullptr || size == 0U || !connected()) return false;
  const IPAddress ip = WiFi.localIP();
  std::snprintf(destination, size, "%u.%u.%u.%u",
                static_cast<unsigned int>(ip[0]),
                static_cast<unsigned int>(ip[1]),
                static_cast<unsigned int>(ip[2]),
                static_cast<unsigned int>(ip[3]));
  return true;
}

bool Esp32WiFiAdapter::connectedSsid(char* const destination,
                                     const std::size_t size) const {
  if (destination == nullptr || size == 0U || !connected()) return false;
  const String ssid = WiFi.SSID();
  std::snprintf(destination, size, "%s", ssid.c_str());
  return true;
}

std::int16_t Esp32WiFiAdapter::rssiDbm() const {
  return connected() ? static_cast<std::int16_t>(WiFi.RSSI()) : 0;
}

}  // namespace keezer::network
