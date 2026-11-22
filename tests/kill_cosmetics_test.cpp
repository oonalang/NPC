#include <cassert>
#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <thread>
#include "../src/main/jni/foxcheats/include/json.hpp"
#include "../src/main/jni/KillCosmetics/Config.h"
struct Tools {static bool ReadAddr(void* p,void* out,std::size_t n){if(!p)return false;std::memcpy(out,p,n);return true;}};
int DobbyHook(void*,void*,void**){return -1;}
#include "../src/main/jni/KillCosmetics/Native.h"
namespace license {bool authorized=false;bool Allows(Capability) noexcept{return authorized;}}
namespace PhantomTrigger {namespace NativeProfileDetail {
bool ValidateEntrypoints(std::uintptr_t,const CodeCheck*,std::size_t,const char*&) noexcept{return false;}
}}
using namespace kill_cosmetics;
namespace {
std::array<unsigned char,0x600> pawn{};
std::array<unsigned char,0x200> info{},inventory{},data{};
std::array<unsigned char,0x40> bg{},fg{};
std::unordered_map<int,void*> roots;
int nextHandle=1,peakRoots=0,localCalls=0,feedCalls=0;
bool nested=false,change=false,reloadInMedal=false,customMedal=false;
void* iconConfig=nullptr;
std::uint32_t expectedBroadcast=680000021;
template<class T,class V>void Put(T& b,std::size_t off,V v){std::memcpy(b.data()+off,&v,sizeof(v));}
std::uint32_t Word(void* p,std::size_t off){std::uint32_t v;std::memcpy(&v,static_cast<unsigned char*>(p)+off,4);return v;}
void* Local(void* a,void* m){assert(!a&&!m);++localCalls;return pawn.data();}
std::uint32_t Player(void* p,void* m){assert(p==pawn.data()&&!m);return 71;}
void* Inventory(void* p,void* m){assert(p==info.data()&&!m);return inventory.data();}
int Allocate(void* a,void* p,int t,void* m){assert(!a&&p&&t==2&&!m);roots[nextHandle]=p;peakRoots=std::max(peakRoots,static_cast<int>(roots.size()));return nextHandle++;}
void* Target(void* a,int h,void* m){assert(!a&&!m&&roots.count(h));return roots.at(h);}
void Release(void* a,int h,void* m){assert(!a&&!m&&roots.erase(h)==1);}
std::uint32_t Board(void*,std::uint32_t p,void*){return p+1;}
void Feed(void*,void*,void*){
 ++feedCalls;assert(Word(inventory.data(),0x58)==expectedBroadcast);
 if(nested){nested=false;native::Feed(nullptr,data.data(),nullptr);assert(Word(inventory.data(),0x58)==expectedBroadcast);}
 if(change)Put(inventory,0x58,std::uint32_t(680000025));
}
void StockFeed(void*,void*,void*){++feedCalls;assert(Word(inventory.data(),0x58)==19);}
bool Medal(void*,void*,void*,std::uint64_t,int level,bool,void* back,void* front,int effect,int,void*){
 if(customMedal&&level<=0){assert(back==bg.data()&&front==fg.data()&&effect==805&&roots.size()==4);}
 else assert(!back&&!front&&effect==7);
 if(reloadInMedal){reloadInMedal=false;native::Capture(Kind::Icon,iconConfig);assert(roots.size()==4);}
 return true;
}
int Deserialize(void*,void**,void*){return 19;}
Row Linked(std::uint32_t id,std::uint32_t weapon){Row row;row.id=id;row.asset=700;row.weapons[0]=weapon;row.weaponCount=1;return row;}
}
int main(){
 WeaponCatalog tiers;Weapon result;
 tiers.Record({2001,100,5001},5);tiers.Record({3001,100,5001},4);
 assert(tiers.Resolve(100,2001,0,result)&&result.id==2001);
 assert(!tiers.Resolve(100,3001,2001,result));assert(!tiers.Resolve(100,0,5001,result));
 tiers.SetVisual(100,2001);assert(tiers.Resolve(100,3001,0,result)&&result.id==2001);
 tiers.SetVisual(100,3001);assert(!tiers.Resolve(100,2001,0,result));
 tiers.Record({2001,100,5001},3);assert(!tiers.Resolve(2001,0,0,result));
 Catalog bounded;
 for(unsigned i=0;i<maximumRows+8;++i)bounded.Put(Kind::Board,Linked(610000001+i,2001));
 assert(bounded.Count(Kind::Board)==maximumRows);
 assert(!bounded.Put(Kind::Icon,Linked(610000001,2001)));
 Row selected;assert(!bounded.Match(Kind::Board,{2001,100,5001},0,selected));
 assert(bounded.Match(Kind::Board,{2001,100,5001},610000007,selected)&&selected.id==610000007);
 nlohmann::json json;configured.automatic=false;Save(json);configured={};Load(json);assert(!requested.load());
 json["KillCosmetics"]={{"Kill Icon",600000001},{"Kill Board",610000001},{"Kill Broadcast",680000021}};Load(json);assert(requested.load());
 json["KillCosmetics"]["Automatic"]="false";Load(json);assert(requested.load());Load(nlohmann::json::object());assert(requested.load());
 native::bindingsReady.store(true);native::localPawn=Local;native::playerId=Player;native::currentInventory=Inventory;
 native::allocateRoot=Allocate;native::rootTarget=Target;native::releaseRoot=Release;
 native::originalBoard=Board;native::originalFeed=StockFeed;native::originalMedal=Medal;
 native::originalBoardData=native::originalBroadcastData=native::originalIconData=native::originalWeaponData=Deserialize;
 Put(pawn,0x5c8,static_cast<void*>(info.data()));Put(inventory,0x14,std::uint32_t(100));Put(inventory,0x38,std::uint32_t(2001));Put(inventory,0x58,std::uint32_t(19));
 Put(data,0x14,std::uint32_t(71));Put(data,0x88,int(100));Put(bg,0x10,int(3));Put(fg,0x10,int(3));
 std::array<unsigned char,0x80> board{},broadcast{},icon{},weapon{};std::array<unsigned char,0x20+maximumWeapons*4> allowed{};
 Put(allowed,0x18,std::uintptr_t(1));Put(allowed,0x20,std::uint32_t(2001));
 Put(board,0x28,std::uint32_t(610000001));Put(board,0x14,int(700));Put(board,0x38,int(701));Put(board,0x68,static_cast<void*>(allowed.data()));
 Put(broadcast,0x24,std::uint32_t(680000021));Put(broadcast,0x34,int(801));Put(broadcast,0x60,static_cast<void*>(allowed.data()));
 Put(icon,0x2c,std::uint32_t(600000001));Put(icon,0x14,int(804));Put(icon,0x24,int(805));Put(icon,0x40,static_cast<void*>(bg.data()));Put(icon,0x68,static_cast<void*>(fg.data()));Put(icon,0x70,static_cast<void*>(allowed.data()));iconConfig=icon.data();
 Put(weapon,0x40,std::uint32_t(2001));Put(weapon,0x34,std::uint32_t(100));Put(weapon,0x38,std::uint32_t(5001));Put(weapon,0x22,std::uint8_t(5));
 native::BoardData(board.data(),nullptr,nullptr);native::BroadcastData(broadcast.data(),nullptr,nullptr);native::IconData(icon.data(),nullptr,nullptr);native::WeaponData(weapon.data(),nullptr,nullptr);
 assert(catalog.Count(Kind::Icon)==1&&weapons.Count()==1&&roots.size()==2);
 available.store(true);assert(native::Board(nullptr,71,nullptr)==72&&localCalls==0);
 license::authorized=true;assert(native::Board(nullptr,71,nullptr)==610000001);assert(native::Board(nullptr,72,nullptr)==73);
 native::originalFeed=Feed;nested=true;native::Feed(nullptr,data.data(),nullptr);assert(Word(inventory.data(),0x58)==19);
 change=true;native::Feed(nullptr,data.data(),nullptr);assert(Word(inventory.data(),0x58)==680000025);change=false;Put(inventory,0x58,std::uint32_t(19));
 customMedal=true;for(int i=0;i<300;++i)assert(native::Medal(nullptr,nullptr,nullptr,0,-1,false,nullptr,nullptr,7,0,nullptr));
 reloadInMedal=true;native::Medal(nullptr,nullptr,nullptr,0,-1,false,nullptr,nullptr,7,0,nullptr);assert(roots.size()==2&&peakRoots<=6);
 native::Medal(nullptr,nullptr,nullptr,0,1000,false,nullptr,nullptr,7,0,nullptr);
 Put(inventory,0x38,std::uint32_t(3001));native::originalFeed=StockFeed;customMedal=false;
 assert(native::Board(nullptr,71,nullptr)==72);native::Feed(nullptr,data.data(),nullptr);native::Medal(nullptr,nullptr,nullptr,0,-1,false,nullptr,nullptr,7,0,nullptr);
 Put(inventory,0x38,std::uint32_t(2002));weapons.Record({2002,100,5002},5);assert(native::Board(nullptr,71,nullptr)==72);
 auto second=Linked(610000002,2002);assert(catalog.Put(Kind::Board,second));assert(native::Board(nullptr,71,nullptr)==610000002);
 Put(inventory,0x38,std::uint32_t(2001));assert(native::Board(nullptr,71,nullptr)==610000001);
 assert(catalog.Put(Kind::Board,Linked(610000003,2001)));assert(native::Board(nullptr,71,nullptr)==72);
 Put(inventory,0x48,std::uint32_t(610000003));assert(native::Board(nullptr,71,nullptr)==610000003);
 Put(inventory,0x48,std::uint32_t(610000002));assert(native::Board(nullptr,71,nullptr)==72);
 weapons.SetVisual(100,2002);assert(native::Board(nullptr,71,nullptr)==610000002);
 weapons.SetVisual(100,3001);assert(native::Board(nullptr,71,nullptr)==72);
 configured.automatic=false;Publish(configured);auto calls=localCalls;native::Feed(nullptr,data.data(),nullptr);assert(calls==localCalls);
 const auto count=catalog.Count(Kind::Icon);Put(icon,0x2c,std::uint32_t(600000004));Put(allowed,0x18,std::uintptr_t(maximumWeapons+1));native::IconData(icon.data(),nullptr,nullptr);assert(catalog.Count(Kind::Icon)==count&&roots.size()==2);
 bool caught=false;try{ScopedBroadcast scoped(inventory.data(),680000021);throw 1;}catch(int){caught=true;}assert(caught&&Word(inventory.data(),0x58)==19);
 std::thread writer([&]{for(int i=0;i<1000;++i)catalog.Put(Kind::Board,Linked(610000003,2001+i%2));});
 for(int i=0;i<1000;++i){catalog.Match(Kind::Board,{2001,100,5001},0,selected);}
 writer.join();
 available.store(false);assert(!native::Install(0));
 std::puts("Automatic mythic cosmetics: verified weapon tier, native weapon associations, switching and visual overrides, ambiguity/native preference, non-mythic stock behavior, bounds, legacy-ID migration, native adapters, locality/license/disable guards, scoped restoration and rooted lifetime passed");
}
