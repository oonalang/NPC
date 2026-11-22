#pragma once
#include "State.h"
#include "../Trigger/NativeProfile.h"
#include "../Security/Access.h"

namespace kill_cosmetics { namespace native {
using Metadata=void*;
inline constexpr PhantomTrigger::NativeProfileDetail::CodeCheck profile[]{
#include "Profile.inc"
};
inline std::uintptr_t base=0;
inline std::mutex installMutex;
inline std::size_t installed=0;
inline std::atomic<bool> bindingsReady{false};
using Getter=void*(*)(void*,Metadata);
inline Getter localPawn=nullptr,currentInventory=nullptr;
inline std::uint32_t (*playerId)(void*,Metadata)=nullptr;
inline int (*allocateRoot)(void*,void*,int,Metadata)=nullptr;
inline void* (*rootTarget)(void*,int,Metadata)=nullptr;
inline void (*releaseRoot)(void*,int,Metadata)=nullptr;
using Deserialize=int(*)(void*,void**,Metadata);
inline Deserialize originalBoardData=nullptr,originalBroadcastData=nullptr,originalIconData=nullptr;
inline std::uint32_t (*originalBoard)(void*,std::uint32_t,Metadata)=nullptr;
inline bool (*originalMedal)(void*,void*,void*,std::uint64_t,int,bool,void*,void*,int,int,Metadata)=nullptr;
inline void (*originalFeed)(void*,void*,Metadata)=nullptr;
template<class T> inline bool Read(const void* object,std::size_t offset,T& value) {
    return object && Tools::ReadAddr(reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(object)+offset),&value,sizeof(value));
}
inline bool String(void* value) {
    int length=0;return value && Read(value,0x10,length) && length>=0 && length<=1024;
}
inline int Root(void* value) {
    return allocateRoot && value && String(value)?allocateRoot(nullptr,value,2,nullptr):0;
}
inline void Free(int handle) {
    if(handle && releaseRoot) releaseRoot(nullptr,handle,nullptr);
}
inline void* Target(int handle) {
    return handle && rootTarget?rootTarget(nullptr,handle,nullptr):nullptr;
}
struct IconRoots { int background=0,foreground=0,effect=0; };
struct ScopedHandle {
    int value;
    explicit ScopedHandle(int handle):value(handle) {}
    ~ScopedHandle() {Free(value);}
    ScopedHandle(const ScopedHandle&)=delete;
    ScopedHandle& operator=(const ScopedHandle&)=delete;
};
struct ScopedRoots {
    ScopedRoots()=default;
    ScopedRoots(const ScopedRoots&)=delete;
    ScopedRoots& operator=(const ScopedRoots&)=delete;
    int background=0,foreground=0;
    ~ScopedRoots() {Free(background);Free(foreground);}
};
struct Roots {
    std::mutex mutex;
    std::map<std::uint32_t,IconRoots> icons;
};
inline Roots& RootStore() { static Roots* roots=new Roots;return *roots; }
inline void Capture(Kind kind,void* object) {
    if(!bindingsReady.load(std::memory_order_acquire)) return;
    Row row;
    const bool broadcast=kind==Kind::Broadcast;
    if(!Read(object,broadcast?0x24:kind==Kind::Board?0x28:0x2c,row.id) ||
       !Read(object,broadcast?0x14:0x18,row.rarity)) return;
    if(broadcast) { if(!Read(object,0x34,row.asset)) return; }
    else {
        if(!Read(object,0x14,row.asset) || !Read(object,kind==Kind::Board?0x38:0x24,row.effect)) return;
    }
    if(!catalog.Put(kind,row) || kind!=Kind::Icon) return;
    void* background=nullptr;void* foreground=nullptr;
    if(!Read(object,0x40,background) || !Read(object,0x68,foreground)) return;
    const auto next=IconRoots{Root(background),Root(foreground),row.effect};
    auto& store=RootStore();std::lock_guard<std::mutex> lock(store.mutex);
    auto& previous=store.icons[row.id];
    Free(previous.background);Free(previous.foreground);previous=next;
}
inline int BoardData(void* object,void** bytes,Metadata method) {
    const int result=originalBoardData(object,bytes,method);if(result>=0) Capture(Kind::Board,object);return result;
}
inline int BroadcastData(void* object,void** bytes,Metadata method) {
    const int result=originalBroadcastData(object,bytes,method);if(result>=0) Capture(Kind::Broadcast,object);return result;
}
inline int IconData(void* object,void** bytes,Metadata method) {
    const int result=originalIconData(object,bytes,method);if(result>=0) Capture(Kind::Icon,object);return result;
}
struct Local { void* pawn=nullptr;void* info=nullptr;std::uint32_t id=0; };
inline Local Owner() {
    Local local;
    if(!localPawn || !playerId) return {};
    local.pawn=localPawn(nullptr,nullptr);
    if(!Read(local.pawn,0x5c8,local.info) || !local.info) return {};
    local.id=playerId(local.pawn,nullptr);
    return local;
}
inline bool Selected(Kind kind,Row& row) {
    const auto id=Requested(kind);
    return id && available.load(std::memory_order_acquire) && license::Allows(license::Capability::Features) && catalog.Find(kind,id,row);
}
inline std::uint32_t Board(void* hud,std::uint32_t player,Metadata method) {
    Row row;if(Selected(Kind::Board,row)) {
        const auto local=Owner();if(LocalKill(local.id,player,true)) return row.id;
    }
    return originalBoard(hud,player,method);
}
inline bool Medal(void* hud,void* icon,void* name,std::uint64_t sound,int level,bool pending,
                  void* background,void* foreground,int effect,int difficulty,Metadata method) {
    Row row;
    ScopedRoots retained;
    if(level<=0 && Selected(Kind::Icon,row) && Owner().id) {
        auto& store=RootStore();
        {
            std::lock_guard<std::mutex> lock(store.mutex);
            const auto found=store.icons.find(row.id);
            if(found!=store.icons.end()) {
                retained.background=Root(Target(found->second.background));
                retained.foreground=Root(Target(found->second.foreground));
                effect=found->second.effect;
            }
        }
        if(auto* value=Target(retained.background)) background=value;
        if(auto* value=Target(retained.foreground)) foreground=value;
    }
    return originalMedal(hud,icon,name,sound,level,pending,background,foreground,effect,difficulty,method);
}
inline void Feed(void* cell,void* data,Metadata method) {
    Row row;std::uint32_t killer=0;
    if(!Selected(Kind::Broadcast,row) || !Read(data,0x14,killer)) {originalFeed(cell,data,method);return;}
    const auto local=Owner();
    if(!LocalKill(local.id,killer,true)) {originalFeed(cell,data,method);return;}
    auto* inventory=currentInventory?currentInventory(local.info,nullptr):nullptr;
    int item=0,weapon=0;std::uint32_t previous=0;
    if(!Read(inventory,0x14,item) || !Read(data,0x88,weapon) || item<=0 || item!=weapon || !Read(inventory,0x58,previous)) {
        originalFeed(cell,data,method);return;
    }
    ScopedHandle retained(allocateRoot(nullptr,inventory,2,nullptr));
    if(!retained.value) {originalFeed(cell,data,method);return;}
    ScopedBroadcast scoped(inventory,row.id);originalFeed(cell,data,method);
}
inline bool Install(std::uintptr_t library) {
    std::lock_guard<std::mutex> lock(installMutex);
    if(available.load(std::memory_order_acquire)) return true;
    attempted.store(true,std::memory_order_release);
    if(!library || (base && base!=library)) return false;
    const char* failure=nullptr;
    if(!PhantomTrigger::NativeProfileDetail::ValidateEntrypoints(library,profile+installed,std::size(profile)-installed,failure)) return false;
    base=library;
    localPawn=reinterpret_cast<Getter>(base+0x8016060);
    playerId=reinterpret_cast<decltype(playerId)>(base+0x538a910);
    currentInventory=reinterpret_cast<Getter>(base+0xa0bbaf8);
    allocateRoot=reinterpret_cast<decltype(allocateRoot)>(base+0x5208604);
    rootTarget=reinterpret_cast<decltype(rootTarget)>(base+0x52084a8);
    releaseRoot=reinterpret_cast<decltype(releaseRoot)>(base+0x5208638);
    bindingsReady.store(true,std::memory_order_release);
    void* hooks[]{reinterpret_cast<void*>(BoardData),reinterpret_cast<void*>(BroadcastData),reinterpret_cast<void*>(IconData),
                  reinterpret_cast<void*>(Board),reinterpret_cast<void*>(Medal),reinterpret_cast<void*>(Feed)};
    void** originals[]{reinterpret_cast<void**>(&originalBoardData),reinterpret_cast<void**>(&originalBroadcastData),reinterpret_cast<void**>(&originalIconData),
                       reinterpret_cast<void**>(&originalBoard),reinterpret_cast<void**>(&originalMedal),reinterpret_cast<void**>(&originalFeed)};
    while(installed<std::size(hooks)) {
        const auto status=DobbyHook(reinterpret_cast<void*>(base+profile[installed].rva),hooks[installed],originals[installed]);
        if(status!=0 || !*originals[installed]) return false;
        ++installed;
    }
    available.store(true,std::memory_order_release);return true;
}
}}
