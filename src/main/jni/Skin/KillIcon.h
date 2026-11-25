#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <unordered_map>

namespace kill_icon {

inline constexpr std::size_t decorationInfoKillIcon = 0x34;
inline constexpr std::size_t inventoryKillIcon = 0x4C;
inline constexpr std::size_t modifyDeltaKillIcon = 0x50;
inline constexpr std::size_t weaponAttrKillIcon = 0x78;
inline constexpr std::int32_t modifyInfoKillIcon = 512;
inline constexpr std::int32_t mythicShowKillIcon = 6;
inline constexpr std::int32_t upLevelFlagKillIcon = 12;
inline constexpr std::uint32_t killIconItemType = 60u;
inline constexpr std::size_t maximumEntries = 2048;

inline std::mutex iconMutex;
inline std::unordered_map<std::uint32_t, std::uint32_t> icons;

inline std::uint32_t ReadField(const void* object, std::size_t offset) noexcept {
    std::uint32_t value = 0;
    if (object) std::memcpy(&value, static_cast<const unsigned char*>(object) + offset, sizeof(value));
    return value;
}

inline void WriteField(void* object, std::size_t offset, std::uint32_t value) noexcept {
    if (object) std::memcpy(static_cast<unsigned char*>(object) + offset, &value, sizeof(value));
}

inline bool ValidIcon(std::uint32_t id) noexcept {
    return id != 0 && id / 10000000u == killIconItemType;
}

inline void Remember(std::uint32_t weaponId, std::uint32_t iconId) noexcept {
    if (!weaponId) return;
    std::lock_guard<std::mutex> lock(iconMutex);
    if (!ValidIcon(iconId)) {
        icons.erase(weaponId);
        return;
    }
    if (icons.find(weaponId) == icons.end() && icons.size() >= maximumEntries) return;
    icons[weaponId] = iconId;
}

inline std::uint32_t IconFor(std::uint32_t weaponId) noexcept {
    if (!weaponId) return 0;
    std::lock_guard<std::mutex> lock(iconMutex);
    const auto found = icons.find(weaponId);
    return found != icons.end() ? found->second : 0u;
}

inline int Captured(const void* weaponConf) noexcept {
    const std::uint32_t raw = ReadField(weaponConf, weaponAttrKillIcon);
    return ValidIcon(raw) ? static_cast<int>(raw) : 0;
}

struct Applied {
    void* inventory = nullptr;
    std::uint32_t key = 0;
    std::uint32_t original = 0;
    std::uint32_t written = 0;
};

inline Applied& AppliedState() noexcept {
    static Applied state;
    return state;
}

inline bool Sync(void* inventory, const std::array<std::uint32_t, 3>& keys) noexcept {
    if (!inventory) return false;
    std::lock_guard<std::mutex> lock(iconMutex);
    auto& state = AppliedState();
    const std::uint32_t current = ReadField(inventory, inventoryKillIcon);
    if (state.inventory != inventory) {
        state = Applied{};
        state.inventory = inventory;
    }
    std::uint32_t key = 0, icon = 0;
    for (std::size_t i = 0; i < keys.size() && !key; ++i) {
        const auto found = icons.find(keys[i]);
        if (found != icons.end() && ValidIcon(found->second)) {
            key = keys[i];
            icon = found->second;
        }
    }
    if (!key) {
        if (state.written && current == state.written)
            WriteField(inventory, inventoryKillIcon, state.original);
        state = Applied{};
        return false;
    }
    state.inventory = inventory;
    state.key = key;
    if (!state.written || current != state.written) state.original = current;
    if (current != icon) WriteField(inventory, inventoryKillIcon, icon);
    state.written = icon;
    return true;
}

inline void Forget() noexcept {
    std::lock_guard<std::mutex> lock(iconMutex);
    AppliedState() = Applied{};
}

#if defined(__ANDROID__) && defined(__aarch64__) && !defined(STUDIO_STANDALONE_PREVIEW)
#include "../Trigger/NativeProfile.h"
#include "../Security/Access.h"

namespace native {

using Metadata = void*;
using Getter = void* (*)(void*, Metadata);

inline constexpr std::size_t pawnPlayerInfo = 0x5c8;
inline constexpr std::uintptr_t localPawnRva = 0x8012A6C;
inline constexpr std::uintptr_t currentInventoryRva = 0xA0BBAF8;

inline constexpr std::array<PhantomTrigger::NativeProfileDetail::CodeCheck, 2> profile{{
    {"Kill icon local player differs", localPawnRva,
     {0xfe,0x4f,0xbf,0xa9,0xf3,0xa4,0x03,0xb0,0x68,0xce,0x50,0x39,0xe8,0x00,0x00,0x37}},
    {"Kill icon inventory differs", currentInventoryRva,
     {0x00,0xe8,0x41,0xf9,0xc0,0x03,0x5f,0xd6,0x00,0xec,0x41,0xf9,0xc0,0x03,0x5f,0xd6}}
}};

inline std::atomic<bool> available{false};
inline std::atomic<bool> attempted{false};
inline std::mutex installMutex;
inline std::uintptr_t library = 0;
inline Getter localPawn = nullptr;
inline Getter currentInventory = nullptr;

inline bool Install(std::uintptr_t base) noexcept {
    if (!base) return false;
    std::lock_guard<std::mutex> lock(installMutex);
    if (available.load(std::memory_order_acquire)) return true;
    if (library && library != base) return false;
    if (!license::Allows(license::Capability::Features)) return false;
    attempted.store(true, std::memory_order_release);
    const char* failure = nullptr;
    if (!PhantomTrigger::NativeProfileDetail::ValidateEntrypoints(
            base, profile.data(), profile.size(), failure)) return false;
    localPawn = reinterpret_cast<Getter>(base + localPawnRva);
    currentInventory = reinterpret_cast<Getter>(base + currentInventoryRva);
    library = base;
    available.store(true, std::memory_order_release);
    return true;
}

inline void* LocalInventory() {
    if (!available.load(std::memory_order_acquire)) return nullptr;
    if (!license::Allows(license::Capability::Features)) return nullptr;
    if (!localPawn || !currentInventory) return nullptr;
    void* pawn = localPawn(nullptr, nullptr);
    if (!pawn) return nullptr;
    void* info = nullptr;
    std::memcpy(&info, static_cast<unsigned char*>(pawn) + pawnPlayerInfo, sizeof(info));
    if (!info) return nullptr;
    return currentInventory(info, nullptr);
}

inline void SyncLocal() {
    void* inventory = LocalInventory();
    if (!inventory) {
        Forget();
        return;
    }
    const std::array<std::uint32_t, 3> keys{
        ReadField(inventory, 0x14), ReadField(inventory, 0x38), ReadField(inventory, 0x34)};
    Sync(inventory, keys);
}

}

#else

namespace native {

inline bool Install(std::uintptr_t) noexcept { return false; }
inline void* LocalInventory() noexcept { return nullptr; }
inline void SyncLocal() noexcept {}

}

#endif

}
