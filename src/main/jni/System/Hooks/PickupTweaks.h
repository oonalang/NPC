#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <mutex>

namespace pickup_tweaks {

constexpr uintptr_t HorizontalDistanceSquare = 0x70;
constexpr uintptr_t VerticalDistance = 0x74;
constexpr float MinimumRange = 3.0f;
constexpr float MaximumRange = 15.0f;

inline std::atomic<bool> fastEnabled{false};
inline std::atomic<bool> rangeEnabled{false};
inline std::atomic<float> selectedRange{6.0f};
inline std::atomic<uint64_t> intervalCalls{0};
inline std::atomic<uint64_t> rangeWrites{0};

struct ManagerState {
    uintptr_t address = 0;
    float originalHorizontal = 0.0f;
    float originalVertical = 0.0f;
    float adjustedHorizontal = 0.0f;
    bool adjusted = false;
    bool baselineValid = false;
};

inline ManagerState managerState{};
inline std::mutex stateMutex;

inline void Publish(bool fast, bool extended, float range) {
    fastEnabled.store(fast, std::memory_order_relaxed);
    rangeEnabled.store(extended, std::memory_order_relaxed);
    selectedRange.store(std::isfinite(range) ? std::clamp(range, MinimumRange, MaximumRange) : 6.0f,
                        std::memory_order_relaxed);
}

inline float ReadFloat(uintptr_t object, uintptr_t offset) {
    float result = 0.0f;
    std::memcpy(&result, reinterpret_cast<const void*>(object + offset), sizeof(result));
    return result;
}

inline void WriteFloat(uintptr_t object, uintptr_t offset, float value) {
    std::memcpy(reinterpret_cast<void*>(object + offset), &value, sizeof(value));
}

inline bool ValidNativeDistance(float horizontal, float vertical) {
    return std::isfinite(horizontal) && horizontal > 0.0f && horizontal <= 10000.0f &&
        std::isfinite(vertical) && vertical > 0.0f && vertical <= 150.0f;
}

inline bool SameDistance(float a, float b) {
    return std::fabs(a - b) <= std::max(0.0001f, std::fabs(b) * 0.00001f);
}

inline void BeginManager(uintptr_t address) {
    std::lock_guard<std::mutex> lock(stateMutex);
    managerState = ManagerState{};
    managerState.address = address;
}

inline void UpdateManager(void* instance) {
    if (!instance) return;
    const uintptr_t address = reinterpret_cast<uintptr_t>(instance);
    std::lock_guard<std::mutex> lock(stateMutex);

    if (managerState.address != address) {
        managerState = ManagerState{};
        managerState.address = address;
    }

    const float horizontal = ReadFloat(address, HorizontalDistanceSquare);
    const float vertical = ReadFloat(address, VerticalDistance);
    if (!ValidNativeDistance(horizontal, vertical)) return;

    if (managerState.adjusted && !SameDistance(horizontal, managerState.adjustedHorizontal)) {
        managerState.originalHorizontal = horizontal;
        managerState.originalVertical = vertical;
        managerState.adjusted = false;
    }
    if (!managerState.baselineValid) {
        managerState.originalHorizontal = horizontal;
        managerState.originalVertical = vertical;
        managerState.baselineValid = true;
    }

    if (!rangeEnabled.load(std::memory_order_relaxed)) {
        if (managerState.adjusted &&
            SameDistance(horizontal, managerState.adjustedHorizontal) &&
            ValidNativeDistance(managerState.originalHorizontal, managerState.originalVertical)) {
            WriteFloat(address, HorizontalDistanceSquare, managerState.originalHorizontal);
        }
        managerState.adjusted = false;
        return;
    }

    const float range = selectedRange.load(std::memory_order_relaxed);
    const float desiredSquare = range * range;
    const float next = std::max(managerState.originalHorizontal, desiredSquare);
    if (!SameDistance(horizontal, next)) {
        WriteFloat(address, HorizontalDistanceSquare, next);
        rangeWrites.fetch_add(1, std::memory_order_relaxed);
    }
    managerState.adjusted = !SameDistance(next, managerState.originalHorizontal);
    managerState.adjustedHorizontal = next;
}

inline float (*origAutoPickUpInterval)(void*) = nullptr;
inline float HookAutoPickUpInterval(void* instance) {
    if (!origAutoPickUpInterval) return 0.0f;
    const float normal = origAutoPickUpInterval(instance);
    intervalCalls.fetch_add(1, std::memory_order_relaxed);
    if (!fastEnabled.load(std::memory_order_relaxed) || !std::isfinite(normal) || normal <= 0.0f)
        return normal;
    return std::min(normal, std::max(0.04f, normal * 0.45f));
}

inline void (*origBRManagerTick)(void*, float) = nullptr;
inline void HookBRManagerTick(void* instance, float deltaTime) {
    if (origBRManagerTick) origBRManagerTick(instance, deltaTime);
    UpdateManager(instance);
}

inline void (*origInitMaxPickUpDistance)(void*) = nullptr;
inline void HookInitMaxPickUpDistance(void* instance) {
    if (origInitMaxPickUpDistance) origInitMaxPickUpDistance(instance);
    if (instance) {
        BeginManager(reinterpret_cast<uintptr_t>(instance));
        UpdateManager(instance);
    }
}

}
