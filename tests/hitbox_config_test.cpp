#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <sys/stat.h>
struct Vector2 { float x,y; };
struct Vector3 { float x,y,z; };
#include "../src/main/jni/System/Core/SaveConfig.h"
float menu[4]{.6f,.4f,.9f,1.f};
bool isJumpAdjustmentEnabled=false,SnowB=false,isSpeedHackEnabled=false;
float jumpHeightMultiplier=1.f,SnowBsize=0.f,speedHackMultiplier=1.f;

int main(int argc,char** argv) {
    assert(argc==2);androidFilesDir=std::string(argv[1])+"/";
    Config.ExtraMenu.Hit=true;Config.ExtraMenu.HitboxScale=2.75f;Config.ExtraMenu.Kinetic=true;
    Config.ExtraMenu.ProtectCodmLogin=true;
    SaveConfiguration("hitbox");
    Config.ExtraMenu.Hit=false;Config.ExtraMenu.HitboxScale=1.f;Config.ExtraMenu.Kinetic=false;
    Config.ExtraMenu.ProtectCodmLogin=false;
    assert(LoadConfiguration("hitbox"));
    assert(Config.ExtraMenu.Hit&&Config.ExtraMenu.HitboxScale==2.75f&&Config.ExtraMenu.Kinetic);
    assert(Config.ExtraMenu.ProtectCodmLogin&&session_protection::Read().enabled);
    assert(hitbox::Read().enabled&&std::fabs(hitbox::Read().scale-2.75f)<.0001f);
    nlohmann::json stored;
    {std::ifstream file(androidFilesDir+"configs/hitbox.json");file>>stored;}
    auto write=[&] {std::ofstream file(androidFilesDir+"configs/old.json");file<<stored;};
    stored["ExtraMenu"].erase("HitboxScale");stored["ExtraMenu"].erase("ProtectCodmLogin");write();
    Config.ExtraMenu.HitboxScale=4.f;assert(LoadConfiguration("old"));
    assert(Config.ExtraMenu.HitboxScale==1.f&&Config.ExtraMenu.Hit);
    assert(!Config.ExtraMenu.ProtectCodmLogin&&!session_protection::Read().enabled);
    stored["ExtraMenu"]["HitboxScale"]=99.f;stored["ExtraMenu"]["Kinetic"]=false;write();
    assert(LoadConfiguration("old")&&Config.ExtraMenu.HitboxScale==5.f&&!Config.ExtraMenu.Kinetic);
    stored["ExtraMenu"]["HitboxScale"]=-2.f;write();
    assert(LoadConfiguration("old")&&Config.ExtraMenu.HitboxScale==1.f);
    Config.ExtraMenu.HitboxScale=INFINITY;SaveConfiguration("invalid");
    assert(LoadConfiguration("invalid")&&Config.ExtraMenu.HitboxScale==1.f);
    std::puts("Hitbox config: JSON round trip, legacy defaults, bounds, nonfinite values and Kinetic key compatibility passed");
}
