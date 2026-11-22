#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <sys/stat.h>
struct Vector2 {float x,y;};
struct Vector3 {float x,y,z;};
#include "../src/main/jni/System/Core/SaveConfig.h"
float menu[4]{.6f,.4f,.9f,1.f};
bool isJumpAdjustmentEnabled=false,SnowB=false,isSpeedHackEnabled=false;
float jumpHeightMultiplier=1.f,SnowBsize=0.f,speedHackMultiplier=1.f;
int main(int argc,char** argv) {
    assert(argc==2);androidFilesDir=std::string(argv[1])+"/";
    const kill_cosmetics::Options selected{{600000001,610000001,680000021}};
    kill_cosmetics::configured=selected;Config.ESPMenu.Box=true;Config.ExtraMenu.HitboxScale=1.21f;
    assert(SaveConfiguration("cosmetics"));
    kill_cosmetics::configured={};assert(LoadConfiguration("cosmetics"));
    assert(kill_cosmetics::configured.ids==selected.ids && Config.ESPMenu.Box && Config.ExtraMenu.HitboxScale==1.21f);
    assert(kill_cosmetics::Requested(kill_cosmetics::Kind::Board)==610000001);
    nlohmann::json stored;
    {std::ifstream stream(androidFilesDir+"configs/cosmetics.json");stream>>stored;}
    stored.erase("KillCosmetics");
    auto write=[&]{std::ofstream stream(androidFilesDir+"configs/cosmetics.json");stream<<stored;};
    write();assert(LoadConfiguration("cosmetics") && kill_cosmetics::configured.ids==kill_cosmetics::Options{}.ids);
    assert(kill_cosmetics::Requested(kill_cosmetics::Kind::Board)==0 && Config.ESPMenu.Box && Config.ExtraMenu.HitboxScale==1.21f);
    stored["KillCosmetics"]={{"Kill Icon","600000001"},{"Kill Board",600000001},{"Kill Broadcast",UINT64_MAX}};
    write();assert(LoadConfiguration("cosmetics") && kill_cosmetics::configured.ids==kill_cosmetics::Options{}.ids);
    assert(kill_cosmetics::Requested(kill_cosmetics::Kind::Icon)==0);
    std::puts("Kill cosmetic configuration: actual SaveConfiguration/LoadConfiguration round trip, legacy clearing, invalid IDs and existing ESP/Hitbox values passed");
}
