#include <cassert>

enum EspBoxType { Fill = 0, Outline = 1, Corner = 2, ThreeD = 3 };
enum LineTarget { Top = 0, Center = 1, Bottom = 2 };
enum EspHealthPosition { HealthTop = 0, HealthSide = 1 };

struct Configuration {
    struct Esp {
        bool isPlayerLine = false;
        bool Box = false;
        bool Name = false;
        bool Health = false;
        bool Armor = false;
        bool Distance = false;
        bool Count = false;
        bool Skeleton = false;
        bool ItemESP = false;
        bool VisibilityCheck = false;
        bool GrenadeWarn = false;
        EspBoxType BoxType = Outline;
        LineTarget Target = Center;
        EspHealthPosition HealthPosition = HealthSide;
    } ESPMenu;
    struct AimConfig {
        bool Aimbot360 = false;
        bool AimSilent = false;
    } Aim;
    struct Extra {
        bool FastPickup = false;
        bool ExtendedPickupRange = false;
        bool WallHack = false;
        float PickupRange = 6.0f;
    } ExtraMenu;
} Config;

#include "../src/main/jni/System/Core/LiteProfile.h"

int main() {
    Config.ESPMenu.Box = true;
    Config.ESPMenu.Name = true;
    Config.ESPMenu.Health = true;
    Config.ESPMenu.Armor = true;
    Config.ESPMenu.Distance = true;
    Config.ESPMenu.Count = true;
    Config.ESPMenu.isPlayerLine = true;
    Config.ESPMenu.BoxType = ThreeD;
    Config.ESPMenu.HealthPosition = HealthSide;
    Config.ESPMenu.Skeleton = true;
    Config.ESPMenu.ItemESP = true;
    Config.ESPMenu.GrenadeWarn = true;
    Config.ESPMenu.VisibilityCheck = true;
    Config.Aim.Aimbot360 = true;
    Config.Aim.AimSilent = true;
    Config.ExtraMenu.FastPickup = true;
    Config.ExtraMenu.ExtendedPickupRange = true;
    Config.ExtraMenu.WallHack = true;

    shai_lite::Apply();
    assert(Config.ESPMenu.Box);
    assert(Config.ESPMenu.Name);
    assert(Config.ESPMenu.Health);
    assert(Config.ESPMenu.Armor);
    assert(Config.ESPMenu.Distance);
    assert(Config.ESPMenu.Count);
    assert(Config.ESPMenu.isPlayerLine);
    assert(Config.ESPMenu.BoxType == Outline);
    assert(Config.ESPMenu.HealthPosition == HealthSide);
    assert(!Config.ESPMenu.Skeleton);
    assert(!Config.ESPMenu.ItemESP);
    assert(!Config.ESPMenu.GrenadeWarn);
    assert(!Config.ESPMenu.VisibilityCheck);
    assert(!Config.Aim.Aimbot360);
    assert(!Config.Aim.AimSilent);
    assert(!Config.ExtraMenu.FastPickup);
    assert(!Config.ExtraMenu.ExtendedPickupRange);
    assert(!Config.ExtraMenu.WallHack);

    Config.ESPMenu.Box = false;
    shai_lite::Apply();
    assert(!Config.ESPMenu.Box);
    assert(Config.ESPMenu.Name);
    return 0;
}
