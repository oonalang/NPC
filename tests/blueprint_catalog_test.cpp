#include <array>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <string>
#include <cstring>
#include <cstdint>
#include <cassert>
namespace Tools { bool IsPtrValid(void* p) { return p!=nullptr; } }
bool loadskinhack=false;
std::mutex g_dataMutex;
std::vector<void*> weaponConfInstance;
struct itemInfo { std::string itemName; std::array<int,5> WeaponConf; };
std::vector<itemInfo> itemData;
std::unordered_map<std::string,bool> sBool;
struct { struct { bool Blueprints=true; } ExtraMenu; } Config;
uint32_t applied=0;
void ApplyWeaponSkin(const itemInfo& item) { applied=item.WeaponConf[2]; }
uintptr_t getAbsoluteAddress(const char*,uintptr_t) { return 0; }
int DobbyHook(void*,void*,void**) { return 0; }
#include "../src/main/jni/BlueprintEquipment.h"
int main() {
    std::array<std::array<unsigned char,0x48>,3> rows{};
    const uint32_t ids[]{101,102,103};
    const uint8_t colors[]{5,4,3};
    for(int i=0;i<3;++i) {
        std::memcpy(rows[i].data()+0x40,&ids[i],4);
        rows[i][0x22]=colors[i];
        weaponConfInstance.push_back(rows[i].data());
    }
    BlueprintTierFilter::SetMask(BlueprintTierFilter::Mythic|BlueprintTierFilter::Legendary);
    std::array<unsigned char,0x48> late{};
    weaponConfInstance.push_back(late.data());
    BlueprintEquipment::RefreshTierCatalog(0.0);
    assert(!loadskinhack && itemData.empty());
    assert(BlueprintEquipment::CatalogContains(101));
    assert(BlueprintEquipment::CatalogContains(102));
    assert(!BlueprintEquipment::CatalogContains(103));
    const uint32_t lateId=104;
    std::memcpy(late.data()+0x40,&lateId,4);late[0x22]=4;
    BlueprintEquipment::RefreshTierCatalog(.1);
    assert(!BlueprintEquipment::CatalogContains(104));
    BlueprintEquipment::RefreshTierCatalog(.6);
    assert(BlueprintEquipment::CatalogContains(104));
    BlueprintEquipment::pendingId.store(101);
    BlueprintEquipment::ApplyPending();
    assert(applied==0 && BlueprintEquipment::pendingId.load()==101);
    itemData.push_back({"Mythic",{1,5,101,0,0}});
    itemData.push_back({"Legendary",{2,4,102,0,0}});
    loadskinhack=true;
    BlueprintEquipment::ApplyPending();
    assert(applied==101 && BlueprintEquipment::IsEquipped(nullptr,1,101,-1,nullptr));
    BlueprintEquipment::pendingId.store(102);
    BlueprintEquipment::ApplyPending();
    assert(applied==102 && BlueprintEquipment::IsEquipped(nullptr,2,102,-1,nullptr));
    BlueprintEquipment::pendingId.store(103);
    BlueprintEquipment::ApplyPending();
    assert(applied==102);
}
