#include "../src/main/jni/System/Hooks/FrameRate.h"
#include <cassert>
#include <fstream>
#include <limits>
#include <cstdio>
int main(int argc,char** argv) {
    using namespace frame_rate;
    const auto defaults=Read();assert(defaults.unlock&&!defaults.custom&&!defaults.ultra&&defaults.regular==60&&defaults.high==240);
    assert(Configured(false,60,false,240).unlock&&Configured(true,144,true,240).unlock);
    auto value=Normalize(false,false,0,false,std::numeric_limits<float>::quiet_NaN());
    assert(!value.Active()&&value.regular==60&&value.high==240&&value.ForLevel(30,5)==30);
    value=Normalize(true,false,60,false,240);assert(value.Active()&&value.ForLevel(30,5)==30);
    value=Normalize(false,true,144.8f,false,0);assert(value.regular==145&&value.ForLevel(30,5)==145&&value.ForLevel(-1,5)==-1&&value.ForLevel(0,5)==0);
    value=Normalize(false,true,5000,true,5000);assert(value.regular==360&&value.high==360&&value.ForLevel(60,5)==360);
    value=Normalize(false,false,60,true,240);assert(value.ForLevel(60,4)==60&&value.ForLevel(90,5)==240&&value.ForLevel(-1,5)==-1);
    value.custom=true;value.regular=90;assert(value.ForLevel(30,0)==90&&value.ForLevel(120,6)==240);
    for(auto regular:{30,60,90,120,144,165,185,240,360}) for(auto high:{60,120,240,360}) {
        Publish({true,true,true,regular,high});const auto copy=Read();assert(copy.unlock&&copy.custom&&copy.ultra&&copy.regular==regular&&copy.high==high);
    }
    alignas(8) std::array<unsigned char,128> instance{};
    instance.fill(0x5a);instance[0x40]=0;instance[0x4c]=0;
    int originalRate=60,originalBR=40,quality=2;
    std::memcpy(instance.data()+0x44,&originalRate,4);std::memcpy(instance.data()+0x48,&originalBR,4);std::memcpy(instance.data()+0x50,&quality,4);
    const auto original=instance;DeviceState state;
    state.Apply(instance.data(),{});assert(instance==original);
    state.Apply(instance.data(),{true,false,false,60,240});
    int rate=0,br=0;std::memcpy(&rate,instance.data()+0x44,4);std::memcpy(&br,instance.data()+0x48,4);
    assert(instance[0x40]==1&&instance[0x4c]==1&&rate==240&&br==240);
    assert(std::memcmp(instance.data(),original.data(),0x40)==0&&std::memcmp(instance.data()+0x50,original.data()+0x50,instance.size()-0x50)==0);
    state.Apply(instance.data(),{false,false,true,60,120});std::memcpy(&rate,instance.data()+0x44,4);assert(rate==120);
    state.Apply(instance.data(),{});assert(instance==original);state.Apply(nullptr,{});
    auto changed=profile[0].bytes;changed[4]^=1;assert(!Matches(profile[0],changed.data(),changed.size()));
    assert(!Matches(profile[0],profile[0].bytes.data(),8));
    if(argc>1) {
        std::ifstream input(argv[1],std::ios::binary);assert(input);
        for(const auto& target:profile) {std::array<unsigned char,16> bytes{};input.seekg(target.rva-0x4000);input.read(reinterpret_cast<char*>(bytes.data()),bytes.size());assert(Matches(target,bytes.data(),input.gcount()));}
    }
    std::puts("FPS ranges, override priority, disabled/sentinel passthrough, atomic settings, device-field restoration and six 1.6.59 method fingerprints passed");
}
