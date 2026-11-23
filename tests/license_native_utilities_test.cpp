#define __ANDROID__
#include "license_gate_stub.h"
#include "../src/main/jni/System/Hooks/FrameRate.h"
#include <cassert>
#include <cstdio>
int Supported(void*,int level,const void*) {return level;}
int main() {
    alignas(8) std::array<unsigned char,0x200> object{};
    const int original=60;std::memcpy(object.data()+0x44,&original,4);std::memcpy(object.data()+0x48,&original,4);
    const auto before=object;
    frame_rate::available=true;frame_rate::originalSupported=Supported;
    assert(frame_rate::Read().unlock);
    license_test::allowed=false;
    assert(frame_rate::Supported(object.data(),4,nullptr)==4&&object==before);
    license_test::allowed=true;
    assert(frame_rate::Supported(object.data(),4,nullptr)==9);
    assert(object[0x40]==1);
    license_test::allowed=false;
    assert(frame_rate::Supported(object.data(),4,nullptr)==4&&object==before);
    std::puts("Native license gates restore FPS device fields on revocation");
}
