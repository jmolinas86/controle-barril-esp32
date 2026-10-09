#pragma once

#include <WebServer.h>

#include <array>
#include <cstddef>
#include <cstdint>

namespace keezer::ota {

class OtaService final {
 public:
  using MaintenanceHandler = void (*)(void* context);

  bool begin(const char* username, const char* password,
             MaintenanceHandler maintenanceHandler = nullptr,
             void* maintenanceContext = nullptr);
  void update(bool networkConnected, std::uint32_t nowMs);

  bool uploadInProgress() const;
  bool restartPending() const;

 private:
  static constexpr std::size_t kUsernameBytes = 33U;
  static constexpr std::size_t kPasswordBytes = 65U;

  bool authenticate(bool requestChallenge);
  void handlePage();
  void handleUpload();
  void handleUploadFinished();
  void failUpload(const char* reason);
  void scheduleRestart(std::uint32_t nowMs);

  WebServer server_{80U};
  std::array<char, kUsernameBytes> username_{};
  std::array<char, kPasswordBytes> password_{};
  std::array<char, 96U> result_{};
  MaintenanceHandler maintenanceHandler_{nullptr};
  void* maintenanceContext_{nullptr};
  std::size_t expectedFirmwareSize_{0U};
  std::uint32_t lastUploadActivityAtMs_{0U};
  std::uint32_t restartAtMs_{0U};
  bool started_{false};
  bool networkWasConnected_{false};
  bool uploadAuthenticated_{false};
  bool uploadInProgress_{false};
  bool uploadSucceeded_{false};
  bool restartPending_{false};
};

}  // namespace keezer::ota
