#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <map>
#include <mutex>
#include <vector>

namespace kill_cosmetics {
enum class Kind : unsigned { Icon, Board, Broadcast };
inline constexpr std::size_t categoryCount=3, maximumRows=512;
inline constexpr std::uint32_t itemTypes[]{60,61,68};
inline constexpr const char* labels[]{"Kill Icon","Kill Board","Kill Broadcast"};
inline std::size_t Index(Kind kind) { return static_cast<std::size_t>(kind); }
inline bool Valid(Kind kind,std::uint32_t id) {
    const auto index=Index(kind);
    return index<categoryCount && id>0 && id/10000000u==itemTypes[index];
}
struct Options { std::array<std::uint32_t,categoryCount> ids{}; };
inline Options configured;
inline std::array<std::atomic<std::uint32_t>,categoryCount> requested{};
inline void Publish(const Options& options) {
    for(std::size_t i=0;i<categoryCount;++i)
        requested[i].store(Valid(static_cast<Kind>(i),options.ids[i])?options.ids[i]:0,std::memory_order_release);
}
inline std::uint32_t Requested(Kind kind) {
    return Index(kind)<categoryCount?requested[Index(kind)].load(std::memory_order_acquire):0;
}
struct Row {
    std::uint32_t id=0;
    int rarity=0,asset=0,effect=0;
    bool operator==(const Row& other) const {
        return id==other.id && rarity==other.rarity && asset==other.asset && effect==other.effect;
    }
};
class Catalog {
public:
    bool Put(Kind kind,Row row) {
        if(!Valid(kind,row.id) || row.rarity<0 || row.rarity>5 || row.asset<0 || row.effect<0) return false;
        std::lock_guard<std::mutex> lock(mutex_);
        auto& rows=rows_[Index(kind)];const auto found=rows.find(row.id);
        if(found==rows.end() && rows.size()>=maximumRows) return false;
        if(found!=rows.end() && found->second==row) return true;
        rows[row.id]=row;revision_.fetch_add(1,std::memory_order_release);return true;
    }
    bool Find(Kind kind,std::uint32_t id,Row& row) const {
        if(!Valid(kind,id)) return false;
        std::lock_guard<std::mutex> lock(mutex_);
        const auto& rows=rows_[Index(kind)];const auto found=rows.find(id);
        if(found==rows.end()) return false;
        row=found->second;return true;
    }
    std::vector<Row> Rows(Kind kind) const {
        if(Index(kind)>=categoryCount) return {};
        std::lock_guard<std::mutex> lock(mutex_);std::vector<Row> result;
        const auto& rows=rows_[Index(kind)];result.reserve(rows.size());
        for(const auto& entry:rows) result.push_back(entry.second);
        return result;
    }
    std::uint64_t Revision() const { return revision_.load(std::memory_order_acquire); }
private:
    mutable std::mutex mutex_;
    std::array<std::map<std::uint32_t,Row>,categoryCount> rows_;
    std::atomic<std::uint64_t> revision_{0};
};
inline Catalog catalog;
inline std::atomic<bool> available{false};
inline std::atomic<bool> attempted{false};
inline bool LocalKill(std::uint32_t local,std::uint32_t killer,bool authorized) {
    return authorized && local!=0 && local==killer;
}
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
