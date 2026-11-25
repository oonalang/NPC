#pragma once




#include <cstddef>
#include <cstdint>
#include <cstring>

namespace KillIconRef {


constexpr int kMythicShowType_KillIcon = 6;


constexpr int kAwakenTask_KillIcon = 512;


constexpr int kUpLevelFlag_KillIcon = 12;


constexpr std::size_t kSSDecorationInfo_KillIconOffset = 0x34;


constexpr std::size_t kInventoryInfo_KillIconOffset = 0x4C;


constexpr std::uintptr_t kGetRedGunKillIcon_RVA_1_6_59 = 0x61D3780;


inline bool ReadKillIconId(const void* objectBytes,
                           std::size_t objectSize,
                           std::size_t fieldOffset,
                           std::uint32_t& outKillIconId) noexcept {
    if (objectBytes == nullptr || fieldOffset > objectSize ||
        sizeof(outKillIconId) > objectSize - fieldOffset) {
        return false;
    }

    std::memcpy(&outKillIconId,
                static_cast<const std::uint8_t*>(objectBytes) + fieldOffset,
                sizeof(outKillIconId));
    return true;
}

inline bool ReadFromSSDecorationInfo(const void* bytes,
                                     std::size_t size,
                                     std::uint32_t& outId) noexcept {
    return ReadKillIconId(bytes, size, kSSDecorationInfo_KillIconOffset, outId);
}

inline bool ReadFromInventoryInfo(const void* bytes,
                                  std::size_t size,
                                  std::uint32_t& outId) noexcept {
    return ReadKillIconId(bytes, size, kInventoryInfo_KillIconOffset, outId);
}

} // made by Francheska
