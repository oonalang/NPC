#pragma once


#include <cstdint>

namespace KillIconRef {


constexpr std::uintptr_t SSDecorationInfo_killicon = 0x34; 


constexpr std::uintptr_t InventoryInfo_KillIcon = 0x4C; 



constexpr std::uintptr_t WeaponModifyDelta_KillIconItem = 0x50;

constexpr std::uintptr_t WeaponAttrData_m_KillIconItem = 0x78; 

enum class ModifyInfoType : std::int32_t {
    None                  = 0,
    EAttachment           = 1,
    ECamo                 = 2,
    EOrnament             = 4,
    ECrossHair            = 8,
    ESticker              = 16,
    EName                 = 32,
    EKillEffect           = 64,
    EGunFlame             = 128,
    ETransmogrification   = 256,
    EKillIcon             = 512,
    EKillBoard            = 1024,
    EMyhticArmor          = 2048,
    EScopeView            = 4096,
    ECrossHairSize        = 8192,
    EMyhticSight          = 16384,
    EGunFlameSelfOnly     = 32768,
    EMuzzleFlashKeep      = 65536,
    EDMZSkin              = 131072,
    EKillEffectLevel      = 262144,
    ERecolor              = 524288
};


enum class MythicShowType : std::int32_t {
    None              = 0,
    Attche            = 1,
    GunFlame          = 2,
    KillEffect        = 3,
    Ornament          = 4,
    Sticker           = 5,
    KillIcon          = 6,
    KillBoard         = 7,
    FPSeeWeapon       = 8,
    SpecialSeeWeapon  = 9,
    MythicArmor       = 10,
    KillBoardCast     = 11,
    MythicSight       = 12,
    ShareFullLevelImage = 13,
    ShareWeapon       = 1003,
    LoadingShow       = 1004,
    PlayOfEndGame     = 1005,
    Other             = 1006
};


enum class RedGunUpLevelFlagConfType : std::int32_t {
    E_NONE                 = 0,
    E_ATTACH_CAMERA        = 1,
    E_ATTACH_FLAG          = 2,
    E_ORNAMENT_CAMERA      = 3,
    E_ORNAMENT_FLAG        = 4,
    E_STICKER_CAMERA       = 5,
    E_STICKER_FLAG         = 6,
    E_KILLEFFECT_CAMERA_POS = 7,
    E_KILLEFFECT_FLAG      = 8,
    E_GUNFLAME_CAMERA      = 9,
    E_GUNFLAME_FLAG        = 10,
    E_KILLEFFECT_CAMERA_ROT = 11,
    E_KILLICON_FLAG        = 12,
    E_KILLBOARD_FLAG       = 13,
    E_FPWEAPON_CAMERA_POS  = 14,
    E_FPWEAPON_FLAG        = 15
};


constexpr std::uintptr_t WeaponAttrData_get_KillIconItem_RVA   = 0x78578F4;
constexpr std::uintptr_t WeaponAttrData_get_KillIconItemID_RVA  = 0x78578FC;
constexpr std::uintptr_t GetRedGunKillIcon_RVA                 = 0x61D3780;

} //made by Francheska 
