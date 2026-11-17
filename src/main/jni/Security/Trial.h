#pragma once
#include <cstdint>

namespace license::trial {
inline constexpr unsigned freeDays = 3;
inline constexpr std::uint64_t secondsPerDay = 86400;
inline constexpr std::uint64_t windowSeconds = freeDays * secondsPerDay;

struct Window {
    std::uint64_t start = 0;
    std::uint64_t expiry = 0;
};

constexpr Window MakeWindow(std::uint64_t buildEpoch, std::uint64_t anchorEpoch) noexcept {
    const std::uint64_t start = anchorEpoch > buildEpoch ? anchorEpoch : buildEpoch;
    return {start, start + windowSeconds};
}

constexpr bool Within(const Window& window, std::uint64_t now) noexcept {
    return now >= window.start && now < window.expiry;
}

constexpr std::uint64_t EffectiveNow(std::uint64_t wall, std::uint64_t boot,
                                     std::uint64_t sessionWall, std::uint64_t sessionBoot,
                                     std::uint64_t highWater) noexcept {
    const std::uint64_t elapsed = boot > sessionBoot ? boot - sessionBoot : 0;
    const std::uint64_t monotonic = sessionWall + elapsed;
    const std::uint64_t current = wall > monotonic ? wall : monotonic;
    return current > highWater ? current : highWater;
}

std::uint64_t Expiry() noexcept;
bool Active() noexcept;
void Crash() noexcept;
}
