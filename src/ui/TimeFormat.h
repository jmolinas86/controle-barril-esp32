#pragma once

#include <cstddef>
#include <cstdint>

namespace keezer::ui {

void formatLocalDateTime(std::int64_t utcSeconds, bool includeYear,
                         char* destination, std::size_t size);

}  // namespace keezer::ui
