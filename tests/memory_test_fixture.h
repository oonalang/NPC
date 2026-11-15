#pragma once
struct MemoryTestConfig {
    struct {
        bool NoCrouch=false,NoSlideFatigue=false,NoSprintFire=false,Parachute=false,Diving=false,UnlimitedFly=true;
        bool Hit=false,Recoil=false,Spread=false,Shake=false,Rpd=false,Fire=false;
        float HitboxScale=1.f;
        bool SmartReload=false,Reload=false,Scope=false,Switch=false,AutoJumpFire=false,FastArmorPlate=false,Kinetic=false;
        bool Flash=false,NoSmoke=false,ClearTerrain=false,CameraPov=true,Blueprints=false;
        float FlySpeed=1.f,CameraPovSize=90.f;
    } ExtraMenu;
};
inline MemoryTestConfig Config;
inline float SlideRange=0.f,SnowBsize=0.f,speedHackMultiplier=1.f,jumpHeightMultiplier=1.f,MpBsize=0.f;
