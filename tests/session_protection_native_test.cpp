#include "../src/main/jni/System/Hooks/SessionProtection.h"
#include "../src/main/jni/System/Hooks/SessionProtectionProfile.h"
#include "../src/main/jni/Trigger/NativeDriver.h"
#include "../src/main/jni/Trigger/TriggerCompatibility.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <link.h>
#include <string>
#include <sys/mman.h>
#include <vector>

using namespace session_protection;
namespace {
using Meta=const void*;
using SendFn=bool(*)(void*,void*,std::uint32_t,std::uint64_t,Meta);
using ForbidFn=void(*)(void*,bool,Meta);
using TickFn=void(*)(void*,float,Meta);
using EndFn=void(*)(void*,Meta);
using DisconnectedFn=bool(*)(void*,Meta);
SendFn send=nullptr;ForbidFn forbid=nullptr;TickFn tick=nullptr;
EndFn disconnect=nullptr,reset=nullptr,shutdown=nullptr;DisconnectedFn disconnected=nullptr;
dl_phdr_info loaded{};bool loaderReady=false,queued=true,mismatch=false,throwRequest=false;
int hooks=0,failHook=0,ticks=0,sends=0,ends=0;bool notifyResult=true;
std::uintptr_t expectedRequestClass=0;
std::vector<bool> requested;
struct Range {std::uintptr_t begin;std::size_t size;};std::vector<Range> readable;
template<class Buffer,class Value> void Put(Buffer& buffer,std::size_t offset,Value value) {
    assert(offset+sizeof(value)<=buffer.size());std::memcpy(buffer.data()+offset,&value,sizeof(value));
}
struct Module {
    static constexpr std::size_t size=0xea0b000;
    unsigned char* bytes=static_cast<unsigned char*>(mmap(nullptr,size,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));
    std::array<ElfW(Phdr),3> segments{};
    alignas(8) std::array<unsigned char,0x40> requestClass{};
    std::uintptr_t requestVariable=0;
    Module() {
        assert(bytes!=MAP_FAILED);
        segments[0].p_type=PT_LOAD;segments[0].p_flags=PF_R;
        segments[0].p_memsz=segments[0].p_filesz=0x1000;
        segments[1].p_type=PT_LOAD;segments[1].p_flags=PF_R|PF_X;segments[1].p_vaddr=0x1000;
        segments[1].p_memsz=segments[1].p_filesz=size-0x1000;
        segments[2].p_type=PT_NOTE;segments[2].p_vaddr=0x100;
        segments[2].p_memsz=segments[2].p_filesz=24;
        const unsigned char note[]{4,0,0,0,8,0,0,0,3,0,0,0,'G','N','U',0,0x05,0x6d,0x63,0x90,0xcc,0x71,0x28,0x65};
        std::memcpy(bytes+0x100,note,sizeof(note));
        for(const auto& code:profile) std::memcpy(bytes+code.rva,code.expected.data(),code.expected.size());
        requestVariable=reinterpret_cast<std::uintptr_t>(requestClass.data());expectedRequestClass=requestVariable;
        const auto slot=reinterpret_cast<std::uintptr_t>(&requestVariable);std::memcpy(bytes+requestTypeSlot,&slot,8);
        loaded.dlpi_addr=base();loaded.dlpi_name="/game/lib/arm64/libunity.so";
        loaded.dlpi_phdr=segments.data();loaded.dlpi_phnum=segments.size();
        readable.push_back({base(),size});readable.push_back({reinterpret_cast<std::uintptr_t>(this),sizeof(*this)});
    }
    ~Module(){munmap(bytes,size);}
    std::uintptr_t base() const{return reinterpret_cast<std::uintptr_t>(bytes);}
};
bool SendOriginal(void* sender,void* message,std::uint32_t command,std::uint64_t route,Meta info) {
    assert(sender&&message);++sends;
    if(command==42) {assert(route==123&&info==reinterpret_cast<void*>(321));return false;}
    assert(command==0&&route==0);return queued;
}
void ForbidOriginal(void*,bool value,Meta info) {
    requested.push_back(value);
    if(throwRequest) throw 7;
    alignas(8) std::array<unsigned char,0x20> request{};
    Put(request,0,mismatch?std::uintptr_t{8}:expectedRequestClass);Put(request,0x18,int(value));
    send(reinterpret_cast<void*>(64),request.data(),0,0,info);
}
void TickOriginal(void* handler,float deltaTime,Meta info) {
    assert(handler&&deltaTime==.016f&&info==reinterpret_cast<void*>(123));++ticks;
}
void EndOriginal(void* handler,Meta info) {assert(handler&&info==reinterpret_cast<void*>(123));++ends;}
bool NotifyOriginal(void* handler,Meta info) {EndOriginal(handler,info);return notifyResult;}
int Hook(void* address,void* replacement,void** original) {
    ++hooks;if(hooks==failHook) return -1;
    const auto rva=reinterpret_cast<std::uintptr_t>(address)-loaded.dlpi_addr;
    switch(rva) {
        case 0x5B9ADF0:send=reinterpret_cast<SendFn>(replacement);*original=reinterpret_cast<void*>(SendOriginal);break;
        case 0xC977A24:forbid=reinterpret_cast<ForbidFn>(replacement);*original=reinterpret_cast<void*>(ForbidOriginal);break;
        case 0xC448DD8:disconnect=reinterpret_cast<EndFn>(replacement);*original=reinterpret_cast<void*>(EndOriginal);break;
        case 0xC44AA14:disconnected=reinterpret_cast<DisconnectedFn>(replacement);*original=reinterpret_cast<void*>(NotifyOriginal);break;
        case 0xC44A988:reset=reinterpret_cast<EndFn>(replacement);*original=reinterpret_cast<void*>(EndOriginal);break;
        case 0xC44A748:shutdown=reinterpret_cast<EndFn>(replacement);*original=reinterpret_cast<void*>(EndOriginal);break;
        case 0xC444A08:tick=reinterpret_cast<TickFn>(replacement);*original=reinterpret_cast<void*>(TickOriginal);break;
        default:assert(false);
    }
    return 0;
}
}
extern "C" int dl_iterate_phdr(int(*callback)(dl_phdr_info*,std::size_t,void*),void* context) {
    return loaderReady?callback(&loaded,sizeof(loaded),context):0;
}
namespace PhantomTrigger {
bool InstallNativeDriver(std::uintptr_t) noexcept {return false;}
void NativeDriverFailure() noexcept {}
void NativeDriverPending() noexcept {}
bool CompatibilityDetail::ReadSelf(std::uintptr_t address,void* output,std::size_t size) noexcept {
    for(const auto& range:readable) if(address>=range.begin&&address-range.begin<=range.size&&size<=range.size-(address-range.begin)) {
        std::memcpy(output,reinterpret_cast<const void*>(address),size);return true;
    }
    return false;
}
}
int main(int argc,char** argv) {
    const std::string mode=argc==2?argv[1]:"normal";
    Module module;
    if(mode=="--build"||mode=="--prefix"||mode=="--partial") {
        loaderReady=true;
        if(mode=="--build") module.bytes[0x110]^=1;
        if(mode=="--prefix") module.bytes[profile[0].rva]^=1;
        if(mode=="--partial") failHook=2;
        assert(!Install(module.base(),Hook)&&!available);
        assert(hooks==(mode=="--partial"?2:0));
        const int previous=hooks;assert(!Install(module.base(),Hook)&&hooks==previous);
        if(mode=="--partial") {int object=0;assert(!send(&object,&object,42,123,reinterpret_cast<void*>(321)));}
        std::printf("Session native %s passed\n",mode.c_str());return 0;
    }
    assert(!Install(module.base(),Hook)&&backend==Backend::Waiting&&hooks==0);
    loaderReady=true;assert(Install(module.base(),Hook)&&available&&hooks==7);
    assert(Install(module.base(),Hook)&&hooks==7);
    alignas(8) std::array<unsigned char,0x50> zone{};Put(zone,0x34,std::uint8_t{1});Put(zone,0x38,std::uintptr_t{456});
    const auto step=[&]{tick(zone.data(),.016f,reinterpret_cast<void*>(123));};
    step();assert(requested.empty());
    if(mode=="--stock-on") {forbid(nullptr,true,nullptr);assert(requested.back());requested.clear();}
    Publish(true);
    if(mode=="--mismatch") mismatch=true;
    if(mode=="--throw") throwRequest=true;
    step();
    assert(requested.size()==1&&requested[0]);
    if(mode=="--cold") {
        assert(reported==State::Requested);
        Publish(false);step();assert(requested.size()==2&&!requested.back());
        std::puts("Session native uses explicit enable/disable when no stock request was observed");return 0;
    }
    if(mode=="--mismatch"||mode=="--throw") {
        assert(reported==State::Unconfirmed);step();assert(requested.size()==1);
        if(mode=="--throw") {try{forbid(nullptr,false,nullptr);assert(false);}catch(int value){assert(value==7);}}
        std::printf("Session native %s preserves unknown outcomes\n",mode.c_str());return 0;
    }
    assert(reported==State::Requested);step();assert(requested.size()==1);
    if(mode=="--stock-on") {
        Publish(false);step();assert(requested.size()==1&&reported==State::Off);
        std::puts("Session native preserves the game's enabled stock policy");return 0;
    }
    forbid(nullptr,false,reinterpret_cast<void*>(777));assert(requested.size()==2&&requested.back());
    Publish(false);step();assert(requested.size()==3&&!requested.back()&&reported==State::Restored);
    step();assert(requested.size()==3);
    Publish(true);step();assert(requested.size()==4&&requested.back());
    disconnect(zone.data(),reinterpret_cast<void*>(123));assert(ends==1&&reported==State::WaitingForLogin);
    Put(zone,0x34,std::uint8_t{0});step();assert(requested.size()==4);
    Put(zone,0x34,std::uint8_t{1});step();assert(requested.size()==5&&requested.back());
    notifyResult=false;assert(!disconnected(zone.data(),reinterpret_cast<void*>(123))&&ends==2);
    step();assert(requested.size()==6);
    reset(zone.data(),reinterpret_cast<void*>(123));assert(ends==3);step();assert(requested.size()==7);
    shutdown(zone.data(),reinterpret_cast<void*>(123));assert(ends==4&&reported==State::WaitingForLogin);
    int object=0;assert(!send(&object,&object,42,123,reinterpret_cast<void*>(321)));
    std::puts("Session native: login gating, typed request observation, stock restoration, lifecycle invalidation and unchanged forwarding passed");
}
