#pragma once
#include "FrameRate.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <limits>

namespace slide_fatigue {
inline constexpr std::uintptr_t limitRva=0x7C6976C,recoveryRva=0x7C69810;
inline constexpr std::size_t localOffset=0x38,lastLimitOffset=0x170;
inline constexpr float freshTime=std::numeric_limits<float>::lowest();
inline constexpr std::array<unsigned char,32> entryBytes{0xea,0x0f,0x1d,0xfc,0xe9,0xa3,0x00,0x6d,0xfe,0x0f,0x00,0xf9,0xf4,0x4f,0x02,0xa9,0x34,0xc2,0x03,0x90,0xf3,0x03,0x00,0xaa,0x88,0xfa,0x5d,0x39,0xe8,0x00,0x00,0x37};
inline constexpr std::array<unsigned char,16> recoveryBytes{0x61,0x72,0x41,0xbd,0x00,0x38,0x21,0x1e,0x61,0x46,0x40,0xbd,0x00,0x20,0x21,0x1e};
inline std::atomic<bool> enabled{false},available{false};
inline void Publish(bool value) {enabled.store(value);}
inline bool IsLocal(const void* instance) {
    return instance&&*(static_cast<const unsigned char*>(instance)+localOffset)==1;
}
template<class Original> inline void Invoke(void* instance,const void* info,bool active,Original original) {
    unsigned char* stamp=nullptr;std::uint32_t previous=0;
    if(active&&IsLocal(instance)) {
        stamp=static_cast<unsigned char*>(instance)+lastLimitOffset;
        std::memcpy(&previous,stamp,4);std::memcpy(stamp,&freshTime,4);
    }
    original(instance,info);
    if(stamp) {
        float value=0;std::memcpy(&value,stamp,4);
        if(value==freshTime) std::memcpy(stamp,&previous,4);
    }
}
inline bool Matches(const unsigned char* entry,std::size_t entrySize,const unsigned char* recovery,std::size_t recoverySize) {
    return entry&&recovery&&entrySize>=entryBytes.size()&&recoverySize>=recoveryBytes.size()&&
        std::memcmp(entry,entryBytes.data(),entryBytes.size())==0&&std::memcmp(recovery,recoveryBytes.data(),recoveryBytes.size())==0;
}
#ifdef __ANDROID__
inline void (*originalLimit)(void*,const void*)=nullptr;
inline void Limit(void* instance,const void* info) {
    if(originalLimit) Invoke(instance,info,available.load()&&enabled.load(),originalLimit);
}
template<class Hook> inline bool Install(std::uintptr_t base,Hook hook) {
    if(available.load()) return true;
    if(!base||!frame_rate::Executable(base+limitRva,entryBytes.size())||!frame_rate::Executable(base+recoveryRva,recoveryBytes.size())||
       !Matches(reinterpret_cast<const unsigned char*>(base+limitRva),entryBytes.size(),reinterpret_cast<const unsigned char*>(base+recoveryRva),recoveryBytes.size())) return false;
    hook(reinterpret_cast<void*>(base+limitRva),reinterpret_cast<void*>(Limit),reinterpret_cast<void**>(&originalLimit));
    available=originalLimit!=nullptr;return available.load();
}
#endif
}
