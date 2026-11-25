#pragma once


#include <cstddef>
#include <cstdint>

namespace killicon {

// -----------------------------------------------------------------------------
namespace dump_1_6_59 {
    // msg.SSDecorationInfo
    constexpr std::uintptr_t SSDecorationInfo_killicon = 0x34;

    // InventoryInfo
    constexpr std::uintptr_t InventoryInfo_KillIcon = 0x4C;

    // WeaponModifyDelta
    constexpr std::uintptr_t WeaponModifyDelta_KillIconItem = 0x50;

    // WeaponAttrData
    constexpr std::uintptr_t WeaponAttrData_m_KillIconItem = 0x78;

    // RedGun.MythicShowType
    constexpr int MythicShowType_KillIcon = 6;

    // EModifyInfoType
    constexpr int EModifyInfoType_EKillIcon = 512;

    // ERedGunUpLevelFlagConfType
    constexpr int ERedGunUpLevelFlagConfType_E_KILLICON_FLAG = 12;

    // WeaponAttrData getter methods (RVA, version-specific)
    constexpr std::uintptr_t WeaponAttrData_get_KillIconItem_RVA = 0x78578F4;
    constexpr std::uintptr_t WeaponAttrData_get_KillIconItemID_RVA = 0x78578FC;

    // Asset collection method (not a setter/equip function)
    constexpr std::uintptr_t GetRedGunKillIcon_RVA = 0x61D3780;

    // InventoryInfo helper method RVAs
    constexpr std::uintptr_t GetWeaponKillIcon_SyncWeaponExtraInfo_RVA = 0xC0D34BC;
    constexpr std::uintptr_t GetWeaponKillIcon_DecorationInfo_RVA = 0xC0EDA20;
}

// -----------------------------------------------------------------------------
// 2) Standalone mapping model
// Fill this table ONLY with weapon IDs and Kill Icon item IDs verified from
// your own authorized configuration/data source. Zero is treated as invalid.
// -----------------------------------------------------------------------------
struct WeaponKillIconEntry {
    std::uint32_t weaponId;
    std::uint32_t killIconItemId;
};

// Add verified entries here, for example:
// { VERIFIED_WEAPON_ID, VERIFIED_KILL_ICON_ITEM_ID },
// Do not use guessed values. Keep the table empty until IDs are verified.
static constexpr WeaponKillIconEntry kWeaponKillIconMap[] = {
    { 0u, 0u } // placeholder only; replace/add verified IDs
};

static constexpr std::size_t kWeaponKillIconMapCount =
    sizeof(kWeaponKillIconMap) / sizeof(kWeaponKillIconMap[0]);

// Returns 0 when no verified mapping exists.
inline std::uint32_t FindKillIconItemId(std::uint32_t weaponId) noexcept {
    for (std::size_t i = 0; i < kWeaponKillIconMapCount; ++i) {
        if (kWeaponKillIconMap[i].weaponId == weaponId &&
            kWeaponKillIconMap[i].weaponId != 0u &&
            kWeaponKillIconMap[i].killIconItemId != 0u) {
            return kWeaponKillIconMap[i].killIconItemId;
        }
    }
    return 0u;
}

inline bool HasKillIconMapping(std::uint32_t weaponId) noexcept {
    return FindKillIconItemId(weaponId) != 0u;
}

// Simple validation helper for data entered into your own project.
inline bool IsValidEntry(const WeaponKillIconEntry& entry) noexcept {
    return entry.weaponId != 0u && entry.killIconItemId != 0u;
}

}

//kayo na bahala hahahaha 
