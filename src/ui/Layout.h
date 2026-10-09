#pragma once

#include <cstdint>

#include "BuildConfig.h"

namespace keezer::ui::layout {

inline constexpr std::int32_t kScreenWidth = config::kDisplayWidth;
inline constexpr std::int32_t kScreenHeight = config::kDisplayHeight;
inline constexpr std::int32_t kHeaderHeight = 31;
inline constexpr std::int32_t kContentY = kHeaderHeight;
inline constexpr std::int32_t kContentHeight = 250;
inline constexpr std::int32_t kNavigationY = kContentY + kContentHeight;
inline constexpr std::int32_t kNavigationHeight = 39;
inline constexpr std::int32_t kNavigationButtonWidth = kScreenWidth / 3;
inline constexpr std::int32_t kMargin = 5;
inline constexpr std::int32_t kCardWidth = kScreenWidth - (2 * kMargin) - 2;

static_assert(kNavigationY + kNavigationHeight == kScreenHeight,
              "Portrait layout must fill the display height");

}  // namespace keezer::ui::layout
