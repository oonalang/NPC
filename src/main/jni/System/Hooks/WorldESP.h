#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <unordered_map>
#include <vector>

//-- ESP Items (BR) + ESP Placed Nades (MP)
// RVAs and field offsets verified against the 1.6.59 dump.
namespace world_esp {

using Clock = std::chrono::steady_clock;
using Tick = void (*)(void *, float);

namespace LootField {
    constexpr uintptr_t ItemData = 0x50;      // OpenWorldDroppedPickUpBase.ItemData
    constexpr uintptr_t ItemType = 0x10;      // OpenWorldDroppedPickUpData.ItemType
    constexpr uintptr_t ItemId = 0x1C;        // OpenWorldDroppedPickUpData.ItemID
    constexpr uintptr_t Count = 0x20;         // OpenWorldDroppedPickUpData.Count
    constexpr uintptr_t IsInBox = 0x30;       // OpenWorldDroppedPickUpData.IsInBox
    constexpr uintptr_t Position = 0x4C;      // OpenWorldDroppedPickUpData.Pos
    constexpr uintptr_t Config = 0xD8;        // OpenWorldDroppedPickUpNormal.mDroppedPickUpConfig
}

namespace ProjectileField {
    constexpr uintptr_t Exploded = 0xA1;      // WeaponProjectile.bHasExplode
    constexpr uintptr_t Owner = 0xB8;         // WeaponProjectile.SpawnOwner (Pawn)
    constexpr uintptr_t Destroyed = 0x18D;    // WeaponProjectile.mIsDestroyed
}

struct Loot {
    Vector3 pos;
    int kind = 0;
    int item = 0;
    int count = 0;
    char name[64] = {};
    int nameTries = 0;
    Clock::time_point seen;
};

enum class DeviceKind : uint8_t {
    None, TripMine, C4, Sensor, Trophy, Mine, Drill
};

struct Device {
    Vector3 pos;
    DeviceKind kind = DeviceKind::None;
    uint32_t ownerTeam = 0;
    Clock::time_point seen;
};

inline std::mutex mutex;
inline std::unordered_map<uintptr_t, Loot> lootEntries;
inline std::unordered_map<uintptr_t, Device> deviceEntries;
inline std::atomic<bool> wantLoot{false};
inline std::atomic<bool> wantDevices{false};

inline Tick origLootTick = nullptr;
inline Tick origBRPickupTick = nullptr;
inline Tick origStickTick = nullptr;
inline Tick origTripMineTick = nullptr;
inline Tick origTripWireTick = nullptr;
inline Tick origTrapChipTick = nullptr;
inline Tick origPermeateTick = nullptr;

inline bool ValidWorld(const Vector3 &v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z) &&
        std::abs(v.x) < 200000.0f && std::abs(v.y) < 200000.0f && std::abs(v.z) < 200000.0f;
}

inline const char *ClassName(void *instance) {
    if (!Tools::IsPtrValid(instance)) return nullptr;
    auto *klass = reinterpret_cast<Il2CppObject *>(instance)->klass;
    if (!Tools::IsPtrValid(klass) || !Tools::IsPtrValid(const_cast<char *>(klass->_1.name))) return nullptr;
    return klass->_1.name;
}

//-- Placed devices: class-based match, never labelled enemy from the class name.
inline DeviceKind Identify(const char *name) {
    if (!name) return DeviceKind::None;
    if (!std::strcmp(name, "WeaponProjectile_TripMine")) return DeviceKind::TripMine;
    if (!std::strcmp(name, "WeaponProjectile_C4bomb") ||
        !std::strcmp(name, "WeaponProjectile_TankBattleC4")) return DeviceKind::C4;
    if (!std::strcmp(name, "WeaponProjectile_TacTripWire") ||
        !std::strcmp(name, "WeaponProjectile_ElectricTripWire")) return DeviceKind::Sensor;
    if (!std::strcmp(name, "WeaponProjectile_TrophySystem")) return DeviceKind::Trophy;
    if (!std::strcmp(name, "WeaponProjectile_TankMine") ||
        !std::strcmp(name, "WeaponProjectile_TrapChipMine")) return DeviceKind::Mine;
    if (!std::strcmp(name, "WeaponProjectile_DrillCharge")) return DeviceKind::Drill;
    return DeviceKind::None;
}

inline bool DeviceEnabled(DeviceKind kind) {
    switch (kind) {
        case DeviceKind::TripMine: return Config.ESPMenu.PlacedTripMines;
        case DeviceKind::C4: return Config.ESPMenu.PlacedC4;
        case DeviceKind::Sensor: return Config.ESPMenu.PlacedSensors;
        case DeviceKind::Trophy: return Config.ESPMenu.PlacedTrophies;
        default: return Config.ESPMenu.PlacedOther;
    }
}

inline const char *DeviceLabel(DeviceKind kind) {
    switch (kind) {
        case DeviceKind::TripMine: return "TRIP MINE";
        case DeviceKind::C4: return "C4";
        case DeviceKind::Sensor: return "TRIPWIRE / SENSOR";
        case DeviceKind::Trophy: return "TROPHY SYSTEM";
        case DeviceKind::Mine: return "PLACED MINE";
        case DeviceKind::Drill: return "DRILL CHARGE";
        default: return "DEVICE";
    }
}

inline uint32_t PawnTeam(void *pawn) {
    if (!Tools::IsPtrValid(pawn)) return 0;
    using Team = uint32_t (*)(void *);
    static Team fn = reinterpret_cast<Team>(getAbsoluteAddress("libunity.so", 0x538A758)); // Pawn.get_TeamId
    return fn ? fn(pawn) : 0;
}

inline uintptr_t NameGetter(const char *name) {
    if (!name) return 0;
    if (!std::strcmp(name, "BRWeaponConfig")) return 0xA304494;     // get_Name
    if (!std::strcmp(name, "BRClipConfig")) return 0xA2F30E0;       // GetName
    if (!std::strcmp(name, "BREquipConfig")) return 0xA2F883C;      // GetName
    if (!std::strcmp(name, "BREquipPartConfig")) return 0xA2F9E5C;  // GetName
    if (!std::strcmp(name, "BRMedicineConfig")) return 0xA2FD028;   // GetName
    if (!std::strcmp(name, "BRThrowingConfig")) return 0xA303500;   // GetName
    if (!std::strcmp(name, "BRSkillItemConfig")) return 0xA301CFC;  // GetName
    if (!std::strcmp(name, "BRPerkConfig")) return 0xA2FE438;       // GetName
    return 0;
}

inline void CopyItemName(void *instance, char (&buffer)[64]) {
    buffer[0] = 0;
    void *config = *reinterpret_cast<void **>(reinterpret_cast<uintptr_t>(instance) + LootField::Config);
    if (!Tools::IsPtrValid(config)) return;
    const uintptr_t rva = NameGetter(ClassName(config));
    if (!rva) return;
    using GetName = String *(*)(void *);
    const auto fn = reinterpret_cast<GetName>(getAbsoluteAddress("libunity.so", rva));
    if (!fn) return;
    String *result = fn(config);
    if (!Tools::IsPtrValid(result) || result->length <= 0 || result->length > 60) return;
    const uint16_t *chars = &result->start_char;
    size_t written = 0;
    for (int i = 0; i < result->length && written + 1 < sizeof(buffer); ++i) {
        const uint16_t ch = chars[i];
        if (ch >= 32 && ch <= 126) {
            buffer[written++] = static_cast<char>(ch);
        } else if (ch >= 128 && ch < 2048 && written + 2 < sizeof(buffer)) {
            buffer[written++] = static_cast<char>(0xC0 | (ch >> 6));
            buffer[written++] = static_cast<char>(0x80 | (ch & 0x3F));
        }
    }
    buffer[written] = 0;
}

inline bool SearchMatches(const char *haystack, const char *needle) {
    if (!needle || !*needle) return true;
    if (!haystack) return false;
    for (const char *start = haystack; *start; ++start) {
        const char *a = start;
        const char *b = needle;
        while (*a && *b) {
            const unsigned char ax = static_cast<unsigned char>(*a);
            const unsigned char bx = static_cast<unsigned char>(*b);
            const char ca = ax >= 'A' && ax <= 'Z' ? static_cast<char>(ax + 32) : static_cast<char>(ax);
            const char cb = bx >= 'A' && bx <= 'Z' ? static_cast<char>(bx + 32) : static_cast<char>(bx);
            if (ca != cb) break;
            ++a;
            ++b;
        }
        if (!*b) return true;
    }
    return false;
}

//-- EBRDroppedPickUpType: 1 Weapon, 2 Clip, 3 Equip, 4 EquipPart, 5 Medicine,
//-- 6 Throwing, 7 SkillItem, 8 Perk, 9+ reserved/event items.
inline const char *ItemCategory(int type) {
    switch (type) {
        case 1: return "WEAPON";
        case 2: case 9: return "AMMO";
        case 3: return "EQUIPMENT";
        case 4: return "MOD";
        case 5: return "MEDICAL";
        case 6: return "THROWABLE";
        case 7: return "SKILL";
        case 8: return "PERK";
        default: return "ITEM";
    }
}

inline bool ItemEnabled(int type) {
    if (!Config.ESPMenu.ItemESP) return false;
    switch (type) {
        case 1: return Config.ESPMenu.ItemWeapons;
        case 2: case 9: return Config.ESPMenu.ItemAmmo;
        case 3: return Config.ESPMenu.ItemEquipment;
        case 4: return Config.ESPMenu.ItemMods;
        default: return Config.ESPMenu.ItemOther;
    }
}

inline void TrackLoot(void *instance) {
    if (!wantLoot.load(std::memory_order_relaxed) || !Tools::IsPtrValid(instance)) return;
    void *data = *reinterpret_cast<void **>(reinterpret_cast<uintptr_t>(instance) + LootField::ItemData);
    if (!Tools::IsPtrValid(data)) return;
    const auto *bytes = reinterpret_cast<const uint8_t *>(data);
    Loot entry;
    std::memcpy(&entry.kind, bytes + LootField::ItemType, 4);
    std::memcpy(&entry.item, bytes + LootField::ItemId, 4);
    std::memcpy(&entry.count, bytes + LootField::Count, 4);
    std::memcpy(&entry.pos, bytes + LootField::Position, sizeof(Vector3));
    if (bytes[LootField::IsInBox] != 0 || entry.kind <= 0 || entry.kind > 21 ||
        entry.item <= 0 || !ValidWorld(entry.pos)) return;

    const uintptr_t key = reinterpret_cast<uintptr_t>(instance);
    {
        std::lock_guard<std::mutex> lock(mutex);
        const auto existing = lootEntries.find(key);
        if (existing != lootEntries.end()) {
            std::memcpy(entry.name, existing->second.name, sizeof(entry.name));
            entry.nameTries = existing->second.nameTries;
        }
    }
    if (!entry.name[0] && entry.nameTries < 3) {
        ++entry.nameTries;
        CopyItemName(instance, entry.name);
    }
    entry.seen = Clock::now();
    std::lock_guard<std::mutex> lock(mutex);
    if (lootEntries.size() >= 400 && lootEntries.find(key) == lootEntries.end()) return;
    lootEntries[key] = entry;
}

inline void TrackPlaced(void *instance) {
    if (!wantDevices.load(std::memory_order_relaxed) || !Tools::IsPtrValid(instance)) return;
    const auto kind = Identify(ClassName(instance));
    if (kind == DeviceKind::None) return;
    const uintptr_t address = reinterpret_cast<uintptr_t>(instance);
    const auto *raw = reinterpret_cast<const uint8_t *>(instance);
    if (raw[ProjectileField::Exploded] || raw[ProjectileField::Destroyed]) {
        std::lock_guard<std::mutex> lock(mutex);
        deviceEntries.erase(address);
        return;
    }
    const Vector3 pos = GetThrowPos(instance);
    if (!ValidWorld(pos) || pos == Vector3::zero()) return;
    void *owner = nullptr;
    std::memcpy(&owner, raw + ProjectileField::Owner, sizeof(owner));
    Device info;
    info.kind = kind;
    info.pos = pos;
    info.ownerTeam = PawnTeam(owner);
    info.seen = Clock::now();
    std::lock_guard<std::mutex> lock(mutex);
    if (deviceEntries.size() >= 250 && deviceEntries.find(address) == deviceEntries.end()) return;
    deviceEntries[address] = info;
}

inline void HookLootTick(void *instance, float dt) {
    TrackLoot(instance);
    if (origLootTick) origLootTick(instance, dt);
}

inline void HookBRPickupTick(void *instance, float dt) {
    TrackLoot(instance);
    if (origBRPickupTick) origBRPickupTick(instance, dt);
}

inline void HookStickTick(void *instance, float dt) {
    TrackPlaced(instance);
    if (origStickTick) origStickTick(instance, dt);
}

inline void HookTripMineTick(void *instance, float dt) {
    TrackPlaced(instance);
    if (origTripMineTick) origTripMineTick(instance, dt);
}

inline void HookTripWireTick(void *instance, float dt) {
    TrackPlaced(instance);
    if (origTripWireTick) origTripWireTick(instance, dt);
}

inline void HookTrapChipTick(void *instance, float dt) {
    TrackPlaced(instance);
    if (origTrapChipTick) origTrapChipTick(instance, dt);
}

inline void HookPermeateTick(void *instance, float dt) {
    TrackPlaced(instance);
    if (origPermeateTick) origPermeateTick(instance, dt);
}

inline bool Projection(Camera *cam, const Vector3 &world, int width, int height, ImVec2 &screen) {
    if (!cam || !ValidWorld(world)) return false;
    Vector3 p = cam->WorldToScreenPoint(world);
    if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z) || p.z <= 0.01f) return false;
    screen = ImVec2(p.x, height - p.y);
    return screen.x >= -80.0f && screen.y >= -40.0f &&
        screen.x <= width + 80.0f && screen.y <= height + 40.0f;
}

inline void Draw(ImDrawList *draw, int width, int height, const Vector3 &localPosition, Pawn *localPawn) {
    wantLoot.store(Config.ESPMenu.ItemESP, std::memory_order_relaxed);
    wantDevices.store(Config.ESPMenu.PlacedNadesESP, std::memory_order_relaxed);
    if ((!Config.ESPMenu.ItemESP && !Config.ESPMenu.PlacedNadesESP) ||
        !draw || !ValidWorld(localPosition)) return;
    Camera *cam = Camera::get_main();
    if (!cam) return;

    const auto now = Clock::now();
    std::vector<Loot> nearbyLoot;
    std::vector<Device> nearbyDevices;
    {
        std::lock_guard<std::mutex> lock(mutex);
        for (auto it = lootEntries.begin(); it != lootEntries.end();) {
            if (now - it->second.seen > std::chrono::milliseconds(1800)) {
                it = lootEntries.erase(it);
            } else {
                if (Config.ESPMenu.ItemESP && ItemEnabled(it->second.kind) && nearbyLoot.size() < 120)
                    nearbyLoot.push_back(it->second);
                ++it;
            }
        }
        for (auto it = deviceEntries.begin(); it != deviceEntries.end();) {
            if (now - it->second.seen > std::chrono::seconds(3)) {
                it = deviceEntries.erase(it);
            } else {
                if (Config.ESPMenu.PlacedNadesESP && DeviceEnabled(it->second.kind) &&
                    nearbyDevices.size() < 80)
                    nearbyDevices.push_back(it->second);
                ++it;
            }
        }
    }

    const float maxLoot = std::clamp(Config.ESPMenu.ItemDistance, 20.0f, 250.0f);
    const float maxDevices = std::clamp(Config.ESPMenu.PlacedDistance, 10.0f, 120.0f);

    for (const auto &item : nearbyLoot) {
        const float distance = Vector3::Distance(localPosition, item.pos);
        if (!std::isfinite(distance) || distance > maxLoot) continue;
        ImVec2 screen;
        if (!Projection(cam, item.pos, width, height, screen)) continue;
        const ImU32 color = item.kind == 1 ? IM_COL32(255, 210, 78, 235) :
            item.kind == 2 || item.kind == 9 ? IM_COL32(100, 220, 255, 235) :
            item.kind == 4 ? IM_COL32(189, 144, 255, 235) :
            item.kind == 3 ? IM_COL32(150, 255, 170, 235) : IM_COL32(222, 233, 243, 235);
        char label[128];
        if (item.name[0])
            std::snprintf(label, sizeof(label), "%s | %.0fm", item.name, distance);
        else
            std::snprintf(label, sizeof(label), "%s #%d | %.0fm", ItemCategory(item.kind), item.item, distance);
        if (!SearchMatches(label, Config.ESPMenu.ItemSearch)) continue;
        draw->AddCircleFilled(screen, 3.5f, color);
        DrawTextWithBorder1(draw, label, {screen.x + 7.0f, screen.y - 12.0f}, color, IM_COL32(0, 0, 0, 240), 14.0f);
    }

    const uint32_t localTeam = PawnTeam(localPawn);
    for (const auto &entity : nearbyDevices) {
        //-- Unknown owners are still drawn; only a known friendly team is filtered.
        if (Config.ESPMenu.PlacedEnemyOnly && localTeam != 0 &&
            entity.ownerTeam != 0 && entity.ownerTeam == localTeam) continue;
        const float distance = Vector3::Distance(localPosition, entity.pos);
        if (!std::isfinite(distance) || distance > maxDevices) continue;
        ImVec2 screen;
        if (!Projection(cam, entity.pos, width, height, screen)) continue;
        const ImU32 color = entity.kind == DeviceKind::Trophy ? IM_COL32(255, 203, 92, 245) : IM_COL32(255, 83, 92, 245);
        draw->AddCircleFilled(screen, 4.0f, color);
        draw->AddCircle(screen, 11.0f, color, 0, 1.7f);
        char label[100];
        std::snprintf(label, sizeof(label), "%s | %.0fm", DeviceLabel(entity.kind), distance);
        DrawTextWithBorder1(draw, label, {screen.x + 14.0f, screen.y - 13.0f}, color, IM_COL32(0, 0, 0, 240), 15.0f);
    }
}

} // namespace world_esp
