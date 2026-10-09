#include "ui/TimeFormat.h"

#include <cstdio>
#include <ctime>

namespace keezer::ui {
namespace {

constexpr std::int64_t kMinimumValidUtcSeconds = 1'577'836'800LL;

}  // namespace

void formatLocalDateTime(const std::int64_t utcSeconds,
                         const bool includeYear, char* const destination,
                         const std::size_t size) {
  if (destination == nullptr || size == 0U) return;
  if (utcSeconds < kMinimumValidUtcSeconds) {
    std::snprintf(destination, size,
                  includeYear ? "--/--/---- --:--" : "--/-- --:--");
    return;
  }
  const std::time_t value = static_cast<std::time_t>(utcSeconds);
  std::tm localTime{};
  if (localtime_r(&value, &localTime) == nullptr) {
    std::snprintf(destination, size,
                  includeYear ? "--/--/---- --:--" : "--/-- --:--");
    return;
  }
  if (includeYear) {
    std::snprintf(destination, size, "%02d/%02d/%04d %02d:%02d",
                  localTime.tm_mday, localTime.tm_mon + 1,
                  localTime.tm_year + 1900, localTime.tm_hour,
                  localTime.tm_min);
  } else {
    std::snprintf(destination, size, "%02d/%02d %02d:%02d",
                  localTime.tm_mday, localTime.tm_mon + 1,
                  localTime.tm_hour, localTime.tm_min);
  }
}

}  // namespace keezer::ui
