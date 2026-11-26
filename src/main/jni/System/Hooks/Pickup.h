#pragma once

#include "../../Trigger/NativeProfile.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace pickup {

inline constexpr float defaultRange = 1.f;
inline constexpr float maximumRange = 3.f;
inline constexpr float fastInterval = .05f;
inline constexpr std::size_t scanIntervalOffset = 0x68;
inline constexpr std::size_t horizontalRangeOffset = 0x70;
inline constexpr std::size_t verticalRangeOffset = 0x74;

struct Options {
    bool fast = false;
    float range = defaultRange;
    bool Active() const noexcept { return fast || range > defaultRange; }
};

inline float NormalizeRange(float value) noexcept {
    return std::isfinite(value) ? std::clamp(value, defaultRange, maximumRange) : defaultRange;
}

inline std::atomic<std::uint64_t> requested{0x3f800000u};
inline std::atomic<bool> available{false};

inline void Publish(bool fast, float range) noexcept {
    range = NormalizeRange(range);
    std::uint32_t bits = 0;
    std::memcpy(&bits, &range, sizeof(bits));
    const auto packed = bits | (static_cast<std::uint64_t>(fast) << 32);
    if (requested.load(std::memory_order_relaxed) != packed)
        requested.store(packed, std::memory_order_release);
}

inline Options Read() noexcept {
    const auto packed = requested.load(std::memory_order_acquire);
    const auto bits = static_cast<std::uint32_t>(packed);
    float range = defaultRange;
    std::memcpy(&range, &bits, sizeof(range));
    return {(packed >> 32) != 0, NormalizeRange(range)};
}

template<class Value>
inline Value Field(const void* object, std::size_t offset) noexcept {
    Value value{};
    if (object) std::memcpy(&value, static_cast<const unsigned char*>(object) + offset, sizeof(value));
    return value;
}

inline float ScaleLimit(float original, float factor) noexcept {
    if (!std::isfinite(original) || original <= 0.f) return original;
    const float result = original * factor;
    return std::isfinite(result) ? result : original;
}

inline float PickupInterval(float original, bool fast) noexcept {
    return fast && std::isfinite(original) && original > fastInterval ? fastInterval : original;
}

class ScopedFloat {
public:
    ScopedFloat(void* object, std::size_t offset, float value, bool enabled = true) noexcept
        : address_(enabled ? static_cast<unsigned char*>(object) + offset : nullptr) {
        if (!address_) return;
        std::memcpy(&original_, address_, sizeof(original_));
        std::memcpy(&applied_, &value, sizeof(applied_));
        std::memcpy(address_, &applied_, sizeof(applied_));
    }
    ~ScopedFloat() {
        if (address_ && std::memcmp(address_, &applied_, sizeof(applied_)) == 0)
            std::memcpy(address_, &original_, sizeof(original_));
    }
    ScopedFloat(const ScopedFloat&) = delete;
    ScopedFloat& operator=(const ScopedFloat&) = delete;
private:
    unsigned char* address_;
    std::uint32_t original_ = 0;
    std::uint32_t applied_ = 0;
};

inline thread_local const void* activeManager = nullptr;

struct ScopedManager {
    const void* previous = activeManager;
    explicit ScopedManager(const void* manager) noexcept { activeManager = manager; }
    ~ScopedManager() { activeManager = previous; }
    ScopedManager(const ScopedManager&) = delete;
    ScopedManager& operator=(const ScopedManager&) = delete;
};

template<class Original>
inline void Invoke(void* manager, Options options, Original original) {
    if (!manager || !options.Active() || activeManager) {
        original();
        return;
    }
    ScopedManager owner(manager);
    const float horizontal = Field<float>(manager, horizontalRangeOffset);
    const float vertical = Field<float>(manager, verticalRangeOffset);
    const float interval = Field<float>(manager, scanIntervalOffset);
    const float range = NormalizeRange(options.range);
    ScopedFloat horizontalOverride(manager, horizontalRangeOffset,
        ScaleLimit(horizontal, range * range), range > defaultRange);
    ScopedFloat verticalOverride(manager, verticalRangeOffset,
        ScaleLimit(vertical, range), range > defaultRange);
    ScopedFloat intervalOverride(manager, scanIntervalOffset,
        PickupInterval(interval, options.fast), options.fast);
    original();
}

using PhantomTrigger::NativeProfileDetail::CodeCheck;
inline constexpr std::array<CodeCheck, 10> profile{{
    {"Pickup update differs", 0x64E8970, {0xff,0x83,0x00,0xd1,0xfe,0x4f,0x01,0xa9,0xf3,0x03,0x00,0xaa,0xc0,0x93,0x9e,0x52}},
    {"Pickup list differs", 0x64ECD2C, {0xff,0x83,0x05,0xd1,0xef,0x3b,0x0c,0x6d,0xed,0x33,0x0d,0x6d,0xeb,0x2b,0x0e,0x6d}},
    {"Pickup interval differs", 0x90F6DE8, {0xfe,0x0f,0x1e,0xf8,0xf4,0x4f,0x01,0xa9,0x14,0x1e,0x03,0xb0,0xf3,0x03,0x00,0xaa}},
    {"Pickup horizontal layout differs", 0x64EC020, {0x00,0x08,0x20,0x1e,0xa0,0x02,0x40,0xf9,0x60,0x72,0x00,0xbd,0x08,0xb0,0x4c,0x78}},
    {"Pickup vertical layout differs", 0x64EC048, {0x67,0xa5,0x6c,0x94,0x80,0x00,0x00,0x36,0x08,0xf8,0xa7,0x52,0x68,0x76,0x00,0xb9}},
    {"Pickup scan layout differs", 0x64E89C8, {0x61,0x6e,0x40,0xbd,0x28,0x20,0x20,0x1e,0xa0,0x00,0x00,0x54,0x62,0x6a,0x40,0xbd}},
    {"Pickup owner layout differs", 0x64EB308, {0x74,0x0a,0x40,0xf9,0x34,0x50,0x00,0xb4,0x7b,0x29,0x04,0xf0,0x7c,0x29,0x04,0xf0}},
    {"Pickup player mode differs", 0x8016060, {0xfe,0x4f,0xbf,0xa9,0xd3,0xa4,0x03,0xb0,0x68,0xd6,0x50,0x39,0xe8,0x00,0x00,0x37}},
    {"Pickup match differs", 0x801191C, {0xfe,0x4f,0xbf,0xa9,0xf3,0xa4,0x03,0xd0,0x68,0x5e,0x4f,0x39,0xe8,0x00,0x00,0x37}},
    {"Pickup local player differs", 0x8012814, {0xfe,0x4f,0xbf,0xa9,0xf3,0xa4,0x03,0xb0,0x68,0xca,0x50,0x39,0xe8,0x00,0x00,0x37}}
}};

}

#if defined(__ANDROID__) && defined(__aarch64__) && !defined(STUDIO_STANDALONE_PREVIEW)
#include "../../Security/Access.h"
#include <mutex>

namespace pickup {
using Metadata = const void*;
using Getter = void*(*)(void*, Metadata);
inline void (*originalNearby)(void*, Metadata) = nullptr;
inline void (*originalRefresh)(void*, bool*, bool*, Metadata) = nullptr;
inline float (*originalInterval)(void*, Metadata) = nullptr;
inline std::uintptr_t library = 0;
inline std::mutex installMutex;
inline std::size_t installedHooks = 0;

inline Options Current(void* manager) noexcept {
    const auto options = Read();
    if (!options.Active() || !manager || !available.load(std::memory_order_acquire) ||
        !license::Allows(license::Capability::Features)) return {};
    const auto match = reinterpret_cast<Getter>(library + 0x801191C)(nullptr, nullptr);
    const auto pawn = reinterpret_cast<Getter>(library + 0x8016060)(nullptr, nullptr);
    if (!match || !pawn || Field<void*>(manager, 0x38) != match ||
        pawn != reinterpret_cast<Getter>(library + 0x8012814)(nullptr, nullptr) ||
        Field<unsigned char>(pawn, 0x550) != 1) return {};
    return options;
}

inline void Nearby(void* manager, Metadata metadata) {
    if (originalNearby) Invoke(manager, Current(manager), [&] { originalNearby(manager, metadata); });
}

inline void Refresh(void* manager, bool* items, bool* arsenal, Metadata metadata) {
    if (originalRefresh)
        Invoke(manager, Current(manager), [&] { originalRefresh(manager, items, arsenal, metadata); });
}

inline float Interval(void* module, Metadata metadata) {
    const float original = originalInterval ? originalInterval(module, metadata) : 0.f;
    if (!Read().fast) return original;
    return PickupInterval(original, Current(Field<void*>(module, 0x10)).fast);
}

template<class Hook>
inline bool Install(std::uintptr_t base, Hook hook) {
    std::lock_guard<std::mutex> lock(installMutex);
    if (available.load(std::memory_order_acquire)) return true;
    if (!base || (library && library != base)) return false;
    const char* failure = nullptr;
    if (!PhantomTrigger::NativeProfileDetail::ValidateEntrypoints(
            base, profile.data() + installedHooks, profile.size() - installedHooks, failure)) return false;
    library = base;
    void* replacements[]{reinterpret_cast<void*>(Nearby), reinterpret_cast<void*>(Refresh), reinterpret_cast<void*>(Interval)};
    void** originals[]{reinterpret_cast<void**>(&originalNearby), reinterpret_cast<void**>(&originalRefresh), reinterpret_cast<void**>(&originalInterval)};
    while (installedHooks < std::size(replacements)) {
        hook(reinterpret_cast<void*>(base + profile[installedHooks].rva), replacements[installedHooks], originals[installedHooks]);
        if (!*originals[installedHooks]) return false;
        ++installedHooks;
    }
    available.store(true, std::memory_order_release);
    return true;
}
}
#endif
