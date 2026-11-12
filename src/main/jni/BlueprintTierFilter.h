#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace BlueprintTierFilter {

inline constexpr uint32_t Mythic = 1u << 0;
inline constexpr uint32_t Legendary = 1u << 1;
inline constexpr uint32_t Epic = 1u << 2;
inline constexpr uint32_t Common = 1u << 3;
inline constexpr uint32_t All = Mythic | Legendary | Epic | Common;

using TierCatalog = std::unordered_map<uint32_t, uint32_t>;

inline std::atomic<uint32_t> selectedMask{0};
inline std::atomic<bool> catalogReady{false};
inline std::shared_ptr<const TierCatalog> catalog;

inline uint32_t DetectTier(const std::string& name) {
    if (name.find("[MYTHIC]") != std::string::npos ||
        name.find("[M]") != std::string::npos) return Mythic;
    if (name.find("[LEGENDARY]") != std::string::npos ||
        name.find("[L]") != std::string::npos) return Legendary;
    if (name.find("[EPIC]") != std::string::npos ||
        name.find("[E]") != std::string::npos) return Epic;
    if (name.find("[COMMON]") != std::string::npos ||
        name.find("[C]") != std::string::npos) return Common;
    return 0;
}

inline uint32_t GetMask() {
    return selectedMask.load(std::memory_order_acquire);
}

inline void SetMask(uint32_t mask) {
    selectedMask.store(mask & All, std::memory_order_release);
}

inline void Toggle(uint32_t bit) {
    if ((bit & All) != 0) selectedMask.fetch_xor(bit & All, std::memory_order_acq_rel);
}

inline bool HasSelection(uint32_t bit) {
    return (GetMask() & bit) != 0;
}

inline bool Contains(uint32_t blueprintId) {
    if (blueprintId == 0 || !catalogReady.load(std::memory_order_acquire)) return false;
    const auto snapshot = std::atomic_load_explicit(&catalog, std::memory_order_acquire);
    return snapshot && snapshot->find(blueprintId) != snapshot->end();
}

inline bool Allows(uint32_t blueprintId) {
    const uint32_t mask = GetMask();
    if (mask == 0) return true;
    if (blueprintId == 0) return false;
    // Until the tier catalog has been built, a selected filter must not hide
    // everything; it only narrows blueprints once their tier is known.
    if (!catalogReady.load(std::memory_order_acquire)) return true;
    const auto snapshot = std::atomic_load_explicit(&catalog, std::memory_order_acquire);
    if (!snapshot) return true;
    const auto entry = snapshot->find(blueprintId);
    // Blueprints with no detected tier stay visible instead of vanishing.
    if (entry == snapshot->end() || entry->second == 0) return true;
    return (entry->second & mask) != 0;
}

inline void Publish(TierCatalog updated) {
    std::shared_ptr<const TierCatalog> snapshot =
        std::make_shared<const TierCatalog>(std::move(updated));
    std::atomic_store_explicit(&catalog, std::move(snapshot), std::memory_order_release);
    catalogReady.store(true, std::memory_order_release);
}

inline std::string Preview() {
    const uint32_t mask = GetMask();
    if (mask == 0) return "All blueprints";
    std::string result;
    if (mask & Mythic) result += "Mythic";
    if (mask & Legendary) result += result.empty() ? "Legendary" : ", Legendary";
    if (mask & Epic) result += result.empty() ? "Epic" : ", Epic";
    if (mask & Common) result += result.empty() ? "Common" : ", Common";
    return result;
}

}
