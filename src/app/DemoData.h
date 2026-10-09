#pragma once

#include <array>

#include "app/ViewData.h"

namespace keezer::app::demo {

inline constexpr std::size_t kDemoKegCount = 3U;

inline constexpr FreezerViewData kFreezer{
    2.3F,
    2.0F,
    "COOLING",
    true,
};

inline constexpr std::array<KegViewData, kMaxKegs> kKegs{{
    {1U,
     "GERMAN PILSNER",
     14.2F,
     20.0F,
     71U,
     18.55F,
     4.35F,
     1.000F,
     "04A23F891C",
     models::KegStatus::ActiveOnScale,
     true,
     1'788'442'920LL,
     0.83F,
     {142, 131, 120, 108, 93, 79, 64},
     "KEG_001",
     "GERMAN PILSNER",
     "",
     "",
     "03/09/2026"},
    {2U,
     "WEST COAST IPA",
     6.3F,
     20.0F,
     32U,
     10.65F,
     4.35F,
     1.000F,
     "04B17D2210",
     models::KegStatus::Stored,
     false,
     1'788'442'860LL,
     0.61F,
     {105, 98, 91, 84, 77, 70, 63},
     "KEG_002",
     "WEST COAST IPA",
     "",
     "",
     "03/09/2026"},
    {3U,
     "VIENNA LAGER",
     0.4F,
     20.0F,
     2U,
     4.75F,
     4.35F,
     1.000F,
     "04C09E118A",
     models::KegStatus::Finished,
     false,
     1'788'442'800LL,
     0.42F,
     {43, 35, 28, 20, 14, 8, 4},
     "KEG_003",
     "VIENNA LAGER",
     "",
     "",
     "03/09/2026"},
}};

}  // namespace keezer::app::demo
