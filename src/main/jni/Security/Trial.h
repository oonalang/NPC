#pragma once
#include <cstdint>

namespace license::trial {
inline constexpr unsigned freeDays = 3;

std::uint64_t Expiry() noexcept;
bool Active() noexcept;
void Crash() noexcept;
}
