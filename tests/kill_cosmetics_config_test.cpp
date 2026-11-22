#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <sys/stat.h>
struct Vector2 {float x,y;};struct Vector3 {float x,y,z;};
#include "../src/main/jni/System/Core/SaveConfig.h"
float menu[4]{.6f,.4f,.9f,1.f};
bool isJumpAdjustmentEnabled=false,SnowB=false,isSpeedHackEnabled=false;
float jumpHeightMultiplier=1.f,SnowBsize=0.f,speedHackMultiplier=1.f;
int main(int argc,char** argv){
 assert(argc==2);androidFilesDir=std::string(argv[1])+"/";
 kill_cosmetics::configured.automatic=false;Config.ESPMenu.Box=true;Config.ExtraMenu.HitboxScale=1.21f;
 assert(SaveConfiguration("cosmetics"));kill_cosmetics::configured={};assert(LoadConfiguration("cosmetics"));
 assert(!kill_cosmetics::configured.automatic&&!kill_cosmetics::requested.load());assert(Config.ESPMenu.Box&&Config.ExtraMenu.HitboxScale==1.21f);
 nlohmann::json stored;{std::ifstream f(androidFilesDir+"configs/cosmetics.json");f>>stored;}
 assert(stored["KillCosmetics"].size()==1&&stored["KillCosmetics"].contains("Automatic"));
 auto write=[&]{std::ofstream f(androidFilesDir+"configs/cosmetics.json");f<<stored;};
 stored["KillCosmetics"]={{"Kill Icon",600000001},{"Kill Board",610000001},{"Kill Broadcast",680000021}};
 write();assert(LoadConfiguration("cosmetics")&&kill_cosmetics::configured.automatic);
 stored["KillCosmetics"]["Automatic"]="false";write();assert(LoadConfiguration("cosmetics")&&kill_cosmetics::requested.load());
 stored.erase("KillCosmetics");write();assert(LoadConfiguration("cosmetics")&&kill_cosmetics::configured.automatic&&Config.ESPMenu.Box);
 std::puts("Automatic cosmetics config: saved opt-out, default-on migration, old manual IDs ignored, malformed/default handling and other configuration values preserved");
}
