#include "../src/main/jni/System/Hooks/Hitbox.h"
#include "../src/main/jni/System/Hooks/HitboxProfile.h"
#include "../src/main/jni/Trigger/NativeDriver.h"
#include <cassert>
#include <cstdio>
#include <link.h>
#include <sys/mman.h>

namespace {
dl_phdr_info loaded{};
bool loaderReady=false;
int hooks=0,failHook=0;
struct Module {
    static constexpr std::size_t size=0xd530000;
    unsigned char* bytes=static_cast<unsigned char*>(mmap(nullptr,size,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));
    std::array<ElfW(Phdr),3> segments{};
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
        for(const auto& code:hitbox::anchors) std::memcpy(bytes+code.rva,code.bytes.data(),16);
        for(const auto& code:hitbox::encoded) std::memcpy(bytes+code.rva,code.bytes.data(),16);
        loaded.dlpi_addr=base();loaded.dlpi_name="/game/lib/arm64/libunity.so";
        loaded.dlpi_phdr=segments.data();loaded.dlpi_phnum=segments.size();
    }
    ~Module(){munmap(bytes,size);}
    std::uintptr_t base() const{return reinterpret_cast<std::uintptr_t>(bytes);}
    void Decode() {for(const auto& code:hitbox::encoded) std::memcpy(bytes+code.rva,hitbox::anchors[2].bytes.data(),16);}
};
void Hook(void* address,void* replacement,void** original) {
    ++hooks;
    const std::uintptr_t expected[]{0x66F2A78,0x66FCA40,0xC2BE0C8,0xD52E8E4};
    assert(hooks<=4);
    assert(address==reinterpret_cast<void*>(loaded.dlpi_addr+expected[hooks-1])&&replacement&&original);
    *original=hooks==failHook?nullptr:reinterpret_cast<void*>(1);
}
}
extern "C" int dl_iterate_phdr(int(*callback)(dl_phdr_info*,std::size_t,void*),void* context) {
    return loaderReady?callback(&loaded,sizeof(loaded),context):0;
}
namespace PhantomTrigger {
bool InstallNativeDriver(std::uintptr_t) noexcept {return false;}
void NativeDriverFailure() noexcept {}
void NativeDriverPending() noexcept {}
}
int main(int argc,char** argv) {
    Module module;
    if(argc==2) {
        loaderReady=true;module.Decode();
        if(std::strcmp(argv[1],"--build")==0) {
            module.bytes[0x110]^=1;
            assert(!hitbox::Install(module.base(),Hook)&&hooks==0&&hitbox::status==hitbox::Status::Unsupported);
            module.bytes[0x110]^=1;
            assert(!hitbox::Install(module.base(),Hook)&&hooks==0);
        } else if(std::strcmp(argv[1],"--anchor")==0) {
            module.bytes[hitbox::anchors[0].rva]^=1;
            assert(!hitbox::Install(module.base(),Hook)&&hooks==0&&hitbox::status==hitbox::Status::Unsupported);
        } else {
            failHook=2;
            assert(!hitbox::Install(module.base(),Hook)&&hooks==2&&!hitbox::available&&hitbox::status==hitbox::Status::HookFailed);
            assert(!hitbox::Install(module.base(),Hook)&&hooks==2);
        }
    } else {
        assert(!hitbox::Install(module.base(),Hook)&&hooks==0&&hitbox::status==hitbox::Status::Waiting);
        loaderReady=true;
        assert(!hitbox::Install(module.base(),Hook)&&hooks==0&&hitbox::status==hitbox::Status::Waiting);
        module.Decode();
        for(const auto& code:hitbox::encoded) {
            std::memcpy(module.bytes+code.rva,code.bytes.data(),16);
            assert(!hitbox::Install(module.base(),Hook)&&hooks==0&&hitbox::status==hitbox::Status::Waiting);
            std::memcpy(module.bytes+code.rva,hitbox::anchors[2].bytes.data(),16);
        }
        assert(hitbox::Install(module.base(),Hook)&&hooks==4&&hitbox::available&&hitbox::status==hitbox::Status::Ready);
        assert(hitbox::Install(module.base(),Hook)&&hooks==4);
    }
    std::printf("Hitbox profile %s passed\n",argc==2?argv[1]:"loader/decode retry and one-time installation");
}
