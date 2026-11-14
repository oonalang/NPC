#include <array>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdint>
#include <cassert>
#include <cstring>
#include <cstdio>
using uint=unsigned int;
using string=std::string;
struct String;
template<class T> struct Array;
#include "../src/main/jni/Skin/Fields.h"
int main() {
    struct LiveConfig {std::array<unsigned char,0x20> header{};WeaponConfFields fields{};} config;
    static_assert(offsetof(LiveConfig,fields)==0x20);
    auto* bytes=reinterpret_cast<unsigned char*>(&config);
    void* camo=reinterpret_cast<void*>(0x1110);
    void* desc=reinterpret_cast<void*>(0x2220);
    void* name=reinterpret_cast<void*>(0x3330);
    std::memcpy(bytes+0x58,&camo,8);
    std::memcpy(bytes+0x60,&desc,8);
    auto* fields=&config.fields;
    fields->LOCID_Name=name;
    void* stored=nullptr;std::memcpy(&stored,bytes+0x68,8);
    assert(stored==name && fields->EnableCamoBeta==camo && fields->LOCID_Desc==desc);
    std::puts("1.6.59 weapon name writes at 0x68 preserve the camo and description fields");
}
