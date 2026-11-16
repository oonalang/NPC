#define __ANDROID__
#include "license_gate_stub.h"
#include "../src/main/jni/System/Hooks/FrameRate.h"
#include "../src/main/jni/System/Hooks/SlideFatigue.h"
#include <cassert>
#include <cstdio>
int Supported(void*,int level,const void*) {return level;}
float observed=0;
void Limit(void* object,const void*) {std::memcpy(&observed,static_cast<unsigned char*>(object)+slide_fatigue::lastLimitOffset,4);}
int main() {
    alignas(8) std::array<unsigned char,0x200> object{};
    const int original=60;std::memcpy(object.data()+0x44,&original,4);std::memcpy(object.data()+0x48,&original,4);
    const auto before=object;
    frame_rate::available=true;frame_rate::originalSupported=Supported;
    frame_rate::Publish({true,false,false,60,240});license_test::allowed=true;
    assert(frame_rate::Supported(object.data(),4,nullptr)==9);
    assert(object[0x40]==1);
    license_test::allowed=false;
    assert(frame_rate::Supported(object.data(),4,nullptr)==4&&object==before);
    slide_fatigue::available=true;slide_fatigue::Publish(true);slide_fatigue::originalLimit=Limit;
    object[slide_fatigue::localOffset]=1;
    const float stamp=5.f;std::memcpy(object.data()+slide_fatigue::lastLimitOffset,&stamp,4);
    slide_fatigue::Limit(object.data(),nullptr);assert(observed==stamp);
    license_test::allowed=true;slide_fatigue::Limit(object.data(),nullptr);assert(observed==slide_fatigue::freshTime);
    license_test::allowed=false;slide_fatigue::Limit(object.data(),nullptr);assert(observed==stamp);
    std::puts("Native license gates restore FPS device fields and stop slide overrides on revocation");
}
