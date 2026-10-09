#include "scale/HttpScaleTransport.h"

#include <Arduino.h>
#include <ESPmDNS.h>

#include <cstdlib>
#include <cstring>

#include "BuildConfig.h"
#include "diagnostics/Logger.h"
#include "scale/ScaleHttpJson.h"

namespace keezer::scale {
namespace {

constexpr char kLogTag[] = "SCALE_HTTP";
constexpr std::uint8_t kFailuresBeforeResolveAgain = 3U;
constexpr std::uint32_t kFailureLogPeriodMs = 10'000U;

std::int64_t absoluteDifference(const std::int32_t lhs,
                                const std::int32_t rhs) {
  const std::int64_t difference =
      static_cast<std::int64_t>(lhs) - static_cast<std::int64_t>(rhs);
  return difference < 0 ? -difference : difference;
}

bool endsWith(const char* const text, const char* const suffix) {
  if (text == nullptr || suffix == nullptr) return false;
  const std::size_t textLength = std::strlen(text);
  const std::size_t suffixLength = std::strlen(suffix);
  return textLength >= suffixLength &&
         std::strcmp(text + textLength - suffixLength, suffix) == 0;
}

int httpStatusCode(const char* const response) {
  if (response == nullptr || std::strncmp(response, "HTTP/1.", 7U) != 0) {
    return 0;
  }
  const char* const separator = std::strchr(response, ' ');
  if (separator == nullptr || separator[1] < '0' || separator[1] > '9') {
    return 0;
  }
  return std::atoi(separator + 1);
}

}  // namespace

HttpScaleTransport::HttpScaleTransport(
    services::NetworkService& networkService)
    : networkService_(networkService) {}

bool HttpScaleTransport::begin(const std::uint32_t nowMs) {
  pendingPacket_ = {};
  responseSize_ = 0U;
  requestState_ = RequestState::Idle;
  lastRequestAtMs_ = nowMs - config::kScaleHttpFastPollPeriodMs;
  requestStartedAtMs_ = 0U;
  lastResolveAttemptAtMs_ = nowMs - config::kScaleHttpResolveRetryPeriodMs;
  lastFailureLogAtMs_ = nowMs - kFailureLogPeriodMs;
  networkConfigurationRevision_ = networkService_.configurationRevision();
  scaleAddress_ = static_cast<std::uint32_t>(0U);
  consecutiveFailures_ = 0U;
  addressResolved_ = false;
  mdnsStarted_ = false;
  pending_ = false;
  linkUp_ = false;
  networkWasConnected_ = networkService_.connected();
  fastModeUntilMs_ = nowMs + config::kScaleHttpFastModeHoldMs;
  lastObservedWeightGrams_ = 0;
  hasObservedWeight_ = false;
  pollMode_ = PollMode::Fast;
  return true;
}

void HttpScaleTransport::update(const std::uint32_t nowMs) {
  if (networkConfigurationRevision_ !=
      networkService_.configurationRevision()) {
    networkConfigurationRevision_ = networkService_.configurationRevision();
    resetNetworkBinding();
  }
  const bool networkConnected = networkService_.connected();
  if (!networkConnected) {
    if (networkWasConnected_ || requestState_ == RequestState::Reading) {
      resetNetworkBinding();
    }
    networkWasConnected_ = false;
    return;
  }
  networkWasConnected_ = true;
  if (requestState_ == RequestState::Reading) {
    consumeResponse(nowMs);
    return;
  }
  if (!pending_ && nowMs - lastRequestAtMs_ >= pollPeriodMs(nowMs)) {
    startRequest(nowMs);
  }
}

void HttpScaleTransport::resetNetworkBinding() {
  linkUp_ = false;
  client_.stop();
  requestState_ = RequestState::Idle;
  responseSize_ = 0U;
  pending_ = false;
  addressResolved_ = false;
  scaleAddress_ = static_cast<std::uint32_t>(0U);
  consecutiveFailures_ = 0U;
  if (mdnsStarted_) MDNS.end();
  mdnsStarted_ = false;
  lastResolveAttemptAtMs_ = 0U;
}

bool HttpScaleTransport::resolveScaleAddress(const std::uint32_t nowMs) {
  if (addressResolved_) return true;
  if (nowMs - lastResolveAttemptAtMs_ <
      config::kScaleHttpResolveRetryPeriodMs) {
    return false;
  }
  lastResolveAttemptAtMs_ = nowMs;

  const char* const scaleHost = networkService_.scaleHost();
  if (scaleAddress_.fromString(scaleHost)) {
    addressResolved_ = true;
  } else if (endsWith(scaleHost, ".local")) {
    if (!mdnsStarted_) {
      mdnsStarted_ = MDNS.begin(networkService_.hostname());
      if (!mdnsStarted_) {
        recordFailure(nowMs, "MDNS_START_FAILED");
        return false;
      }
    }
    char host[64]{};
    const std::size_t length =
        std::strlen(scaleHost) - std::strlen(".local");
    if (length == 0U || length >= sizeof(host)) {
      recordFailure(nowMs, "INVALID_MDNS_HOST");
      return false;
    }
    std::memcpy(host, scaleHost, length);
    scaleAddress_ = MDNS.queryHost(host, config::kScaleMdnsQueryTimeoutMs);
    addressResolved_ = static_cast<std::uint32_t>(scaleAddress_) != 0U;
  } else {
    addressResolved_ =
        WiFi.hostByName(scaleHost, scaleAddress_) == 1;
  }

  if (!addressResolved_) {
    recordFailure(nowMs, "HOST_NOT_FOUND");
    return false;
  }
  consecutiveFailures_ = 0U;
  KEEZER_LOG_INFO(kLogTag, "Scale resolved: host=%s ip=%s port=%u",
                  scaleHost, scaleAddress_.toString().c_str(),
                  static_cast<unsigned int>(networkService_.scalePort()));
  return true;
}

void HttpScaleTransport::startRequest(const std::uint32_t nowMs) {
  lastRequestAtMs_ = nowMs;
  if (!resolveScaleAddress(nowMs)) return;
  client_.stop();
  client_.setTimeout(100U);
  if (!client_.connect(scaleAddress_, networkService_.scalePort(), 150)) {
    linkUp_ = false;
    recordFailure(nowMs, "CONNECT_FAILED");
    finishRequest(false);
    return;
  }
  client_.print(F("GET /api/v1/reading HTTP/1.1\r\nHost: "));
  client_.print(networkService_.scaleHost());
  client_.print(F("\r\nAccept: application/json\r\nConnection: close\r\n\r\n"));
  responseSize_ = 0U;
  response_[0] = '\0';
  requestStartedAtMs_ = nowMs;
  requestState_ = RequestState::Reading;
}

void HttpScaleTransport::consumeResponse(const std::uint32_t nowMs) {
  while (client_.available() > 0) {
    if (responseSize_ + 1U >= sizeof(response_)) {
      KEEZER_LOG_WARN(kLogTag, "HTTP response too large");
      finishRequest(false);
      return;
    }
    response_[responseSize_++] = static_cast<char>(client_.read());
    response_[responseSize_] = '\0';
  }
  if (std::strstr(response_, "\r\n\r\n") != nullptr &&
      std::strchr(std::strstr(response_, "\r\n\r\n") + 4, '}') != nullptr) {
    const ResponseResult result = parseBufferedResponse(nowMs);
    finishRequest(result != ResponseResult::Invalid);
    return;
  }
  if (nowMs - requestStartedAtMs_ >= config::kScaleHttpResponseTimeoutMs ||
      (!client_.connected() && client_.available() == 0)) {
    recordFailure(nowMs, "RESPONSE_TIMEOUT");
    finishRequest(false);
  }
}

HttpScaleTransport::ResponseResult
HttpScaleTransport::parseBufferedResponse(const std::uint32_t nowMs) {
  const int status = httpStatusCode(response_);
  if (status == 503) {
    return ResponseResult::NoReading;
  }
  if (status != 200) {
    KEEZER_LOG_WARN(kLogTag, "Unexpected HTTP status=%d", status);
    return ResponseResult::Invalid;
  }
  const char* const body = std::strstr(response_, "\r\n\r\n");
  models::ScalePacket packet{};
  if (body == nullptr || !ScaleHttpJson::parseReading(body + 4, packet)) {
    KEEZER_LOG_WARN(kLogTag, "Invalid scale JSON response");
    return ResponseResult::Invalid;
  }
  updatePollMode(packet, nowMs);
  pendingPacket_ = packet;
  pending_ = true;
  return ResponseResult::Reading;
}

void HttpScaleTransport::updatePollMode(const models::ScalePacket& packet,
                                        const std::uint32_t nowMs) {
  const bool changed =
      !hasObservedWeight_ ||
      absoluteDifference(packet.weightGrams, lastObservedWeightGrams_) >=
          config::kScaleHttpActivityThresholdGrams;
  if (changed || !packet.stable) {
    fastModeUntilMs_ = nowMs + config::kScaleHttpFastModeHoldMs;
  }
  // Keep an anchor until the accumulated movement reaches the threshold.
  // This catches slow consumption such as 10 g + 10 g instead of resetting
  // the comparison after every response.
  if (changed) {
    lastObservedWeightGrams_ = packet.weightGrams;
    hasObservedWeight_ = true;
  }

  if (static_cast<std::int32_t>(fastModeUntilMs_ - nowMs) > 0 ||
      !packet.stable) {
    setPollMode(PollMode::Fast);
  } else if (packet.weightGrams <=
             config::kScaleHttpEmptyThresholdGrams) {
    setPollMode(PollMode::Idle);
  } else {
    setPollMode(PollMode::Stable);
  }
}

std::uint32_t HttpScaleTransport::pollPeriodMs(
    const std::uint32_t nowMs) const {
  if (static_cast<std::int32_t>(fastModeUntilMs_ - nowMs) > 0) {
    return config::kScaleHttpFastPollPeriodMs;
  }
  switch (pollMode_) {
    case PollMode::Fast:
      return config::kScaleHttpFastPollPeriodMs;
    case PollMode::Stable:
      return config::kScaleHttpStablePollPeriodMs;
    case PollMode::Idle:
      return config::kScaleHttpIdlePollPeriodMs;
  }
  return config::kScaleHttpFastPollPeriodMs;
}

void HttpScaleTransport::setPollMode(const PollMode mode) {
  if (pollMode_ == mode) return;
  pollMode_ = mode;
  const char* name = "FAST";
  std::uint32_t period = config::kScaleHttpFastPollPeriodMs;
  if (mode == PollMode::Stable) {
    name = "STABLE";
    period = config::kScaleHttpStablePollPeriodMs;
  } else if (mode == PollMode::Idle) {
    name = "IDLE";
    period = config::kScaleHttpIdlePollPeriodMs;
  }
  KEEZER_LOG_INFO(kLogTag, "POLL_MODE mode=%s period=%lu ms", name,
                  static_cast<unsigned long>(period));
}

void HttpScaleTransport::finishRequest(const bool serverResponded) {
  client_.stop();
  requestState_ = RequestState::Idle;
  responseSize_ = 0U;
  if (serverResponded) {
    consecutiveFailures_ = 0U;
  } else if (consecutiveFailures_ < 255U) {
    ++consecutiveFailures_;
  }
  linkUp_ = serverResponded;
  if (consecutiveFailures_ >= kFailuresBeforeResolveAgain) {
    addressResolved_ = false;
    consecutiveFailures_ = 0U;
  }
}

void HttpScaleTransport::recordFailure(const std::uint32_t nowMs,
                                       const char* const reason) {
  if (nowMs - lastFailureLogAtMs_ < kFailureLogPeriodMs) return;
  lastFailureLogAtMs_ = nowMs;
  KEEZER_LOG_WARN(kLogTag, "%s host=%s", reason,
                  networkService_.scaleHost());
}

bool HttpScaleTransport::isConnected() const { return linkUp_; }

bool HttpScaleTransport::tryRead(models::ScalePacket& packet) {
  if (!pending_) return false;
  packet = pendingPacket_;
  pending_ = false;
  return true;
}

}  // namespace keezer::scale
