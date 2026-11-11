#pragma once
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <unordered_map>
#include "BlueprintTierFilter.h"

namespace BlueprintEquipment {

struct EquippedBlueprint {
    uint32_t blueprintId;
    int diyIndex;
};

inline std::mutex equippedMutex;
inline std::unordered_map<uint32_t, EquippedBlueprint> equipped;
inline std::atomic<uint32_t> pendingId{0};
inline std::atomic<int> pendingDiy{-1};
inline std::atomic<void*> lastController{nullptr};
inline std::atomic<int> lastClickedIndex{-1};
inline std::atomic<bool> installStarted{false};

using ButtonFn = void (*)(void*, void*);
using EquipFn = void (*)(void*, int, void*);
using ClickFn = bool (*)(void*, void*, void*, int, int, void*);
using EquippedFn = bool (*)(void*, uint32_t, uint32_t, int, void*);
using InfoEquippedFn = bool (*)(void*, void*, void*);

inline ButtonFn originalTemplateEquip = nullptr;
inline EquipFn originalEquip = nullptr;
inline ClickFn originalClick = nullptr;
inline EquippedFn originalIsWeaponEquipped = nullptr;
inline InfoEquippedFn originalGunsmithViewEquipped = nullptr;
inline InfoEquippedFn originalQuicklySelectEquipped = nullptr;
inline InfoEquippedFn originalQuickItemEquipped = nullptr;

template <typename T>
inline bool Read(const void* at, T& value) {
    if (!at || !Tools::IsPtrValid(const_cast<void*>(at))) return false;
    std::memcpy(&value, at, sizeof(value));
    return true;
}

inline void RefreshTierCatalog() {
    if (!loadskinhack || BlueprintTierFilter::catalogReady.load(std::memory_order_acquire)) return;
    BlueprintTierFilter::TierCatalog updated;
    updated.reserve(itemData.size());
    for (const auto& item : itemData) {
        if (item.WeaponConf[0] <= 0 || item.WeaponConf[2] <= 0) continue;
        const auto blueprintId = static_cast<uint32_t>(item.WeaponConf[2]);
        updated[blueprintId] |= BlueprintTierFilter::DetectTier(item.itemName);
    }
    BlueprintTierFilter::Publish(std::move(updated));
}

inline bool CatalogContains(uint32_t blueprintId) {
    return loadskinhack && BlueprintTierFilter::Contains(blueprintId) &&
           BlueprintTierFilter::Allows(blueprintId);
}

inline bool SelectBlueprint(void* controller, uint32_t& blueprintId, int& diyIndex) {
    blueprintId = 0;
    diyIndex = -1;
    if (!controller || !Tools::IsPtrValid(controller)) return false;
    auto* fields = static_cast<const unsigned char*>(controller);
    void* easy = nullptr;
    void* list = nullptr;
    if (!Read(fields + 0x1C8, easy) || !Read(fields + 0x1D0, list)) return false;
    if (!easy || !list || !Tools::IsPtrValid(easy) || !Tools::IsPtrValid(list)) return false;
    int index = -1;
    if (!Read(static_cast<unsigned char*>(easy) + 0x108, index)) return false;
    if (lastController.load(std::memory_order_acquire) == controller)
        index = lastClickedIndex.load(std::memory_order_acquire);
    void* items = nullptr;
    int count = 0;
    auto* listFields = static_cast<unsigned char*>(list);
    if (!Read(listFields + 0x10, items) || !Read(listFields + 0x18, count)) return false;
    if (!items || !Tools::IsPtrValid(items) || count < 1 || count > 4096 ||
        index < 0 || index >= count) return false;
    size_t capacity = 0;
    auto* array = static_cast<unsigned char*>(items);
    if (!Read(array + 0x18, capacity) || capacity > 4096 ||
        static_cast<size_t>(index) >= capacity) return false;
    void* entry = nullptr;
    if (!Read(array + 0x20 + static_cast<size_t>(index) * sizeof(entry), entry) ||
        !entry || !Tools::IsPtrValid(entry)) return false;
    auto* entryFields = static_cast<unsigned char*>(entry);
    uint32_t baseId = 0;
    if (!Read(entryFields + 0x20, baseId) ||
        !Read(entryFields + 0x24, blueprintId) ||
        !Read(entryFields + 0x28, diyIndex)) return false;
    return baseId > 0 && blueprintId > 0 && baseId != blueprintId;
}

inline bool QueueSelected(void* controller) {
    if (!Config.ExtraMenu.Blueprints) return false;
    uint32_t id = 0;
    int diy = -1;
    if (!SelectBlueprint(controller, id, diy) || !CatalogContains(id)) return false;
    pendingDiy.store(diy, std::memory_order_relaxed);
    pendingId.store(id, std::memory_order_release);
    return true;
}

inline void OnTemplateEquip(void* controller, void* method) {
    if (QueueSelected(controller)) return;
    if (originalTemplateEquip) originalTemplateEquip(controller, method);
}

inline void OnEquip(void* controller, int equipType, void* method) {
    if (QueueSelected(controller)) return;
    if (originalEquip) originalEquip(controller, equipType, method);
}

inline bool OnItemClick(void* controller, void* list, void* item,
                        int index, int context, void* method) {
    if (controller && list && index >= 0 && Tools::IsPtrValid(controller)) {
        void* weaponList = nullptr;
        if (Read(static_cast<unsigned char*>(controller) + 0x1C8, weaponList) &&
            weaponList == list) {
            lastController.store(controller, std::memory_order_release);
            lastClickedIndex.store(index, std::memory_order_release);
        }
    }
    return originalClick ? originalClick(controller, list, item, index, context, method) : false;
}

inline bool IsEquipped(void* controller, uint32_t baseId, uint32_t blueprintId,
                       int diyIndex, void* method) {
    if (Config.ExtraMenu.Blueprints && BlueprintTierFilter::Allows(blueprintId)) {
        std::lock_guard<std::mutex> lock(equippedMutex);
        const auto found = equipped.find(baseId);
        if (found != equipped.end())
            return found->second.blueprintId == blueprintId &&
                   (found->second.diyIndex < 0 || found->second.diyIndex == diyIndex);
    }
    return originalIsWeaponEquipped
        ? originalIsWeaponEquipped(controller, baseId, blueprintId, diyIndex, method)
        : false;
}

inline bool IsInfoEquipped(void* info, bool& result) {
    if (!Config.ExtraMenu.Blueprints || !info || !Tools::IsPtrValid(info)) return false;
    uint32_t baseId = 0;
    uint32_t blueprintId = 0;
    int diyIndex = -1;
    auto* fields = static_cast<unsigned char*>(info);
    if (!Read(fields + 0x20, baseId) || !Read(fields + 0x24, blueprintId) ||
        !Read(fields + 0x28, diyIndex)) return false;
    if (!BlueprintTierFilter::Allows(blueprintId)) return false;
    std::lock_guard<std::mutex> lock(equippedMutex);
    const auto found = equipped.find(baseId);
    if (found == equipped.end()) return false;
    result = found->second.blueprintId == blueprintId &&
             (found->second.diyIndex < 0 || found->second.diyIndex == diyIndex);
    return true;
}

inline bool IsGunsmithViewEquipped(void* view, void* info, void* method) {
    bool result = false;
    if (IsInfoEquipped(info, result)) return result;
    return originalGunsmithViewEquipped
        ? originalGunsmithViewEquipped(view, info, method) : false;
}

inline bool IsQuicklySelectEquipped(void* view, void* info, void* method) {
    bool result = false;
    if (IsInfoEquipped(info, result)) return result;
    return originalQuicklySelectEquipped
        ? originalQuicklySelectEquipped(view, info, method) : false;
}

inline bool IsQuickItemEquipped(void* view, void* info, void* method) {
    bool result = false;
    if (IsInfoEquipped(info, result)) return result;
    return originalQuickItemEquipped
        ? originalQuickItemEquipped(view, info, method) : false;
}

inline void ApplyPending() {
    if (!Config.ExtraMenu.Blueprints || !loadskinhack) return;
    RefreshTierCatalog();
    const uint32_t blueprintId = pendingId.exchange(0, std::memory_order_acq_rel);
    if (blueprintId == 0) return;
    const int diyIndex = pendingDiy.exchange(-1, std::memory_order_acq_rel);
    const auto found = std::find_if(itemData.begin(), itemData.end(), [blueprintId](const itemInfo& item) {
        return item.WeaponConf[0] > 0 &&
               static_cast<uint32_t>(item.WeaponConf[2]) == blueprintId;
    });
    if (found == itemData.end() || !BlueprintTierFilter::Allows(blueprintId)) return;
    for (const auto& item : itemData) {
        if (item.WeaponConf[0] == found->WeaponConf[0])
            sBool[item.itemName] = item.WeaponConf[2] == found->WeaponConf[2];
    }
    ApplyWeaponSkin(*found);
    std::lock_guard<std::mutex> lock(equippedMutex);
    equipped[static_cast<uint32_t>(found->WeaponConf[0])] = {blueprintId, diyIndex};
}

template <typename Fn>
inline bool HookAt(uintptr_t offset, void* replacement, Fn& original) {
    const uintptr_t address = getAbsoluteAddress("libunity.so", offset);
    if (!address || !Tools::IsPtrValid(reinterpret_cast<void*>(address))) return false;
    return DobbyHook(reinterpret_cast<void*>(address), replacement,
                     reinterpret_cast<void**>(&original)) == 0;
}

inline void Install() {
    if (installStarted.exchange(true, std::memory_order_acq_rel)) return;
    HookAt(0x654BC98, reinterpret_cast<void*>(OnTemplateEquip), originalTemplateEquip);
    HookAt(0x654BDC4, reinterpret_cast<void*>(OnEquip), originalEquip);
    HookAt(0x654FF2C, reinterpret_cast<void*>(OnItemClick), originalClick);
    HookAt(0x654A168, reinterpret_cast<void*>(IsEquipped), originalIsWeaponEquipped);
    HookAt(0x5A3D0A0, reinterpret_cast<void*>(IsGunsmithViewEquipped), originalGunsmithViewEquipped);
    HookAt(0x656D058, reinterpret_cast<void*>(IsQuicklySelectEquipped), originalQuicklySelectEquipped);
    HookAt(0x62DEC1C, reinterpret_cast<void*>(IsQuickItemEquipped), originalQuickItemEquipped);
}
}
