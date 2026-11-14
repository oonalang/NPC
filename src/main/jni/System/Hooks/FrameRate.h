#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <mutex>

namespace frame_rate {
struct Options {
    bool unlock=false,custom=false,ultra=false;
    int regular=60,high=240;
    bool Active() const {return unlock||custom||ultra;}
    int ForLevel(int original,int level) const {return original>0?(ultra&&level>=5?high:custom?regular:original):original;}
};
inline int Fps(float value,int fallback,int minimum=30) {
    return std::isfinite(value)&&value>=minimum?static_cast<int>(std::clamp(std::round(value),float(minimum),360.f)):fallback;
}
inline Options Normalize(bool unlock,bool custom,float regular,bool ultra,float high) {
    return {unlock,custom,ultra,Fps(regular,60),Fps(high,240,60)};
}
inline std::atomic<std::uint32_t> settings{0};
inline void Publish(Options value) {
    value=Normalize(value.unlock,value.custom,value.regular,value.ultra,value.high);
    settings.store((value.unlock?1u:0u)|(value.custom?2u:0u)|(value.ultra?4u:0u)|(std::uint32_t(value.regular)<<3)|(std::uint32_t(value.high)<<12));
}
inline Options Read() {
    const auto value=settings.load();
    return Normalize(value&1,value&2,(value>>3)&511,value&4,(value>>12)&511);
}
class DeviceState {
    void* instance_=nullptr;
    std::array<unsigned char,20> original_{};
    bool changed_=false;
public:
    void Reset(void* instance) {if(instance==instance_) {instance_=nullptr;changed_=false;}}
    void Apply(void* instance,const Options& options) {
        if(!instance) return;
        auto* data=static_cast<unsigned char*>(instance)+0x40;
        if(instance_!=instance) {instance_=instance;std::memcpy(original_.data(),data,original_.size());changed_=false;}
        if(options.Active()) {
            int mp=0,br=0;std::memcpy(&mp,original_.data()+4,4);std::memcpy(&br,original_.data()+8,4);
            const int limit=options.ultra?options.high:options.custom?options.regular:240;
            mp=std::max(mp,limit);br=std::max(br,limit);
            data[0]=data[12]=1;std::memcpy(data+4,&mp,4);std::memcpy(data+8,&br,4);changed_=true;
        } else if(changed_) {std::memcpy(data,original_.data(),original_.size());changed_=false;}
    }
};
struct Target {std::uintptr_t rva;const char* member;std::array<unsigned char,16> bytes;};
inline constexpr std::array<Target,7> profile{{
    {0xAFAAEF8,"GetMaxSupportedFrameRateLevel(int)",{0xfe,0x0f,0x1e,0xf8,0xf4,0x4f,0x01,0xa9,0xf4,0x03,0x00,0xaa,0xe0,0x3b,0x84,0x52}},
    {0xAFAC1C4,"GetMaxSupportedFrameRateLevelForDevice()",{0xfe,0x4f,0xbf,0xa9,0xf3,0x03,0x00,0xaa,0x00,0x3c,0x84,0x52,0x20,0x00,0xa0,0x72}},
    {0xAFAC128,"GetMaxFrameRateLevel(int)",{0xfe,0x0f,0x1e,0xf8,0xf4,0x4f,0x01,0xa9,0xf3,0x03,0x00,0xaa,0xe0,0x3d,0x84,0x52}},
    {0xAFAC238,"GetUltraFrameValue(int)",{0xfe,0x57,0xbe,0xa9,0xf4,0x4f,0x01,0xa9,0xd5,0x28,0x02,0xf0,0xf3,0x03,0x01,0x2a}},
    {0xAFAC73C,"GetFrameRateValue(int,int)",{0xff,0x03,0x01,0xd1,0xfe,0x0b,0x00,0xf9,0xf6,0x57,0x02,0xa9,0xf4,0x4f,0x03,0xa9}},
    {0xAFAD000,"SetFrameRate(int,bool)",{0x76,0xf5,0x17,0x8f,0xb9,0x1b,0x59,0xe0,0x31,0xd6,0x18,0xe9,0x11,0x58,0x5b,0xe0}},
    {0xAFAB394,"SetUltraFrameRateDeviceInfo(bool,int,int,int,bool)",{0xff,0x43,0x02,0xd1,0xfd,0x7b,0x03,0xa9,0xfc,0x6f,0x04,0xa9,0xfa,0x67,0x05,0xa9}},
}};
inline bool Matches(const Target& target,const unsigned char* data,std::size_t size) {
    return data&&size>=target.bytes.size()&&std::memcmp(data,target.bytes.data(),target.bytes.size())==0;
}
#ifdef __ANDROID__
}
#include <link.h>
namespace frame_rate {
inline std::atomic<bool> available{false};
inline DeviceState device;
inline std::mutex deviceMutex;
inline thread_local bool configuring=false;
inline Options Current() {return available.load()?Read():Options{};}
inline void Apply(void* instance) {if(configuring||!available.load()) return;std::lock_guard<std::mutex> lock(deviceMutex);device.Apply(instance,Read());}
inline int (*originalSupported)(void*,int,const void*)=nullptr;
inline int (*originalDevice)(void*,const void*)=nullptr;
inline int (*originalMaximum)(void*,int,const void*)=nullptr;
inline int (*originalUltra)(void*,int,const void*)=nullptr;
inline int (*originalValue)(void*,int,int,const void*)=nullptr;
inline void (*originalSet)(void*,int,bool,const void*)=nullptr;
inline void (*originalInfo)(void*,bool,int,int,int,bool,const void*)=nullptr;
inline int Supported(void* instance,int level,const void* info) {Apply(instance);return instance&&Current().Active()?9:originalSupported?originalSupported(instance,level,info):level;}
inline int Device(void* instance,const void* info) {Apply(instance);return instance&&Current().Active()?9:originalDevice?originalDevice(instance,info):0;}
inline int Maximum(void* instance,int level,const void* info) {Apply(instance);return instance&&Current().Active()?9:originalMaximum?originalMaximum(instance,level,info):level;}
inline int Ultra(void* instance,int level,const void* info) {Apply(instance);return Current().ForLevel(originalUltra?originalUltra(instance,level,info):0,level);}
inline int Value(void* instance,int level,int mode,const void* info) {Apply(instance);return Current().ForLevel(originalValue?originalValue(instance,level,mode,info):0,level);}
inline void Set(void* instance,int target,bool mode,const void* info) {
    Apply(instance);int level=-1;if(instance) std::memcpy(&level,static_cast<unsigned char*>(instance)+0x3c,4);
    if(originalSet) originalSet(instance,instance?Current().ForLevel(target,level):target,mode,info);
}
inline void Info(void* instance,bool enabled,int mp,int br,int quality,bool customized,const void* info) {
    const bool nested=configuring;configuring=true;
    if(originalInfo) originalInfo(instance,enabled,mp,br,quality,customized,info);
    configuring=nested;
    if(!nested&&available.load()) {std::lock_guard<std::mutex> lock(deviceMutex);device.Reset(instance);device.Apply(instance,Read());}
}
inline bool Executable(std::uintptr_t address,std::size_t size) {
    struct Query {std::uintptr_t address;std::size_t size;bool found=false;} query{address,size};
    dl_iterate_phdr([](dl_phdr_info* info,std::size_t,void* context) {
        auto& q=*static_cast<Query*>(context);
        if(!info->dlpi_name||!std::strstr(info->dlpi_name,"libunity.so")) return 0;
        for(unsigned i=0;i<info->dlpi_phnum;++i) {
            const auto& segment=info->dlpi_phdr[i];const auto lo=info->dlpi_addr+segment.p_vaddr;
            if(segment.p_type==PT_LOAD&&(segment.p_flags&(PF_R|PF_X))==(PF_R|PF_X)&&q.address>=lo&&q.address-lo<=segment.p_memsz&&q.size<=segment.p_memsz-(q.address-lo)) {q.found=true;return 1;}
        }
        return 0;
    },&query);
    return query.found;
}
template<class Hook> inline bool Install(std::uintptr_t base,Hook hook) {
    if(available) return true;
    if(!base) return false;
    for(const auto& target:profile) if(!Executable(base+target.rva,target.bytes.size())||!Matches(target,reinterpret_cast<const unsigned char*>(base+target.rva),target.bytes.size())) return false;
    void* replacements[]{reinterpret_cast<void*>(Supported),reinterpret_cast<void*>(Device),reinterpret_cast<void*>(Maximum),reinterpret_cast<void*>(Ultra),reinterpret_cast<void*>(Value),reinterpret_cast<void*>(Set),reinterpret_cast<void*>(Info)};
    void** originals[]{reinterpret_cast<void**>(&originalSupported),reinterpret_cast<void**>(&originalDevice),reinterpret_cast<void**>(&originalMaximum),reinterpret_cast<void**>(&originalUltra),reinterpret_cast<void**>(&originalValue),reinterpret_cast<void**>(&originalSet),reinterpret_cast<void**>(&originalInfo)};
    for(unsigned i=0;i<profile.size();++i) {hook(reinterpret_cast<void*>(base+profile[i].rva),replacements[i],originals[i]);if(!*originals[i]) return false;}
    available=true;return true;
}
#endif
}
