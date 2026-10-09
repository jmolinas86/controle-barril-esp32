#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace keezer::models {

inline constexpr std::size_t kWifiSsidBytes = 33U;
inline constexpr std::size_t kWifiPasswordBytes = 65U;
inline constexpr std::size_t kNetworkHostnameBytes = 33U;
inline constexpr std::size_t kNetworkAddressBytes = 65U;
inline constexpr std::size_t kIpAddressBytes = 16U;

enum class NetworkStatus : std::uint8_t {
  Disabled = 0U,
  Connecting,
  Connected,
  Reconnecting,
  ConfigurationError,
};

struct NetworkSettings {
  std::array<char, kWifiSsidBytes> ssid{};
  std::array<char, kWifiPasswordBytes> password{};
  std::array<char, kNetworkHostnameBytes> hostname{};
  std::array<char, kNetworkAddressBytes> scaleHost{};
  std::uint16_t scalePort{80U};
};

struct NetworkState {
  NetworkStatus status{NetworkStatus::Disabled};
  std::array<char, kWifiSsidBytes> ssid{};
  std::array<char, kNetworkHostnameBytes> hostname{};
  std::array<char, kIpAddressBytes> ipAddress{};
  std::int16_t rssiDbm{0};
  std::uint32_t retryInMs{0U};
  std::uint32_t revision{0U};
  bool configured{false};
  bool connected{false};
  bool hasRssi{false};
};

const char* networkStatusName(NetworkStatus status);

}  // namespace keezer::models
