#pragma once

#include <cstdint>

namespace keezer::ui {

enum class Route : std::uint8_t {
  Home = 0U,
  KegDetail,
  KegEdit,
  Temperature,
  Settings,
  NetworkSettings,
  ArchivedKegs,
  ArchivedKegDetail,
  DisplaySettings,
};

const char* routeTitle(Route route);

}  // namespace keezer::ui
