#include <cassert>
#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <thread>
#include "../src/main/jni/foxcheats/include/json.hpp"
#include "../src/main/jni/KillCosmetics/Config.h"
struct Tools {
    static bool ReadAddr(void* address,void* out,std::size_t length) {
        if(!address) return false;
        std::memcpy(out,address,length);return true;
    }
};
int DobbyHook(void*,void*,void**) {return -1;}
#include "../src/main/jni/KillCosmetics/Native.h"
namespace license { bool authorized=false;bool Allows(Capability) noexcept {return authorized;} }
namespace PhantomTrigger { namespace NativeProfileDetail {
bool ValidateEntrypoints(std::uintptr_t,const CodeCheck*,std::size_t,const char*&) noexcept {return false;}
}}
using namespace kill_cosmetics;
namespace {
std::array<unsigned char,0x600> pawn{};
std::array<unsigned char,0x200> info{},inventory{},data{};
int localCalls=0,feedCalls=0,medalCalls=0;
std::unordered_map<int,void*> roots;
int nextHandle=1,peakRoots=0;
bool nested=false,change=false,reloadInMedal=false;
void* iconConfig=nullptr;
template<class T,class V> void Put(T& object,std::size_t offset,V value){std::memcpy(object.data()+offset,&value,sizeof(value));}
std::uint32_t Word(void* object,std::size_t offset){std::uint32_t value;std::memcpy(&value,static_cast<unsigned char*>(object)+offset,4);return value;}
void* Local(void* unused,void* method){assert(!unused&&!method);++localCalls;return pawn.data();}
std::uint32_t Player(void* object,void* method){assert(object==pawn.data()&&!method);return 71;}
void* Inventory(void* object,void* method){assert(object==info.data()&&!method);return inventory.data();}
int Allocate(void* unused,void* value,int type,void* method){assert(!unused&&!method&&type==2&&value);roots[nextHandle]=value;peakRoots=std::max(peakRoots,static_cast<int>(roots.size()));return nextHandle++;}
void* Target(void* unused,int handle,void* method){assert(!unused&&!method);assert(roots.count(handle));return roots[handle];}
void Release(void* unused,int handle,void* method){assert(!unused&&!method&&roots.erase(handle)==1);}
std::uint32_t Board(void*,std::uint32_t player,void*){return player+1;}
void Feed(void*,void*,void*){
 ++feedCalls;assert(Word(inventory.data(),0x58)==680000021);
 if(nested){nested=false;native::Feed(nullptr,data.data(),nullptr);assert(Word(inventory.data(),0x58)==680000021);}
 if(change)Put(inventory,0x58,std::uint32_t(680000025));
}
void StockFeed(void*,void*,void*){++feedCalls;assert(Word(inventory.data(),0x58)==19);}
std::array<unsigned char,0x40> bg{},fg{};
bool Medal(void*,void*,void*,std::uint64_t,int level,bool,void* background,void* foreground,int effect,int,void*){
 ++medalCalls;
 if(level<=0){assert(background==bg.data()&&foreground==fg.data()&&effect==805);assert(roots.size()==4);}
 else {assert(!background&&!foreground&&effect==7);assert(roots.size()==2);}
 if(reloadInMedal){reloadInMedal=false;native::Capture(Kind::Icon,iconConfig);assert(roots.size()==4);}
 return true;
}
int Deserialize(void*,void**,void*) {return 0;}
}
int main(){
 Catalog bounded;
 for(int i=0;i<static_cast<int>(maximumRows)+8;++i)bounded.Put(Kind::Board,{610000001u+static_cast<unsigned>(i),5,200,0});
 assert(bounded.Rows(Kind::Board).size()==maximumRows);
 assert(!bounded.Put(Kind::Icon,{610000001,5,200,0}));assert(!bounded.Put(static_cast<Kind>(3),{610000001,5,200,0}));
 assert(!bounded.Put(Kind::Board,{610000001,99,200,0}));
 auto revision=bounded.Revision();assert(bounded.Put(Kind::Board,{610000001,5,200,0}));assert(bounded.Revision()==revision);
 nlohmann::json json;
 configured.ids={600000001,610000001,680000021};Save(json);configured={};Load(json);assert(Requested(Kind::Broadcast)==680000021);
 json["KillCosmetics"]["Kill Icon"]=610000001;json["KillCosmetics"]["Kill Board"]=-1;json["KillCosmetics"]["Kill Broadcast"]=UINT64_MAX;
 Load(json);assert(configured.ids==Options{}.ids);
 json["KillCosmetics"]["Kill Icon"]="600000001";json["KillCosmetics"]["Kill Board"]=610000001.5;Load(json);assert(configured.ids==Options{}.ids);
 Load(nlohmann::json::object());assert(Requested(Kind::Icon)==0);
 native::bindingsReady.store(true);native::localPawn=Local;native::playerId=Player;native::currentInventory=Inventory;
 native::allocateRoot=Allocate;native::rootTarget=Target;native::releaseRoot=Release;
 native::originalBoard=Board;native::originalFeed=StockFeed;native::originalMedal=Medal;
 Put(pawn,0x5c8,static_cast<void*>(info.data()));Put(inventory,0x14,int(100));Put(inventory,0x58,std::uint32_t(19));
 Put(data,0x14,std::uint32_t(71));Put(data,0x88,int(100));Put(bg,0x10,int(3));Put(fg,0x10,int(3));
 std::array<unsigned char,0x80> board{},broadcast{},icon{};
 Put(board,0x28,std::uint32_t(610000001));Put(board,0x18,int(5));Put(board,0x14,int(700));Put(board,0x38,int(701));
 Put(broadcast,0x24,std::uint32_t(680000021));Put(broadcast,0x14,int(5));Put(broadcast,0x34,int(801));
 Put(icon,0x2c,std::uint32_t(600000001));Put(icon,0x18,int(5));Put(icon,0x14,int(804));Put(icon,0x24,int(805));
 iconConfig=icon.data();
 Put(icon,0x40,static_cast<void*>(bg.data()));Put(icon,0x68,static_cast<void*>(fg.data()));
 native::originalBoardData=native::originalBroadcastData=native::originalIconData=Deserialize;
 native::BoardData(board.data(),nullptr,nullptr);native::BroadcastData(broadcast.data(),nullptr,nullptr);native::IconData(icon.data(),nullptr,nullptr);
 assert(catalog.Rows(Kind::Icon).size()==1&&roots.size()==2);
 for(int i=0;i<50;++i)native::IconData(icon.data(),nullptr,nullptr);
 assert(roots.size()==2&&peakRoots<=4);
 configured.ids={600000001,610000001,680000021};Publish(configured);available.store(true);
 assert(native::Board(nullptr,71,nullptr)==72&&localCalls==0);
 native::Feed(nullptr,data.data(),nullptr);assert(localCalls==0);
 license::authorized=true;assert(native::Board(nullptr,71,nullptr)==610000001);assert(native::Board(nullptr,72,nullptr)==73);
 native::originalFeed=Feed;nested=true;native::Feed(nullptr,data.data(),nullptr);assert(Word(inventory.data(),0x58)==19);
 change=true;native::Feed(nullptr,data.data(),nullptr);assert(Word(inventory.data(),0x58)==680000025);change=false;Put(inventory,0x58,std::uint32_t(19));
 native::originalFeed=StockFeed;Put(data,0x14,std::uint32_t(99));native::Feed(nullptr,data.data(),nullptr);
 Put(data,0x14,std::uint32_t(71));Put(data,0x88,int(101));native::Feed(nullptr,data.data(),nullptr);Put(data,0x88,int(100));
 for(int i=0;i<300;++i)assert(native::Medal(nullptr,nullptr,nullptr,0,-1,false,nullptr,nullptr,7,0,nullptr));
 reloadInMedal=true;native::Medal(nullptr,nullptr,nullptr,0,-1,false,nullptr,nullptr,7,0,nullptr);
 assert(roots.size()==2&&peakRoots<=6);native::Medal(nullptr,nullptr,nullptr,0,1000,false,nullptr,nullptr,7,0,nullptr);
 configured={};Publish(configured);const auto calls=localCalls;native::Feed(nullptr,data.data(),nullptr);assert(localCalls==calls);
 bool caught=false;try {ScopedBroadcast scoped(inventory.data(),680000021);throw 1;}catch(int){caught=true;}assert(caught&&Word(inventory.data(),0x58)==19);
 std::thread writer([&]{for(int i=0;i<5000;++i)catalog.Put(Kind::Board,{610000002,5,100+i,0});});
 for(int i=0;i<5000;++i){Row row;catalog.Find(Kind::Board,610000002,row);}writer.join();
 available.store(false);assert(!native::Install(0));
 std::puts("Kill cosmetics: catalog bounds/types, JSON/defaults, actual native adapters, authorization/locality/weapon guards, stock behavior, nested and exceptional restoration, native updates, GC handle ABI/bounds and concurrent catalog access passed");
}
