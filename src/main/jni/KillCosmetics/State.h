#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <map>
#include <mutex>
#include <vector>

namespace kill_cosmetics {
enum class Kind : unsigned { Icon, Board, Broadcast };
inline constexpr std::size_t categoryCount=3,maximumRows=512,maximumWeapons=64;
inline constexpr std::uint32_t itemTypes[]{60,61,68};
inline constexpr const char* labels[]{"Kill Icon","Kill Board","Kill Broadcast"};
inline std::size_t Index(Kind kind) {return static_cast<std::size_t>(kind);}
inline bool Valid(Kind kind,std::uint32_t id) {
    return Index(kind)<categoryCount && id>0 && id/10000000u==itemTypes[Index(kind)];
}
struct Options {bool automatic=true;};
inline Options configured;
inline std::atomic<bool> requested{true};
inline void Publish(Options options) {requested.store(options.automatic,std::memory_order_release);}
struct Weapon {std::uint32_t id=0,base=0,skin=0;};
class WeaponCatalog {
public:
    void Record(Weapon weapon,unsigned rarity) {
        if(!weapon.id) return;
        std::lock_guard<std::mutex> lock(mutex_);
        if(rarity!=5) {if(!rows_.erase(weapon.id))return;}
        else {
            const auto found=rows_.find(weapon.id);
            if(found==rows_.end() && rows_.size()>=maximumRows)return;
            if(found!=rows_.end() && found->second.base==weapon.base && found->second.skin==weapon.skin)return;
            rows_[weapon.id]=weapon;
        }

    }
    void SetVisual(std::uint32_t item,std::uint32_t blueprint) {
        if(!item)return;
        std::lock_guard<std::mutex> lock(mutex_);
        if(visual_.find(item)==visual_.end() && visual_.size()>=maximumRows*2)return;
        visual_[item]=blueprint;
    }
    bool Resolve(std::uint32_t item,std::uint32_t blueprint,std::uint32_t skin,Weapon& result) const {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto visual=visual_.find(item);
        const auto candidate=visual!=visual_.end()?visual->second:blueprint?blueprint:item;
        auto found=rows_.find(candidate);
        if(found==rows_.end() && visual==visual_.end() && !blueprint && skin)found=rows_.find(skin);
        if(found==rows_.end())return false;
        result=found->second;return true;
    }
    std::size_t Count() const {std::lock_guard<std::mutex> lock(mutex_);return rows_.size();}
private:
    mutable std::mutex mutex_;
    std::map<std::uint32_t,Weapon> rows_;
    std::map<std::uint32_t,std::uint32_t> visual_;
};
inline WeaponCatalog weapons;
struct Row {
    std::uint32_t id=0;
    int asset=0,effect=0;
    std::uint16_t weaponCount=0;
    std::array<std::uint32_t,maximumWeapons> weapons{};
    bool operator==(const Row& other) const {
        return id==other.id && asset==other.asset && effect==other.effect && weaponCount==other.weaponCount && weapons==other.weapons;
    }
};
class Catalog {
public:
    bool Put(Kind kind,Row row) {
        if(!Valid(kind,row.id) || row.asset<0 || row.effect<0 || row.weaponCount>maximumWeapons)return false;
        std::sort(row.weapons.begin(),row.weapons.begin()+row.weaponCount);
        row.weaponCount=static_cast<std::uint16_t>(std::unique(row.weapons.begin(),row.weapons.begin()+row.weaponCount)-row.weapons.begin());
        if(row.weaponCount && !row.weapons[0])return false;
        std::fill(row.weapons.begin()+row.weaponCount,row.weapons.end(),0);
        std::lock_guard<std::mutex> lock(mutex_);
        auto& rows=rows_[Index(kind)];auto& links=links_[Index(kind)];
        const auto found=rows.find(row.id);
        if(found==rows.end() && rows.size()>=maximumRows)return false;
        if(found!=rows.end() && found->second==row)return true;
        if(found!=rows.end())for(std::size_t i=0;i<found->second.weaponCount;++i) {
            const auto link=links.find(found->second.weapons[i]);if(link==links.end())continue;
            auto& ids=link->second;ids.erase(std::remove(ids.begin(),ids.end(),row.id),ids.end());
            if(ids.empty())links.erase(link);
        }
        rows[row.id]=row;
        for(std::size_t i=0;i<row.weaponCount;++i)links[row.weapons[i]].push_back(row.id);
        return true;
    }
    bool Match(Kind kind,const Weapon& weapon,std::uint32_t preferred,Row& result) const {
        if(Index(kind)>=categoryCount || !weapon.id)return false;
        std::lock_guard<std::mutex> lock(mutex_);
        const auto& rows=rows_[Index(kind)];const auto& links=links_[Index(kind)];
        const auto native=rows.find(preferred);
        if(native!=rows.end()) {
            const auto& row=native->second;
            if(!row.weaponCount){result=row;return true;}
            for(std::size_t i=0;i<row.weaponCount;++i)if(row.weapons[i]==weapon.id || row.weapons[i]==weapon.skin || row.weapons[i]==weapon.base){result=row;return true;}
        }
        for(const auto key:{weapon.id,weapon.skin,weapon.base})if(key) {
            const auto match=links.find(key);if(match==links.end())continue;
            if(match->second.size()!=1)return false;
            result=rows.at(match->second.front());return true;
        }
        return false;
    }
    std::size_t Count(Kind kind) const {
        if(Index(kind)>=categoryCount)return 0;
        std::lock_guard<std::mutex> lock(mutex_);return rows_[Index(kind)].size();
    }
private:
    mutable std::mutex mutex_;
    std::array<std::map<std::uint32_t,Row>,categoryCount> rows_;
    std::array<std::map<std::uint32_t,std::vector<std::uint32_t>>,categoryCount> links_;
};
inline Catalog catalog;
inline std::atomic<bool> available{false},attempted{false};
inline bool LocalKill(std::uint32_t local,std::uint32_t killer,bool authorized) {return authorized && local && local==killer;}
class ScopedBroadcast {
public:
    explicit ScopedBroadcast(void* inventory,std::uint32_t id):inventory_(inventory),applied_(id) {
        if(!inventory || !Valid(Kind::Broadcast,id) || current_==inventory) {inventory_=nullptr;return;}
        previous_=current_;current_=inventory;
        auto* field=static_cast<unsigned char*>(inventory)+0x58;
        std::memcpy(&original_,field,4);std::memcpy(field,&applied_,4);
    }
    ~ScopedBroadcast() {
        if(!inventory_) return;
        auto* field=static_cast<unsigned char*>(inventory_)+0x58;std::uint32_t value;
        std::memcpy(&value,field,4);
        if(value==applied_) std::memcpy(field,&original_,4);
        current_=previous_;
    }
    ScopedBroadcast(const ScopedBroadcast&)=delete;
    ScopedBroadcast& operator=(const ScopedBroadcast&)=delete;
private:
    inline static thread_local void* current_=nullptr;
    void* inventory_=nullptr;
    void* previous_=nullptr;
    std::uint32_t original_=0,applied_=0;
};
}
