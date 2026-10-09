#include "network/NetworkManager.h"

#include <Arduino.h>
#include <ESP8266mDNS.h>

#include <cstdio>
#include <cstring>

#include "config.h"

namespace balanca::network {
namespace {

const IPAddress kPortalIp(192, 168, 4, 1);
const IPAddress kPortalMask(255, 255, 255, 0);

void copyText(char* destination, const std::size_t destinationSize,
              const char* source) {
  std::snprintf(destination, destinationSize, "%s", source == nullptr ? "" : source);
}

}  // namespace

void NetworkManager::begin(const char* const ssid, const char* const password) {
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  WiFi.hostname(DEVICE_HOSTNAME);
  copyText(ssid_, sizeof(ssid_), ssid);
  copyText(password_, sizeof(password_), password);
  if (ssid_[0] == '\0') {
    startPortal();
  } else {
    WiFi.mode(WIFI_STA);
    connectStartedAtMs_ = millis();
    connect(connectStartedAtMs_);
  }
}

void NetworkManager::update(const std::uint32_t nowMs) {
  const wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    if (previousStatus_ != WL_CONNECTED) {
      Serial.printf("[WIFI] Connected - %s (%d dBm)\n",
                    WiFi.localIP().toString().c_str(), WiFi.RSSI());
      stopPortal();
      startMdns();
    }
    if (mdnsStarted_) MDNS.update();
  } else {
    if (previousStatus_ == WL_CONNECTED) {
      Serial.println(F("[WIFI] Disconnected"));
      mdnsStarted_ = false;
      connectStartedAtMs_ = nowMs;
    }
    if (ssid_[0] != '\0' &&
        nowMs - lastAttemptAtMs_ >= WIFI_RETRY_INTERVAL_MS) {
      connect(nowMs);
    }
    if (!provisioning_ && ssid_[0] != '\0' &&
        nowMs - connectStartedAtMs_ >= WIFI_CONNECT_TIMEOUT_MS) {
      startPortal();
    }
  }
  previousStatus_ = status;
  if (provisioning_) dns_.processNextRequest();
}

void NetworkManager::applyCredentials(const char* const ssid,
                                      const char* const password,
                                      const std::uint32_t nowMs) {
  copyText(ssid_, sizeof(ssid_), ssid);
  copyText(password_, sizeof(password_), password);
  connectStartedAtMs_ = nowMs;
  connect(nowMs);
}

void NetworkManager::connect(const std::uint32_t nowMs) {
  if (ssid_[0] == '\0') return;
  if (!provisioning_) WiFi.mode(WIFI_STA);
  WiFi.begin(ssid_, password_);
  lastAttemptAtMs_ = nowMs;
  Serial.printf("[WIFI] Connecting to %s\n", ssid_);
}

void NetworkManager::startPortal() {
  if (provisioning_) return;
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(kPortalIp, kPortalIp, kPortalMask);
  WiFi.softAP("BC Balanca");
  dns_.start(53U, "*", WiFi.softAPIP());
  provisioning_ = true;
  Serial.printf("[WIFI] Portal: BC Balanca / http://%s/\n",
                WiFi.softAPIP().toString().c_str());
}

void NetworkManager::stopPortal() {
  if (!provisioning_) return;
  dns_.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  provisioning_ = false;
}

void NetworkManager::startMdns() {
  mdnsStarted_ = MDNS.begin(DEVICE_HOSTNAME);
  if (mdnsStarted_) {
    MDNS.addService("http", "tcp", 80U);
    Serial.printf("[MDNS] http://%s.local/\n", DEVICE_HOSTNAME);
  }
}

bool NetworkManager::connected() const { return WiFi.status() == WL_CONNECTED; }
bool NetworkManager::provisioning() const { return provisioning_; }
std::int32_t NetworkManager::rssiDbm() const {
  return connected() ? WiFi.RSSI() : 0;
}
IPAddress NetworkManager::ip() const {
  return connected() ? WiFi.localIP() : WiFi.softAPIP();
}
const char* NetworkManager::ssid() const { return ssid_; }

}  // namespace balanca::network
