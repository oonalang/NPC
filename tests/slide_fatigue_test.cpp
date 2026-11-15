#include "../src/main/jni/System/Hooks/SlideFatigue.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>

using Pawn=std::array<unsigned char,0x200>;
static void Float(Pawn& pawn,std::size_t offset,float value) {std::memcpy(pawn.data()+offset,&value,4);}
static float Float(const Pawn& pawn,std::size_t offset) {float value=0;std::memcpy(&value,pawn.data()+offset,4);return value;}
int main(int argc,char** argv) {
    using namespace slide_fatigue;
    Pawn pawn{};pawn.fill(0x5a);pawn[localOffset]=1;
    Float(pawn,lastLimitOffset,99.f);Float(pawn,0x44,2.f);Float(pawn,0x6c,.7f);Float(pawn,0x16c,10.f);
    const auto before=pawn;int calls=0;int metadata=0;
    const auto original=[&](void* instance,const void* info) {
        assert(instance==pawn.data()&&info==&metadata);++calls;
        const float since=100.f-Float(pawn,lastLimitOffset);
        Float(pawn,0x16c,since<Float(pawn,0x44)?Float(pawn,0x16c)*.8f:10.f);
        Float(pawn,lastLimitOffset,100.f);
    };
    Invoke(pawn.data(),&metadata,true,original);
    assert(calls==1&&Float(pawn,0x16c)==10.f&&Float(pawn,lastLimitOffset)==100.f);
    assert(Float(pawn,0x44)==2.f&&Float(pawn,0x6c)==.7f);
    assert(std::memcmp(pawn.data(),before.data(),lastLimitOffset)==0);
    assert(std::memcmp(pawn.data()+lastLimitOffset+4,before.data()+lastLimitOffset+4,pawn.size()-lastLimitOffset-4)==0);
    Invoke(pawn.data(),&metadata,false,original);assert(calls==2&&std::fabs(Float(pawn,0x16c)-8.f)<.001f);
    pawn=before;pawn[localOffset]=0;
    Invoke(pawn.data(),&metadata,true,original);assert(calls==3&&std::fabs(Float(pawn,0x16c)-8.f)<.001f);
    pawn=before;
    Invoke(pawn.data(),&metadata,true,[&](void*,const void*) {assert(Float(pawn,lastLimitOffset)==freshTime);++calls;});
    assert(pawn==before&&calls==4);
    Invoke(nullptr,&metadata,true,[&](void* instance,const void* info){assert(!instance&&info==&metadata);++calls;});assert(calls==5);
    for(int i=0;i<10;++i) {Invoke(pawn.data(),&metadata,true,original);assert(Float(pawn,0x16c)==10.f&&Float(pawn,0x6c)==.7f);}
    assert(Matches(entryBytes.data(),entryBytes.size(),recoveryBytes.data(),recoveryBytes.size()));
    auto bad=entryBytes;bad[8]^=1;assert(!Matches(bad.data(),bad.size(),recoveryBytes.data(),recoveryBytes.size()));
    auto badRecovery=recoveryBytes;badRecovery[4]^=1;assert(!Matches(entryBytes.data(),entryBytes.size(),badRecovery.data(),badRecovery.size()));
    assert(!Matches(entryBytes.data(),8,recoveryBytes.data(),recoveryBytes.size()));
    if(argc>1) {
        std::ifstream binary(argv[1],std::ios::binary);assert(binary);
        std::array<unsigned char,32> entry{};std::array<unsigned char,16> recovery{};
        binary.seekg(limitRva-0x4000);binary.read(reinterpret_cast<char*>(entry.data()),entry.size());assert(binary.gcount()==entry.size());
        binary.seekg(recoveryRva-0x4000);binary.read(reinterpret_cast<char*>(recovery.data()),recovery.size());assert(binary.gcount()==recovery.size());
        assert(Matches(entry.data(),entry.size(),recovery.data(),recovery.size()));
    }
    std::puts("Local-only fatigue reset, repeated slides, disable passthrough, unchanged cooldown/config fields, no-update restoration, metadata forwarding and 1.6.59 code anchors passed");
}
