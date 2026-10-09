#pragma once

#include "drivers/ScaleHardware.h"
#include "network/NetworkManager.h"
#include "storage/SettingsStore.h"
#include "web/WebPortal.h"

namespace balanca::app {

class ScaleApplication final {
 public:
  ScaleApplication();

  bool begin();
  void update();

 private:
  storage::DeviceSettings settings_{};
  storage::SettingsStore settingsStore_;
  drivers::ScaleHardware hardware_;
  network::NetworkManager network_;
  web::WebPortal webPortal_;
};

}  // namespace balanca::app
