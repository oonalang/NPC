#pragma once
#include "State.h"
#include "Memory.h"
#include "../Trigger/NativeProfile.h"
#include "../Security/Access.h"
#include <limits>

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
inline Deserialize originalBoardData=nullptr,originalBroadcastData=nullptr,originalIconData=nullptr,originalWeaponData=nullptr;
inline std::uint32_t (*originalBoard)(void*,std::uint32_t,Metadata)=nullptr;
inline bool (*originalMedal)(void*,void*,void*,std::uint64_t,int,bool,void*,void*,int,int,Metadata)=nullptr;
inline void (*originalFeed)(void*,void*,Metadata)=nullptr;
template<class T> inline bool Read(const void* object,std::size_t offset,T& value) {
    const auto address=reinterpret_cast<std::uintptr_t>(object);
    if(!address || offset>std::numeric_limits<std::uintptr_t>::max()-address) return false;
    const auto field=address+offset;
    if(sizeof(value)>std::numeric_limits<std::uintptr_t>::max()-field) return false;
    T next{};
    if(!memory::Copy(reinterpret_cast<void*>(field),&next,sizeof(next))) return false;
    value=next;return true;
}
template<std::size_t N> struct Snapshot {
    std::array<unsigned char,N> bytes{};
    std::size_t size=0;
    bool valid=false;
    explicit Snapshot(const void* object,std::size_t count=N):size(count),valid(count<=N && memory::Copy(object,bytes.data(),count)) {}
    template<class T> bool Read(std::size_t offset,T& value) const {
        if(!valid || offset>size || sizeof(value)>size-offset)return false;
        std::memcpy(&value,bytes.data()+offset,sizeof(value));return true;
    }
};
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
    const Snapshot<0x78> fields(object,broadcast?0x68:kind==Kind::Board?0x70:0x78);
    if(!fields.Read(broadcast?0x24:kind==Kind::Board?0x28:0x2c,row.id))return;
    if(broadcast) {if(!fields.Read(0x34,row.asset))return;}
    else if(!fields.Read(0x14,row.asset) || !fields.Read(kind==Kind::Board?0x38:0x24,row.effect))return;
    void* allowed=nullptr;
    if(!fields.Read(broadcast?0x60:kind==Kind::Board?0x68:0x70,allowed))return;
    if(allowed) {
        std::uintptr_t count=0;
        if(!Read(allowed,0x18,count) || count>maximumWeapons)return;
        row.weaponCount=static_cast<std::uint16_t>(count);
        if(count && !memory::Copy(static_cast<unsigned char*>(allowed)+0x20,row.weapons.data(),count*4))return;
        for(std::size_t i=0;i<count;++i)if(!row.weapons[i] || row.weapons[i]>INT32_MAX)return;
    }
    if(!catalog.Put(kind,row) || kind!=Kind::Icon) return;
    void* background=nullptr;void* foreground=nullptr;
    if(!fields.Read(0x40,background) || !fields.Read(0x68,foreground)) return;
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
inline int WeaponData(void* object,void** bytes,Metadata method) {
    const int result=originalWeaponData(object,bytes,method);
    if(result>=0 && bindingsReady.load(std::memory_order_acquire)) {
        Weapon weapon;unsigned char rarity=0;
        const Snapshot<0x44> fields(object);
        if(fields.Read(0x40,weapon.id) && fields.Read(0x34,weapon.base) && fields.Read(0x38,weapon.skin) && fields.Read(0x22,rarity))
            weapons.Record(weapon,rarity);
    }
    return result;
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
inline bool Ready() {
    return requested.load(std::memory_order_acquire) && available.load(std::memory_order_acquire) && license::Allows(license::Capability::Features);
}
inline bool Resolve(Kind kind,const Local& local,void*& inventory,Row& row) {
    auto& status=matchStatus[Index(kind)];
    if(!local.id || !currentInventory) {
        status.store(MatchStatus::NoLocalPlayer,std::memory_order_relaxed);return false;
    }
    inventory=currentInventory(local.info,nullptr);
    std::uint32_t item=0,blueprint=0,skin=0,preferred=0;
    constexpr std::size_t offsets[]{0x4c,0x48,0x58};
    if(!Read(inventory,0x14,item) || !Read(inventory,0x38,blueprint) || !Read(inventory,0x34,skin) || !Read(inventory,offsets[Index(kind)],preferred)) {
        status.store(MatchStatus::NoInventory,std::memory_order_relaxed);return false;
    }
    Weapon mythic;
    if(!weapons.Resolve(item,blueprint,skin,mythic)) {
        status.store(MatchStatus::NoMythic,std::memory_order_relaxed);return false;
    }
    const bool matched=catalog.Match(kind,mythic,preferred,row);
    status.store(matched?MatchStatus::Matched:MatchStatus::NoMatch,std::memory_order_relaxed);
    return matched;
}
inline std::uint32_t Board(void* hud,std::uint32_t player,Metadata method) {
    if(Ready()) {
        const auto local=Owner();Row row;void* inventory=nullptr;
        if(LocalKill(local.id,player,true) && Resolve(Kind::Board,local,inventory,row))return row.id;
    }
    return originalBoard(hud,player,method);
}
inline bool Medal(void* hud,void* icon,void* name,std::uint64_t sound,int level,bool pending,
                  void* background,void* foreground,int effect,int difficulty,Metadata method) {
    Row row;
    ScopedRoots retained;
    void* inventory=nullptr;
    if(level<=0 && Ready() && Resolve(Kind::Icon,Owner(),inventory,row)) {
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
    if(!Ready() || !Read(data,0x14,killer)) {originalFeed(cell,data,method);return;}
    const auto local=Owner();
    if(!LocalKill(local.id,killer,true)) {originalFeed(cell,data,method);return;}
    void* inventory=nullptr;
    if(!Resolve(Kind::Broadcast,local,inventory,row)){originalFeed(cell,data,method);return;}
    int item=0,weapon=0;std::uint32_t previous=0;
    if(!Read(inventory,0x14,item) || !Read(data,0x88,weapon) || item<=0 || item!=weapon || !Read(inventory,0x58,previous)) {
        originalFeed(cell,data,method);return;
    }
    ScopedHandle retained(allocateRoot(nullptr,inventory,2,nullptr));
    if(!retained.value) {originalFeed(cell,data,method);return;}
    ScopedBroadcast scoped(inventory,row.id);originalFeed(cell,data,method);
}
inline bool InstallThrough(std::uintptr_t library,std::size_t count) {
    std::lock_guard<std::mutex> lock(installMutex);
    attempted.store(true,std::memory_order_release);
    if(!library || !count || count>7 || (base && base!=library)) return false;
    if(installed>=count) return true;
    const char* failure=nullptr;
    if(!PhantomTrigger::NativeProfileDetail::ValidateEntrypoints(library,profile+installed,std::size(profile)-installed,failure)) return false;
    base=library;
    localPawn=reinterpret_cast<Getter>(base+0x8012a6c);
    playerId=reinterpret_cast<decltype(playerId)>(base+0x538a910);
    currentInventory=reinterpret_cast<Getter>(base+0xa0bbaf8);
    allocateRoot=reinterpret_cast<decltype(allocateRoot)>(base+0x5208604);
    rootTarget=reinterpret_cast<decltype(rootTarget)>(base+0x52084a8);
    releaseRoot=reinterpret_cast<decltype(releaseRoot)>(base+0x5208638);
    bindingsReady.store(true,std::memory_order_release);
    void* hooks[]{reinterpret_cast<void*>(BoardData),reinterpret_cast<void*>(BroadcastData),reinterpret_cast<void*>(IconData),reinterpret_cast<void*>(WeaponData),
                  reinterpret_cast<void*>(Board),reinterpret_cast<void*>(Medal),reinterpret_cast<void*>(Feed)};
    void** originals[]{reinterpret_cast<void**>(&originalBoardData),reinterpret_cast<void**>(&originalBroadcastData),reinterpret_cast<void**>(&originalIconData),reinterpret_cast<void**>(&originalWeaponData),
                       reinterpret_cast<void**>(&originalBoard),reinterpret_cast<void**>(&originalMedal),reinterpret_cast<void**>(&originalFeed)};
    while(installed<count) {
        const auto status=DobbyHook(reinterpret_cast<void*>(base+profile[installed].rva),hooks[installed],originals[installed]);
        if(status!=0 || !*originals[installed]) return false;
        ++installed;
    }
    if(installed==std::size(hooks)) available.store(true,std::memory_order_release);
    return true;
}
inline bool InstallCollectors(std::uintptr_t library) {return InstallThrough(library,4);}
inline bool Install(std::uintptr_t library) {
    return license::Allows(license::Capability::Features) && InstallThrough(library,7);
}
}}
