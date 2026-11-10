#pragma once

#include <cstdint>
#include <string>

extern bool MpB;
extern float MpBsize;
extern bool SnowB;
extern float SnowBsize;
extern bool isSpeedHackEnabled;
extern float speedHackMultiplier;
extern bool isJumpAdjustmentEnabled;
extern float jumpHeightMultiplier;
extern float SlideRange;
extern bool UnlimitedAmmo;
extern std::chrono::steady_clock::time_point lastAmmoRefillTime;

//-- Smart Reload
namespace SmartReloadCfg
{
    constexpr uintptr_t CarriedAmmoCountRva = 0x5474F04;
    constexpr uintptr_t PawnTryChangeClipRva = 0x543B08C;
}
namespace AutoJumpFireCfg
{
    constexpr uintptr_t IsJumpingActiveRva = 0x53A63C4;
    constexpr uintptr_t DoJumpRva = 0x53FBA0C;
    constexpr uintptr_t StartFireRva = 0x5439550;

}
inline bool g_SmartReloadPending = false;

inline int SmartReloadCarriedAmmo(void *weapon)
{
    using GetAmmoFn = int (*)(void *);
    static GetAmmoFn getCarriedAmmo = reinterpret_cast<GetAmmoFn>(
        getAbsoluteAddress("libunity.so", SmartReloadCfg::CarriedAmmoCountRva));
    return getCarriedAmmo != nullptr ? getCarriedAmmo(weapon) : 0;
}

inline void SmartReloadRequestChangeClip(Pawn *local)
{
    using TryChangeClipFn = void (*)(Pawn *, bool);
    static TryChangeClipFn tryChangeClip = reinterpret_cast<TryChangeClipFn>(
        getAbsoluteAddress("libunity.so", SmartReloadCfg::PawnTryChangeClipRva));

    if (tryChangeClip != nullptr)
        tryChangeClip(local, true);
}

inline void SmartReloadMarkShotFired()
{
    if (!Config.ExtraMenu.SmartReload)
        return;

    Pawn *local = GamePlay::get_LocalPawn();
    if (!Tools::IsPtrValid(local) || !local->m_IsAlive())
        return;

    g_SmartReloadPending = true;
}

inline bool SmartReloadCanRequestForPawn(Pawn *local)
{
    if (!Config.ExtraMenu.SmartReload || !g_SmartReloadPending)
        return false;

    if (!Tools::IsPtrValid(local) || !local->m_IsAlive()) {
        g_SmartReloadPending = false;
        return false;
    }

    Pawn *currentLocal = GamePlay::get_LocalPawn();
    if (currentLocal != local) {
        g_SmartReloadPending = false;
        return false;
    }

    Weapon *weapon = local->get_CurrentWeapon();
    if (!Tools::IsPtrValid(weapon)) {
        g_SmartReloadPending = false;
        return false;
    }

    if (SmartReloadCarriedAmmo(weapon) <= 0) {
        g_SmartReloadPending = false;
        return false;
    }

    return true;
}

inline void SmartReloadRequestAfterStopFire(Pawn *local)
{
    if (!SmartReloadCanRequestForPawn(local))
        return;

    g_SmartReloadPending = false;
    SmartReloadRequestChangeClip(local);
}

inline void (*orig_Pawn_StopFire)(Pawn *instance, bool isImmidiately) = nullptr;
inline void hook_Pawn_StopFire(Pawn *instance, bool isImmidiately)
{
    orig_Pawn_StopFire(instance, isImmidiately);
    SmartReloadRequestAfterStopFire(instance);
}


//-- Clear Terrain
inline void (*orig_SetGrassShowState)(bool showGrass) = nullptr;
inline void hook_SetGrassShowState(bool showGrass)
{
    if (Config.ExtraMenu.ClearTerrain)
        showGrass = false;
    return orig_SetGrassShowState(showGrass);
}

inline void (*orig_SetGrassLODBias)(float lodBias, float midLodBias, bool showGrass) = nullptr;
inline void hook_SetGrassLODBias(float lodBias, float midLodBias, bool showGrass)
{
    if (Config.ExtraMenu.ClearTerrain) {
        lodBias = 0.0f;
        midLodBias = 0.0f;
        showGrass = false;
    }
    return orig_SetGrassLODBias(lodBias, midLodBias, showGrass);
}

inline void ApplyWorldVisualsRuntime()
{
    static bool lastClearTerrain = false;
    if (Config.ExtraMenu.ClearTerrain) {
        if (orig_SetGrassShowState)
            orig_SetGrassShowState(false);
        if (orig_SetGrassLODBias)
            orig_SetGrassLODBias(0.0f, 0.0f, false);
    } else if (lastClearTerrain) {
        if (orig_SetGrassShowState)
            orig_SetGrassShowState(true);
        if (orig_SetGrassLODBias)
            orig_SetGrassLODBias(1.0f, 1.0f, true);
    }
    lastClearTerrain = Config.ExtraMenu.ClearTerrain;
}

//-- Aim Assist
inline float (*orig_GetAssitAimSpeed)(void *, Vector3, float, float, float, bool, bool) = nullptr;
inline float GetAssitAimSpeed(void * instance, Vector3 assistCentorPos, float assistDis, float dis, float angle, bool isPVE, bool gamepadInput) {
    if (instance != NULL) {
        if (Config.Aim.AimAssistSize > 0.0f) {
            return (float)Config.Aim.AimAssistSize;
        }
    }
    return orig_GetAssitAimSpeed(instance, assistCentorPos, assistDis, dis, angle, isPVE, gamepadInput);
}

//-- Anti Flashbang
inline void (*orig_OnFlashBangExplode)(void *, int, float, float, float) = nullptr;
inline void hook_OnFlashBangExplode(void *instance, int weaponItemID, float whiteTime, float whiteAlphaTime, float initIntensity) {
    if (instance != nullptr && Config.ExtraMenu.Flash) {
        whiteTime = 0.1f;
        whiteAlphaTime = 0.1f;
        initIntensity = 0.1f;
    }
    orig_OnFlashBangExplode(instance, weaponItemID, whiteTime, whiteAlphaTime, initIntensity);
}

//-- Firerate Speed
inline float (*orig_get_FireBoltTime)(void *) = nullptr;
inline float get_FireBoltTime(void * instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Fire) {
            return 0.00001f;
        }
    }
    return orig_get_FireBoltTime(instance);
}

inline float (*orig_get_FireInterval)(void *) = nullptr;
inline float get_FireInterval(void * instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Fire) {
            return 0.00001f;
        }
    }
    return orig_get_FireInterval(instance);
}

inline float (*orig_get_DelaySprintFire)(void *) = nullptr;
inline float get_DelaySprintFire(void * instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Fire) {
            return 0.00001f;
        }
    }
    return orig_get_DelaySprintFire(instance);
}

typedef void (*SetUltraFrameRateDeviceInfo_t)(void* thiz, bool enableUltraFrameRate, int ultraFrameRate, int ultraFrameRateBR, int ultraFrameRateQualityLimit, bool customizedFrameRate);
SetUltraFrameRateDeviceInfo_t orig_SetUltraFrameRateDeviceInfo;

void hooked_SetUltraFrameRateDeviceInfo(void* thiz, bool enableUltraFrameRate, int ultraFrameRate, int ultraFrameRateBR, int ultraFrameRateQualityLimit, bool customizedFrameRate) {
    if (thiz != NULL) {
        enableUltraFrameRate = true;
        ultraFrameRate = 144;
        ultraFrameRateBR = 144;
        ultraFrameRateQualityLimit = 144;
        customizedFrameRate = true;
    }
    orig_SetUltraFrameRateDeviceInfo(thiz, enableUltraFrameRate, ultraFrameRate, ultraFrameRateBR, ultraFrameRateQualityLimit, customizedFrameRate);
}

//-- High Jump
inline float (*orig_GetMaxJumpHeight)(void*) = nullptr;
inline float hook_GetMaxJumpHeight(void* instance) {
    float orig_ = orig_GetMaxJumpHeight(instance);
    return (jumpHeightMultiplier > 1.0f) ? orig_ * jumpHeightMultiplier : orig_;
}
//-- MP VIEW
inline float (*get_m_MaxPovmp)(void*) = nullptr;
inline float hook_get_m_MaxPovmp(void* instance) {
    if (MpBsize > 0.0f) {
        return MpBsize;
    }
    return get_m_MaxPovmp(instance);
}

//-- Increase Damage
inline bool (*orig_SingleLineCheckPhysics)(void* instance, int hitType, void* hitTarget, void* hitCollider, Vector3 startPos, Vector3 dir, void* impactInfo) = nullptr;
inline bool SingleLineCheckPhysics(void* instance, int hitType, void* hitTarget, void* hitCollider, Vector3 startPos, Vector3 dir, void* impactInfo) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Hit) {
            return true;
        }
    }
    return orig_SingleLineCheckPhysics(instance, hitType, hitTarget, hitCollider, startPos, dir, impactInfo);
}

//-- Long Slide
inline float (*o_get_SlideTackleAcclerationSpeed)(void*) = nullptr;
inline float h_get_SlideTackleAcclerationSpeed(void* ins) {
    if (SlideRange > 0.0f) {
        return SlideRange + 1;
    }
    return o_get_SlideTackleAcclerationSpeed(ins);
}

inline float (*o_PawnGetMaxSpeed)(void*) = nullptr;
inline float h_PawnGetMaxSpeed(void* ins) {
    if (SlideRange > 0.0f) {
        return SlideRange + 256;
    }
    return o_PawnGetMaxSpeed(ins);
}

inline float (*o_get_SlideTackleSpeed)(void*) = nullptr;
inline float h_get_SlideTackleSpeed(void* ins) {
    if (SlideRange > 0.0f) {
        return SlideRange;
    }
    return o_get_SlideTackleSpeed(ins);
}

inline float (*o_GetSuperSlideRate)(void*) = nullptr;
inline float h_GetSuperSlideRate(void* ins) {
    if (SlideRange > 0.0f) {
        return SlideRange + 256;
    }
    return o_GetSuperSlideRate(ins);
}

inline void (*o_TickLocalPlayer)(void*, float) = nullptr;
inline void h_TickLocalPlayer(void* ins, float deltaTime) {
    if (SlideRange <= 0.0f) {
        o_TickLocalPlayer(ins, deltaTime);
    }
}

//-- No Overheat
inline float (*orig_get_AddHotTime)(void* instance) = nullptr;
inline float get_AddHotTime(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Rpd) {
            return 0.00001f;
        }
    }
    return orig_get_AddHotTime(instance);
}

//-- No Parachute
inline void (*orig_OpenParachute)(void* instance, bool isAuto) = nullptr;
inline void OpenParachute(void* instance, bool isAuto) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Parachute) {
            return;
        }
    }
    return orig_OpenParachute(instance, isAuto);
}

//-- Quick Reload
inline float (*orig_get_ChangeClipTime)(void* instance) = nullptr;
inline float get_ChangeClipTime(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Reload) {
            return 0.0001f;
        }
    }
    return orig_get_ChangeClipTime(instance);
}

inline float (*orig_get_ChangeClipLoopTime)(void* instance) = nullptr;
inline float get_ChangeClipLoopTime(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Reload) {
            return 0.0001f;
        }
    }
    return orig_get_ChangeClipLoopTime(instance);
}

//-- Quick Scope
inline float (*orig_get_AimingTime)(void* instance) = nullptr;
inline float get_AimingTime(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Scope) {
            return 0.0001f;
        }
    }
    return orig_get_AimingTime(instance);
}

//-- Quick Switch
inline float (*orig_get_EquipTime)(void* instance) = nullptr;
inline float get_EquipTime(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Switch) {
            return 0.0001f;
        }
    }
    return orig_get_EquipTime(instance);
}

inline float (*orig_get_UnequipTime)(void* instance) = nullptr;
inline float get_UnequipTime(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Switch) {
            return 0.0001f;
        }
    }
    return orig_get_UnequipTime(instance);
}

//-- Red Wallhack
bool (*orig_IsInEM3Eye)(void *instance);
bool get_IsInEM3Eye(void *instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.RedWallhack) {
            return true;
        }
    }
    return orig_IsInEM3Eye(instance);
}

float (*orig_GetAccDistance)(void *instance);
float GetAccDistance(void *instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.RedWallhack) {
            return 50.0f;
        }
    }
    return orig_GetAccDistance(instance);
}

//-- Skip Tutorial
inline bool IsTutorialEnabled() {
    return false;
}

//-- Sky Diving Speed
inline float (*orig_get_AccelerationForwardSpeedUp)(void* instance) = nullptr;
inline float get_AccelerationForwardSpeedUp(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Diving) {
            return 200.0f;
        }
    }
    return orig_get_AccelerationForwardSpeedUp(instance);
}

inline float (*orig_get_MaxVelocityForwardSpeedUp)(void* instance) = nullptr;
inline float get_MaxVelocityForwardSpeedUp(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Diving) {
            return 200.0f;
        }
    }
    return orig_get_MaxVelocityForwardSpeedUp(instance);
}

//-- Snowboard Boost
inline float (*get_m_PhysSkisMaxSpeed)(void*) = nullptr;
inline float hooked_get_m_PhysSkisMaxSpeed(void* instance) {
    if (SnowBsize > 0.0f) {
        return SnowBsize;
    }
    return get_m_PhysSkisMaxSpeed(instance);
}

//-- Speed Hack
inline float (*original_CalcFinalMoveScale)(void*) = nullptr;
inline float hooked_CalcFinalMoveScale(void* instance) {
    if (instance == nullptr) {
        return original_CalcFinalMoveScale(instance);
    }
    if (speedHackMultiplier > 1.0f && speedHackMultiplier <= 100.0f) {
        return speedHackMultiplier;
    }
    return original_CalcFinalMoveScale(instance);
}

//-- Weapon Kinetic
inline bool (*orig_get_IsKineticArmor)(void* instance) = nullptr;
inline bool get_IsKineticArmor(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Kinetic) {
            return true;
        }
    }
    return orig_get_IsKineticArmor(instance);
}

//-- Zero Recoil
inline float (*orig_GetScaleRecoil)(void* instance) = nullptr;
inline float GetScaleRecoil(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Recoil) {
            return 0.00001f;
        }
    }
    return orig_GetScaleRecoil(instance);
}

//-- Zero Spread
inline float (*orig_MinInaccuracy)(void *) = nullptr;
inline float MinInaccuracy(void * instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Spread) {
            return 0.00001f;
        }
    }
    return orig_MinInaccuracy(instance);
}

inline float (*orig_MaxInaccuracy)(void *) = nullptr;
inline float MaxInaccuracy(void * instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Spread) {
            return 0.00001f;
        }
    }
    return orig_MaxInaccuracy(instance);
}

inline float (*orig_DisperseBase)(void *) = nullptr;
inline float DisperseBase(void * instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Spread) {
            return 0.00001f;
        }
    }
    return orig_DisperseBase(instance);
} 

// Unlock Blueprints
inline bool (*orig_IsUnlocked)(void *instance);
inline bool IsUnlocked(void *instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Blueprints) {
            return true;
        }
    }
    return orig_IsUnlocked(instance);
}

inline bool (*orig_IsWeaponAttachmentUnlock)(void *instance);
inline bool IsWeaponAttachmentUnlock(void *instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Attachment) {
            return true;
        }
    }
    return orig_IsWeaponAttachmentUnlock(instance);
}

//-- Fast Armor
inline constexpr float PlateTime = 2.75f; // Armor Speed
inline constexpr uintptr_t PlateTimeOff = 0x30;
inline constexpr uintptr_t UseCompTimeOff = 0xA8;

inline void SetObjFloat(void *instance, uintptr_t offset, float value)
{
    if (instance != nullptr)
        *reinterpret_cast<float *>(reinterpret_cast<uintptr_t>(instance) + offset) = value;
}

inline int (*orig_BRUseItemArmorPlate_SetUseItemTime)(void *instance, uint32_t playerId) = nullptr;
inline void *g_fastPlateInst = nullptr;

inline int hook_BRUseItemArmorPlate_SetUseItemTime(void *instance, uint32_t playerId)
{
    int ret = orig_BRUseItemArmorPlate_SetUseItemTime(instance, playerId);
    if (Config.ExtraMenu.FastArmorPlate && instance != nullptr && ret == 1) {
        g_fastPlateInst = instance;
        SetObjFloat(instance, PlateTimeOff, PlateTime);
    }
    return ret;
}

inline void (*orig_BRUseItemArmorPlate_Tick)(void *instance, void *useItemComponent) = nullptr;
inline void hook_BRUseItemArmorPlate_Tick(void *instance, void *useItemComponent)
{
    orig_BRUseItemArmorPlate_Tick(instance, useItemComponent);

    if (Config.ExtraMenu.FastArmorPlate && instance == g_fastPlateInst) {
        SetObjFloat(instance, PlateTimeOff, PlateTime);
        SetObjFloat(useItemComponent, UseCompTimeOff, PlateTime);
    }
}

  //--AutoJump
inline bool g_AutoJumpFireWaitingLand = false;
inline bool g_AutoJumpFireSawJumpActive = false;

inline void (*AutoJumpFire_DoJump)(Pawn *instance) = nullptr;
inline bool (*AutoJumpFire_IsJumpingActive)(Pawn *instance) = nullptr;

inline bool AutoJumpFireIsLocalFiring(Pawn *instance)
{
    if (!Tools::IsPtrValid(instance))
        return false;

    auto isFiring = reinterpret_cast<bool (*)(uintptr_t)>(Class_Pawn_get_IsFiring);
    return isFiring != nullptr && isFiring(reinterpret_cast<uintptr_t>(instance));
}

inline bool AutoJumpFireJumpingActive(Pawn *instance)
{
    if (!Tools::IsPtrValid(instance))
        return false;

    if (AutoJumpFire_IsJumpingActive == nullptr) {
        AutoJumpFire_IsJumpingActive = reinterpret_cast<bool (*)(Pawn *)>(
            getAbsoluteAddress("libunity.so", AutoJumpFireCfg::IsJumpingActiveRva));
    }

    return AutoJumpFire_IsJumpingActive != nullptr && AutoJumpFire_IsJumpingActive(instance);
}

inline void TryAutoJumpFire(Pawn *instance, bool force)
{
    if (!Config.ExtraMenu.AutoJumpFire || !Tools::IsPtrValid(instance) || !instance->m_IsAlive())
        return;

    Pawn *local = GamePlay::get_LocalPawn();
    if (instance != local)
        return;

    if (!force && g_AutoJumpFireWaitingLand)
        return;

    if (AutoJumpFire_DoJump == nullptr) {
        AutoJumpFire_DoJump = reinterpret_cast<void (*)(Pawn *)>(
            getAbsoluteAddress("libunity.so", 0x53F99B0));
    }

    if (AutoJumpFire_DoJump != nullptr) {
        AutoJumpFire_DoJump(instance);
        g_AutoJumpFireWaitingLand = true;
        g_AutoJumpFireSawJumpActive = false;
    }
}

inline void ApplyAutoJumpFireRuntime()
{
    Pawn *local = GamePlay::get_LocalPawn();
    if (!Config.ExtraMenu.AutoJumpFire || !Tools::IsPtrValid(local) || !local->m_IsAlive()) {
        g_AutoJumpFireWaitingLand = false;
        g_AutoJumpFireSawJumpActive = false;
        return;
    }

    if (!AutoJumpFireIsLocalFiring(local)) {
        g_AutoJumpFireWaitingLand = false;
        g_AutoJumpFireSawJumpActive = false;
        return;
    }

    if (g_AutoJumpFireWaitingLand) {
        bool jumpingActive = AutoJumpFireJumpingActive(local);
        if (jumpingActive) {
            g_AutoJumpFireSawJumpActive = true;
            return;
        }

        if (!g_AutoJumpFireSawJumpActive)
            return;

        g_AutoJumpFireWaitingLand = false;
        g_AutoJumpFireSawJumpActive = false;
    }

    TryAutoJumpFire(local, false);
}

inline void (*orig_Pawn_StartFire)(Pawn *instance) = nullptr;
inline void hook_Pawn_StartFire(Pawn *instance)
{
    orig_Pawn_StartFire(instance);
    TryAutoJumpFire(instance, true);
}



/*
//-- Unlimited Bullets
inline bool (*orig_IsInfiniteBullet)(void* instance) = nullptr;
inline bool IsInfiniteBullet(void* instance) {
    if (Config.ExtraMenu.UnliAmmo) {
        return true;
    }
    if (instance != NULL) {
        return orig_IsInfiniteBullet(instance);
    }
    return false;
}

inline bool (*orig_IsNoCostAmmo)(void* instance) = nullptr;
inline bool IsNoCostAmmo(void* instance) {
    if (Config.ExtraMenu.UnliAmmo) {
        return true;
    }
    if (instance != NULL) {
        return orig_IsNoCostAmmo(instance);
    }
    return false;
}

//-- Unlimited Ammo (Functions)
inline bool (*orig_AmmoCanFire)(uintptr_t thiz) = nullptr;
inline bool hk_AmmoCanFire(uintptr_t thiz) {
    if (UnlimitedAmmo) {
        return true;
    }
    return orig_AmmoCanFire(thiz);
}

inline bool (*orig_HasAmmo)(uintptr_t thiz) = nullptr;
inline bool hk_HasAmmo(uintptr_t thiz) {
    if (UnlimitedAmmo) {
        return true;
    }
    return orig_HasAmmo(thiz);
}

inline bool (*orig_HasAmmo_IgnoreInfinite)(uintptr_t thiz) = nullptr;
inline bool hk_HasAmmo_IgnoreInfinite(uintptr_t thiz) {
    if (UnlimitedAmmo) {
        return true;
    }
    return orig_HasAmmo_IgnoreInfinite(thiz);
}

inline void (*orig_ServerStopFire)(uintptr_t thiz, int costAmmo) = nullptr;
inline void hk_ServerStopFire(uintptr_t thiz, int costAmmo) {
    if (UnlimitedAmmo) {
        return orig_ServerStopFire(thiz, 0);
    }
    return orig_ServerStopFire(thiz, costAmmo);
}

inline int (*orig_get_ShotCost)(uintptr_t thiz) = nullptr;
inline int hk_get_ShotCost(uintptr_t thiz) {
    if (UnlimitedAmmo) {
        return 0;
    }
    return orig_get_ShotCost(thiz);
}

inline bool (*orig_IsAmmoFree)(uintptr_t thiz) = nullptr;
inline bool hk_IsAmmoFree(uintptr_t thiz) {
    if (UnlimitedAmmo) {
        return true;
    }
    return orig_IsAmmoFree(thiz);
}

float (*orig_get_SmokeEffectScale)(void *instance);
float get_SmokeEffectScale(void *instance) {
    if (instance != nullptr && Config.ExtraMenu.NoSmoke) {
        return 0.0f;
    }
    return orig_get_SmokeEffectScale(instance);
}
*/
// No Spectate Deelay
inline bool (*o_NeedDelayProcess)(void *instance, int a, int b);
inline bool h_NeedDelayProcess(void *instance, int a, int b) {
    bool orig_val = o_NeedDelayProcess(instance, a, b);

    if (Config.ExtraMenu.Spectatex) {
        return 0; // Return 0 instead of false for int return type
    }

    return orig_val;
}

inline bool (*o_ProcessForDelay)(void *instance, byte* packData, int PackDataCount, int Sequence, float timeQueued, int aiIndex);
inline bool h_ProcessForDelay(void *instance, byte* packData, int PackDataCount, int Sequence, float timeQueued, int aiIndex) {
    bool orig_val = o_ProcessForDelay(instance, packData, PackDataCount, Sequence, timeQueued, aiIndex);

    if (Config.ExtraMenu.Spectatex) {
        return false;
    }

    return orig_val;
}

inline bool (*o_get_IsObserver)(void *);
inline bool h_get_IsObserver(void *instance) {
    bool orig_val = o_get_IsObserver(instance);

    if (Config.ExtraMenu.Spectatex) {
        return true;
    }

    return orig_val;
}

inline float (*o_GetDelayCountDown)(void *);
inline float h_GetDelayCountDown(void *instance) {
    float orig_val = o_GetDelayCountDown(instance);

    if (Config.ExtraMenu.Spectatex) {
        return 0;
    }

    return orig_val;
}

int (*orig_get_DeviceCapacityLevel)(void *instance);
int get_DeviceCapacityLevel(void *instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Grap) {
            return 6;
        }
    }
    return orig_get_DeviceCapacityLevel(instance);
}

// Hook for CheckTargetIsValid_DyingInAzurGameMode
inline bool (*orig_CheckTargetIsValid_DyingInAzurGameMode)(void* instance, void* targetPawn);
inline bool CheckTargetIsValid_DyingInAzurGameMode_Hook(void* instance, void* targetPawn) {
    if (instance != nullptr && targetPawn != nullptr) {
        if (isExecute) {
            return true;
        }
    }
    return orig_CheckTargetIsValid_DyingInAzurGameMode(instance, targetPawn);
}

// Hook for CheckTargetIsValid_PhysState
inline bool (*orig_CheckTargetIsValid_PhysState)(void* instance, void* targetPawn);
inline bool CheckTargetIsValid_PhysState_Hook(void* instance, void* targetPawn) {
    if (instance != nullptr && targetPawn != nullptr) {
        if (isExecute) {
            return true;
        }
    }
    return orig_CheckTargetIsValid_PhysState(instance, targetPawn);
}

// Hook for CheckTargetIsValid_ManualParameter
inline bool (*orig_CheckTargetIsValid_ManualParameter)(void* instance, void* tempPawn, float* distance);
inline bool CheckTargetIsValid_ManualParameter_Hook(void* instance, void* tempPawn, float* distance) {
    if (instance != nullptr && tempPawn != nullptr) {
        if (isExecute) {
            return true;
        }
    }
    return orig_CheckTargetIsValid_ManualParameter(instance, tempPawn, distance);
}

// Hook for CheckTargetIsValid_Ult
inline bool (*orig_CheckTargetIsValid_Ult)(void* instance, void* targetPawn);
inline bool CheckTargetIsValid_Ult_Hook(void* instance, void* targetPawn) {
    if (instance != nullptr && targetPawn != nullptr) {
        if (isExecute) {
            return true;
        }
    }
    return orig_CheckTargetIsValid_Ult(instance, targetPawn);
}



inline bool (*orig_CalculateObstacleSide)(void* instance, float* backDis, float* beforeDis);
inline bool CalculateObstacleSide_Hook(void* instance, float* backDis, float* beforeDis) {
    if (instance != nullptr) {
        if (isExecute) {
            return false;
        }
    }
    return orig_CalculateObstacleSide(instance, backDis, beforeDis);
}

inline bool (*orig_CalculateObstacleTop)(void* instance, Vector3 pawnPos);
inline bool CalculateObstacleTop_Hook(void* instance, Vector3 pawnPos) {
    if (instance != nullptr) {
        if (isExecute) {
            return false;
        }
    }
    return orig_CalculateObstacleTop(instance, pawnPos);
}

int (*oCheckTargetIsValid)(void *ins, void* tempPawn, float distance);
int CheckTargetIsValid(void *ins, void* tempPawn, float distance) {
    if ( ins != nullptr ) {
        if (isExecute) {
            return (int)true;
        }
    }
    return oCheckTargetIsValid(ins, tempPawn, distance);
}

inline bool (*oCheckCanExcution_Extra)(void *ins);
inline bool CheckCanExcution_Extra(void *ins) {
    if ( ins != nullptr) {
        if (isExecute) {
            return true;
        }
    }
    return oCheckCanExcution_Extra(ins);
}


inline bool (*o_CheckExecution_ObstacleAround)(void *ins);
inline bool CheckExecution_ObstacleAround(void *ins) {
    if (ins!=nullptr) {
        if (isExecute) {
            return true;
        }
    }
    return o_CheckExecution_ObstacleAround(ins);
}
/*
//ADJUSTABLE HOOK FOR FRAMES
 int (*original_Fpslevel)(void*) = nullptr;
 int hooked_Fpslevel(void* ins) {
    return (ins && Config.ExtraMenu.EnhanceFPS) ? 7 : original_Fpslevel(ins);
}

 int (*original_Fpslevel1)(void*) = nullptr;
 int hooked_Fpslevel1(void* ins) {
    return (ins && Config.ExtraMenu.EnhanceFPS) ? 7 : original_Fpslevel1(ins);
}

 int (*orig_GetFrameRateValue)(void*, int, int) = nullptr;
 int GetFrameRateValue(void* ins, int f, int m) {
    if (ins) {
        return 360;
    }
    return orig_GetFrameRateValue(ins, f, m);
}

 int (*orig_GetUltraFrameRateFinalFPS)(void*) = nullptr;
 int GetUltraFrameRateFinalFPS(void* g) {
    return (g && Config.ExtraMenu.EnhanceFPS) ? 360 : orig_GetUltraFrameRateFinalFPS(g);
}

inline bool (*original_Extremequality)(void*) = nullptr;
inline bool hooked_Extremequality(void* ins) {
    return (ins && Config.ExtraMenu.EnhanceFPS) ? true : original_Extremequality(ins);
}
*/
/// Walk Under Water
inline bool (*oPawn_GetCurrentDistToWaterSurface)(void *instance);
inline bool Pawn_GetCurrentDistToWaterSurface(void *instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.WalkUnderWater) {
        	return false;
        }
    }
    return oPawn_GetCurrentDistToWaterSurface(instance);
}

inline bool (*oPlayerPawn_IsUnderWaterSurface)(void *instance);
inline bool PlayerPawn_IsUnderWaterSurface(void *instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.WalkUnderWater) {
        	return false;
        }
    }
    return oPlayerPawn_IsUnderWaterSurface(instance);
}

inline bool (*oCheckInWaterComponent_get_CurrentWaterSurfaceHeight)(void *instance);
inline bool CheckInWaterComponent_get_CurrentWaterSurfaceHeight(void *instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.WalkUnderWater) {
        	return true;
        }
    }
    return oCheckInWaterComponent_get_CurrentWaterSurfaceHeight(instance);
}

///Camera Pov
float (*oInputSettingConfig_Instant_GetMainCameraFov_3p)(void *instance);
float InputSettingConfig_Instant_GetMainCameraFov_3p(void *instance) {
    if (instance && Config.ExtraMenu.CameraPov) {
        return (float)Config.ExtraMenu.CameraPovSize;
    }
    return oInputSettingConfig_Instant_GetMainCameraFov_3p(instance);
}

//-- No Smoke
inline bool NameHasSmokeToken(const char *name)
{
    if (!name)
        return false;

    std::string s(name);
    for (char &c : s) {
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c - 'A' + 'a');
    }

    return s.find("smoke") != std::string::npos ||
           s.find("smog") != std::string::npos ||
           s.find("smk") != std::string::npos;
}

inline String *(*UnityObject_get_name)(void *instance) = nullptr;
inline void (*orig_GameObjectSetActive)(void *instance, bool value) = nullptr;
inline void hook_GameObjectSetActive(void *instance, bool value)
{
    if (Config.ExtraMenu.NoSmoke && value && instance && UnityObject_get_name) {
        String *name = UnityObject_get_name(instance);
        if (name && NameHasSmokeToken(name->CString()))
            value = false;
    }
    return orig_GameObjectSetActive(instance, value);
}

/*
typedef void (*SetUltraFrameRateDeviceInfo_t)(void*, bool, int, int, int, bool);
inline SetUltraFrameRateDeviceInfo_t orig_SetUltraFrameRateDeviceInfo = nullptr;

 void hooked_SetUltraFrameRateDeviceInfo(void* thiz, bool enableUltraFrameRate, int ultraFrameRate, int ultraFrameRateBR, int ultraFrameRateQualityLimit, bool customizedFrameRate) {
    if (thiz != NULL) {
        enableUltraFrameRate = true;
        ultraFrameRate = 240;
        ultraFrameRateBR = 240;
        ultraFrameRateQualityLimit = 240;
        customizedFrameRate = true;
    }
    orig_SetUltraFrameRateDeviceInfo(thiz, enableUltraFrameRate, ultraFrameRate, ultraFrameRateBR, ultraFrameRateQualityLimit, customizedFrameRate);
} 
*/
   //-- Unlimited Fly
inline bool (*orig_CheckForwardValid)(void *) = nullptr;
inline bool hook_CheckForwardValid(void *instance)
{
    if (instance != nullptr && Config.ExtraMenu.UnlimitedFly)
        return true;

    return orig_CheckForwardValid != nullptr
        ? orig_CheckForwardValid(instance)
        : false;
}

using UpdateVelocityGravity_t = void (*)(void *, float, float);
inline UpdateVelocityGravity_t orig_UpdateVelocityGravity = nullptr;

inline void hook_UpdateVelocityGravity(void *instance, float deltaTime, float scale)
{
    if (instance != nullptr && Config.ExtraMenu.UnlimitedFly)
        return;

    if (orig_UpdateVelocityGravity != nullptr)
        orig_UpdateVelocityGravity(instance, deltaTime, scale);
}

using Move_t = void (*)(void *, Vector3, float);
inline Move_t orig_Move = nullptr;

inline void hook_Move(void *instance, Vector3 deltaMove, float deltaTime)
{
    if (instance != nullptr && Config.ExtraMenu.UnlimitedFly) {
        const float speed = Config.ExtraMenu.FlySpeed > 1.0f
            ? Config.ExtraMenu.FlySpeed
            : 1.0f;
        deltaMove.x *= speed;
        deltaMove.z *= speed;

        if (deltaMove.y < 0.0f)
            deltaMove.y = 0.0f;
    }

    if (orig_Move != nullptr)
        orig_Move(instance, deltaMove, deltaTime);
}

inline void InitializeAllHooks() {

            //--MP VIEW
    HOOK_LIB("libunity.so", "0XC34782C", hook_get_m_MaxPovmp, get_m_MaxPovmp);
    
   //-- Auto Jump Fire
   HOOK_LIB("libunity.so", "0x5439550", hook_Pawn_StartFire, orig_Pawn_StartFire);

    
   //-- Fast Armor
  // HOOK_LIB("libunity.so", "0x6D9BC4C", hook_BRUseItemArmorPlate_SetUseItemTime,orig_BRUseItemArmorPlate_SetUseItemTime);
    //HOOK_LIB("libunity.so", "0x6D9B670", hook_BRUseItemArmorPlate_Tick, orig_BRUseItemArmorPlate_Tick);

   //-- Unlimited Fly
    HOOK_LIB("libunity.so", "0x5AB9BA0", hook_CheckForwardValid, orig_CheckForwardValid);
   HOOK_LIB("libunity.so", "0x5AB6D70", hook_UpdateVelocityGravity, orig_UpdateVelocityGravity);
    HOOK_LIB("libunity.so", "0x5AB9EF4", hook_Move, orig_Move);

    //-- Smart Reload
    HOOK_LIB("libunity.so", "0x5439674", hook_Pawn_StopFire, orig_Pawn_StopFire);


    //-- Aim Assist
    HOOK_LIB("libunity.so", "0x66BD7F0", GetAssitAimSpeed, orig_GetAssitAimSpeed);

    //-- Anti Flashbang
    HOOK_LIB("libunity.so", "0x53BC2E8", hook_OnFlashBangExplode, orig_OnFlashBangExplode);
    
    //-- Firerate Speed
    HOOK_LIB("libunity.so", "0x54AC7A0", get_FireBoltTime, orig_get_FireBoltTime);
    HOOK_LIB("libunity.so", "0x548DF60", get_FireInterval, orig_get_FireInterval);
    HOOK_LIB("libunity.so", "0x54C9924", get_DelaySprintFire, orig_get_DelaySprintFire);

    //-- High Jump
    HOOK_LIB("libunity.so", "0x53F7E7C", hook_GetMaxJumpHeight, orig_GetMaxJumpHeight);

    //-- Increase Damage
    HOOK_LIB("libunity.so", "0xC2BE0C8", SingleLineCheckPhysics, orig_SingleLineCheckPhysics);

/*
    //-- Long Slide
    HOOK_LIB("libunity.so", "0x54F756C", h_get_SlideTackleAcclerationSpeed, o_get_SlideTackleAcclerationSpeed);
    HOOK_LIB("libunity.so", "0xB10DB7C", h_PawnGetMaxSpeed, o_PawnGetMaxSpeed);
    HOOK_LIB("libunity.so", "0x7C696E8", h_get_SlideTackleSpeed, o_get_SlideTackleSpeed);
    HOOK_LIB("libunity.so", "0x7C687C8", h_GetSuperSlideRate, o_GetSuperSlideRate);
    HOOK_LIB("libunity.so", "0x9AF7808", h_TickLocalPlayer, o_TickLocalPlayer);
*/
    //-- No Overheat
    // HOOK_LIB("libunity.so", "0xC2B7A30", get_AddHotTime, orig_get_AddHotTime);

    //-- No Parachute
    HOOK_LIB("libunity.so", "0x89A3BB0", OpenParachute, orig_OpenParachute);

    //-- Quick Reload
    HOOK_LIB("libunity.so", "0x54752F8", get_ChangeClipTime, orig_get_ChangeClipTime);

    //-- Quick Scope
    HOOK_LIB("libunity.so", "0x5474160", get_AimingTime, orig_get_AimingTime);

    //-- Quick Switch
    HOOK_LIB("libunity.so", "0x54760F0", get_EquipTime, orig_get_EquipTime);

    //-- Red Wallhack
    HOOK_LIB("libunity.so", "0xA0B6018", get_IsInEM3Eye, orig_IsInEM3Eye); 
    HOOK_LIB("libunity.so", "0xB1A2814", GetAccDistance, orig_GetAccDistance);

    //-- Skip Tutorial
    HOOK_LIB_NO_ORIG("libunity.so", "0xCCC3240", IsTutorialEnabled);
    
    //-- Sky Diving Speed
    HOOK_LIB("libunity.so", "0x5AB2784", get_AccelerationForwardSpeedUp, orig_get_AccelerationForwardSpeedUp);
    HOOK_LIB("libunity.so", "0x5AB27E8", get_MaxVelocityForwardSpeedUp, orig_get_MaxVelocityForwardSpeedUp);

    //-- Snowboard Boost
    HOOK_LIB("libunity.so", "0x53FE7FC", hooked_get_m_PhysSkisMaxSpeed, get_m_PhysSkisMaxSpeed);

    //-- Speed Hack
    HOOK_LIB("libunity.so", "0x53A5280", hooked_CalcFinalMoveScale, original_CalcFinalMoveScale);

    //-- Weapon Kinetic
    HOOK_LIB("libunity.so", "0x5385AC8", get_IsKineticArmor, orig_get_IsKineticArmor);

    //-- Zero Recoil
    HOOK_LIB("libunity.so", "0xD535A6C", GetScaleRecoil, orig_GetScaleRecoil);

    //-- Zero Spread
    HOOK_LIB("libunity.so", "0xD5339F8", MinInaccuracy, orig_MinInaccuracy);
    HOOK_LIB("libunity.so", "0xC2C6060", MaxInaccuracy, orig_MaxInaccuracy);
    HOOK_LIB("libunity.so", "0xD541D2C", DisperseBase, orig_DisperseBase);
    
    //-- Spectate
    HOOK_LIB("libunity.so", "0x53665EC", h_ProcessForDelay, o_ProcessForDelay);
    HOOK_LIB("libunity.so", "0x5366558", h_NeedDelayProcess, o_NeedDelayProcess);
    HOOK_LIB("libunity.so", "0x53665EC", h_ProcessForDelay, o_ProcessForDelay);  

/*
    //-- Unlimited Bullets
    HOOK_LIB("libunity.so", "0x6AE86B0", IsInfiniteBullet, orig_IsInfiniteBullet);
    HOOK_LIB("libunity.so", "0xADE82D8", IsNoCostAmmo, orig_IsNoCostAmmo);

	//-- Unli Ammo (Functions)
	HOOK_LIB("libunity.so", "0x5473F58", hk_AmmoCanFire, orig_AmmoCanFire);          // AmmoCanFire - RVA: 0x5473F58
    HOOK_LIB("libunity.so", "0x548FDD8", hk_HasAmmo, orig_HasAmmo);                  // HasAmmo - RVA: 0x548FDD8
    HOOK_LIB("libunity.so", "0xC2BAC9C", hk_HasAmmo_IgnoreInfinite, orig_HasAmmo_IgnoreInfinite); // HasAmmo_IgnoreInfinite - RVA: 0xC2BAC9C
    HOOK_LIB("libunity.so", "0xC2B9D40", hk_ServerStopFire, orig_ServerStopFire);    // ServerStopFire - RVA: 0xC2B9D40
    HOOK_LIB("libunity.so", "0x5474FB0", hk_get_ShotCost, orig_get_ShotCost);        // get_ShotCost - RVA: 0x5474FB0
    HOOK_LIB("libunity.so", "0x54952AC", hk_IsAmmoFree, orig_IsAmmoFree);            // IsAmmoFree - RVA: 0x54952AC
 
     //-- Long Execute
    HOOK_LIB("libunity.so", "0x6AA932C", CheckTargetIsValid_DyingInAzurGameMode_Hook, orig_CheckTargetIsValid_DyingInAzurGameMode);
    HOOK_LIB("libunity.so", "0x6AA93E0", CheckTargetIsValid_PhysState_Hook, orig_CheckTargetIsValid_PhysState);
    HOOK_LIB("libunity.so", "0x6AA9634", CheckTargetIsValid_ManualParameter_Hook, orig_CheckTargetIsValid_ManualParameter);


    //-- Optimize Fps Lock
    HOOK_LIB("libunity.so", "0xAFAC73C", GetFrameRateValue, orig_GetFrameRateValue);
    HOOK_LIB("libunity.so", "0xAFAC128", GetUltraFrameRateFinalFPS, orig_GetUltraFrameRateFinalFPS);
    HOOK_LIB("libunity.so", "0xAFAC1C4", hooked_Fpslevel, original_Fpslevel);
    HOOK_LIB("libunity.so", "0xAFAC1C4", hooked_Fpslevel1, original_Fpslevel1);
    HOOK_LIB("libunity.so", "0xAFB0AE4", hooked_Extremequality, original_Extremequality);
	
    HOOK_LIB("libunity.so", "0xAFAB394", hooked_SetUltraFrameRateDeviceInfo, orig_SetUltraFrameRateDeviceInfo);
*/
    //-- No smoke
    UnityObject_get_name = reinterpret_cast<String *(*)(void *)>(
          getAbsoluteAddress("libunity.so", 0x5302EA4));

    DobbyHook((void *)getAbsoluteAddress("libunity.so", 0x52E8EC0),
          (void *)&hook_GameObjectSetActive,
          (void **)&orig_GameObjectSetActive);
          
    //-- Throwable Alert      
    DobbyHook((void *)getAbsoluteAddress("libunity.so", ThrowAlertCfg::TickRva),
          (void *)&hook_WeaponProjectileTick,
          (void **)&orig_WeaponProjectileTick);
          
    //-- Clear Terrain
    DobbyHook((void *)getAbsoluteAddress("libunity.so", 0x7FDABE8),
          (void *)&hook_SetGrassShowState,
          (void **)&orig_SetGrassShowState);

    DobbyHook((void *)getAbsoluteAddress("libunity.so", 0x9999EBC),
          (void *)&hook_SetGrassLODBias,
          (void **)&orig_SetGrassLODBias);         
          
    DobbyHook((void*)getAbsoluteAddress("libunity.so", 0xAFAB394), (void*)hooked_SetUltraFrameRateDeviceInfo, (void**)&orig_SetUltraFrameRateDeviceInfo);

    ///Walk Under Water
    DobbyHook((void *) getAbsoluteAddress("libunity.so", 0x53A5510), (void *)  &Pawn_GetCurrentDistToWaterSurface, (void **) &oPawn_GetCurrentDistToWaterSurface); 
    DobbyHook((void *) getAbsoluteAddress("libunity.so", 0x5599604), (void *)  &PlayerPawn_IsUnderWaterSurface, (void **) &oPlayerPawn_IsUnderWaterSurface); 
    DobbyHook((void *) getAbsoluteAddress("libunity.so", 0x806B580), (void *)  &CheckInWaterComponent_get_CurrentWaterSurfaceHeight, (void **) &oCheckInWaterComponent_get_CurrentWaterSurfaceHeight); 

  ///Camera Pov
  DobbyHook((void *) getAbsoluteAddress("libunity.so", 0xC0B4584), (void *)  &InputSettingConfig_Instant_GetMainCameraFov_3p, (void **) &oInputSettingConfig_Instant_GetMainCameraFov_3p); 

    //-- Unlock Blueprints
    HOOK_LIB("libunity.so", "0x97CC1D0", IsUnlocked, orig_IsUnlocked);
    
    HOOK_LIB("libunity.so", "0x9829E28", IsWeaponAttachmentUnlock, orig_IsWeaponAttachmentUnlock);
}
