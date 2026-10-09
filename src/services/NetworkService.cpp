#include "services/NetworkService.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>

namespace keezer::services {
namespace {

template <std::size_t Size>
bool terminated(const std::array<char, Size>& value) {
  return std::find(value.begin(), value.end(), '\0') != value.end();
}

bool validHostname(const char* const text) {
  if (text == nullptr || text[0] == '\0') return false;
  const std::size_t length = std::strlen(text);
  if (length >= models::kNetworkHostnameBytes || text[0] == '-' ||
      text[length - 1U] == '-') {
    return false;
  }
  for (std::size_t index = 0U; index < length; ++index) {
    const unsigned char value = static_cast<unsigned char>(text[index]);
    if (!std::isalnum(value) && value != '-') return false;
  }
  return true;
}

bool validAddress(const char* const text) {
  if (text == nullptr || text[0] == '\0' ||
      std::strlen(text) >= models::kNetworkAddressBytes) {
    return false;
  }
  for (const char* cursor = text; *cursor != '\0'; ++cursor) {
    const unsigned char value = static_cast<unsigned char>(*cursor);
    if (!std::isalnum(value) && value != '.' && value != '-' && value != '_') {
      return false;
    }
  }
  return true;
}

}  // namespace

NetworkService::NetworkService(network::INetworkAdapter& adapter)
    : adapter_(adapter) {}

bool NetworkService::begin(const models::NetworkSettings& settings,
                           const std::uint32_t nowMs,
                           const NetworkServiceSettings& serviceSettings) {
  if (!validSettings(settings) || serviceSettings.connectionTimeoutMs == 0U ||
      serviceSettings.initialRetryMs == 0U ||
      serviceSettings.maximumRetryMs < serviceSettings.initialRetryMs ||
      serviceSettings.telemetryRefreshMs == 0U) {
    settings_ = settings;
    state_ = {};
    state_.status = models::NetworkStatus::ConfigurationError;
    state_.revision = 1U;
    started_ = true;
    return false;
  }

  settings_ = settings;
  serviceSettings_ = serviceSettings;
  state_ = {};
  state_.configured = true;
  state_.ssid = settings_.ssid;
  state_.hostname = settings_.hostname;
  state_.revision = 1U;
  currentRetryMs_ = serviceSettings_.initialRetryMs;
  nextRetryAtMs_ = nowMs;
  nextTelemetryAtMs_ = nowMs;
  configurationRevision_ = 1U;
  attemptActive_ = false;
  started_ = true;
  return startAttempt(nowMs);
}

void NetworkService::update(const std::uint32_t nowMs) {
  if (!started_ || state_.status == models::NetworkStatus::ConfigurationError) {
    return;
  }

  if (adapter_.connected()) {
    if (!state_.connected) {
      state_.connected = true;
      attemptActive_ = false;
      currentRetryMs_ = serviceSettings_.initialRetryMs;
      state_.retryInMs = 0U;
      setStatus(models::NetworkStatus::Connected);
      nextTelemetryAtMs_ = nowMs;
    }
    if (static_cast<std::int32_t>(nowMs - nextTelemetryAtMs_) >= 0) {
      updateConnectedTelemetry(nowMs);
    }
    return;
  }

  if (state_.connected) {
    state_.connected = false;
    state_.hasRssi = false;
    state_.ipAddress = {};
    attemptActive_ = false;
    nextRetryAtMs_ = nowMs;
    setStatus(models::NetworkStatus::Reconnecting);
  }

  if (attemptActive_) {
    if (nowMs - attemptStartedAtMs_ < serviceSettings_.connectionTimeoutMs) {
      state_.retryInMs = 0U;
      return;
    }
    adapter_.disconnect();
    attemptActive_ = false;
    scheduleRetry(nowMs);
  }

  if (static_cast<std::int32_t>(nowMs - nextRetryAtMs_) >= 0) {
    startAttempt(nowMs);
  } else {
    state_.retryInMs = nextRetryAtMs_ - nowMs;
  }
}

NetworkError NetworkService::applySettings(
    const models::NetworkSettings& settings, const std::uint32_t nowMs) {
  if (!validSettings(settings)) return NetworkError::InvalidSettings;
  const std::uint32_t nextStateRevision = state_.revision + 1U;
  adapter_.disconnect();
  settings_ = settings;
  state_ = {};
  state_.configured = true;
  state_.ssid = settings_.ssid;
  state_.hostname = settings_.hostname;
  state_.revision = nextStateRevision;
  ++configurationRevision_;
  currentRetryMs_ = serviceSettings_.initialRetryMs;
  nextRetryAtMs_ = nowMs;
  nextTelemetryAtMs_ = nowMs;
  attemptActive_ = false;
  return startAttempt(nowMs) ? NetworkError::None
                             : NetworkError::AdapterError;
}

const models::NetworkState& NetworkService::state() const { return state_; }

const models::NetworkSettings& NetworkService::settings() const {
  return settings_;
}

bool NetworkService::connected() const { return state_.connected; }

const char* NetworkService::scaleHost() const {
  return settings_.scaleHost.data();
}

std::uint16_t NetworkService::scalePort() const {
  return settings_.scalePort;
}

const char* NetworkService::hostname() const {
  return settings_.hostname.data();
}

std::uint32_t NetworkService::configurationRevision() const {
  return configurationRevision_;
}

bool NetworkService::validSettings(const models::NetworkSettings& settings) {
  if (!terminated(settings.ssid) || !terminated(settings.password) ||
      !terminated(settings.hostname) || !terminated(settings.scaleHost)) {
    return false;
  }
  const std::size_t ssidLength = std::strlen(settings.ssid.data());
  const std::size_t passwordLength = std::strlen(settings.password.data());
  return ssidLength < models::kWifiSsidBytes &&
         passwordLength < models::kWifiPasswordBytes &&
         (passwordLength == 0U || passwordLength >= 8U) &&
         validHostname(settings.hostname.data()) &&
         validAddress(settings.scaleHost.data()) && settings.scalePort != 0U;
}

bool NetworkService::startAttempt(const std::uint32_t nowMs) {
  attemptStartedAtMs_ = nowMs;
  state_.retryInMs = 0U;
  setStatus(models::NetworkStatus::Connecting);
  attemptActive_ = adapter_.connect(settings_);
  if (!attemptActive_) {
    scheduleRetry(nowMs);
    return false;
  }
  return true;
}

void NetworkService::updateConnectedTelemetry(const std::uint32_t nowMs) {
  std::array<char, models::kIpAddressBytes> ip{};
  std::array<char, models::kWifiSsidBytes> ssid{};
  const bool hasIp = adapter_.localIp(ip.data(), ip.size());
  const bool hasSsid = adapter_.connectedSsid(ssid.data(), ssid.size());
  const std::int16_t rssi = adapter_.rssiDbm();
  const bool changed = state_.ipAddress != ip ||
                       (hasSsid && state_.ssid != ssid) ||
                       !state_.hasRssi || state_.rssiDbm != rssi;
  if (hasIp) state_.ipAddress = ip;
  if (hasSsid) state_.ssid = ssid;
  state_.hasRssi = true;
  state_.rssiDbm = rssi;
  if (changed) ++state_.revision;
  nextTelemetryAtMs_ = nowMs + serviceSettings_.telemetryRefreshMs;
}

void NetworkService::setStatus(const models::NetworkStatus status) {
  if (state_.status == status) return;
  state_.status = status;
  ++state_.revision;
}

void NetworkService::scheduleRetry(const std::uint32_t nowMs) {
  setStatus(models::NetworkStatus::Reconnecting);
  nextRetryAtMs_ = nowMs + currentRetryMs_;
  state_.retryInMs = currentRetryMs_;
  currentRetryMs_ = std::min(serviceSettings_.maximumRetryMs,
                             currentRetryMs_ * 2U);
}

const char* networkErrorName(const NetworkError error) {
  switch (error) {
    case NetworkError::None:
      return "NONE";
    case NetworkError::InvalidSettings:
      return "INVALID_SETTINGS";
    case NetworkError::AdapterError:
      return "ADAPTER_ERROR";
    case NetworkError::StorageError:
      return "STORAGE_ERROR";
  }
  return "INVALID_SETTINGS";
}

}  // namespace keezer::services
