#pragma once
#include <array>
#include <REL/Relocation.h>

namespace ENBInputDisabler
{
inline constexpr std::array kRuntimes{
    REL::Version{1, 10, 163, 0},
    REL::Version{1, 11, 221, 0},
    REL::Version{1, 11, 240, 0},
};
[[nodiscard]] constexpr bool IsSupported(const REL::Version& version) noexcept
{
    for (const auto& supported : kRuntimes) {
        if (version == supported) return true;
    }
    return false;
}
}
