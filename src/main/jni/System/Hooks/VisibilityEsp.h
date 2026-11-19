#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <unordered_map>

namespace visibility_esp {

using Clock = std::chrono::steady_clock;

struct Record {
    bool blocked = false;
    Clock::time_point checked{};
    Clock::time_point observed{};
};

inline std::unordered_map<uintptr_t, Record> records;
inline Clock::time_point frameTime{};
inline Vector3 viewOrigin = Vector3::zero();
inline bool frameReady = false;
inline unsigned remainingChecks = 0;
inline unsigned frameCount = 0;
inline uintptr_t activeMatch = 0;

inline bool Finite(const Vector3& point) {
    return std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z);
}

inline void BeginFrame(const void* match) {
    frameReady = false;
    remainingChecks = 3;
    const uintptr_t current = reinterpret_cast<uintptr_t>(match);
    if (activeMatch != current) {
        records.clear();
        activeMatch = current;
    }
    if (!Config.ESPMenu.VisibilityCheck) {
        if (!records.empty()) records.clear();
        return;
    }

    frameTime = Clock::now();
    Camera* camera = Camera::get_main();
    if (!camera) return;
    Transform* transform = camera->get_transform();
    if (!Tools::IsPtrValid(transform)) return;
    viewOrigin = transform->get_position();
    frameReady = Finite(viewOrigin);

    if ((++frameCount % 120) == 0) {
        for (auto it = records.begin(); it != records.end();) {
            if (frameTime - it->second.observed > std::chrono::seconds(3))
                it = records.erase(it);
            else
                ++it;
        }
    }
}

inline bool IsBlocked(Pawn* pawn, const Vector3& head) {
    if (!frameReady || !Tools::IsPtrValid(pawn) || !Finite(head)) return false;

    const uintptr_t key = reinterpret_cast<uintptr_t>(pawn);
    auto existing = records.find(key);
    if (existing != records.end()) existing->second.observed = frameTime;

    const float dx = head.x - viewOrigin.x;
    const float dy = head.y - viewOrigin.y;
    const float dz = head.z - viewOrigin.z;
    const float squared = dx * dx + dy * dy + dz * dz;
    if (!std::isfinite(squared) || squared < 0.01f || squared > 220.0f * 220.0f)
        return false;

    if (existing != records.end()) {
        const auto age = frameTime - existing->second.checked;
        const auto interval = existing->second.blocked
            ? std::chrono::milliseconds(190)
            : std::chrono::milliseconds(130);
        if (age < interval) return existing->second.blocked;
    }
    if (remainingChecks == 0) {
        return existing != records.end() &&
            frameTime - existing->second.checked < std::chrono::milliseconds(650) &&
            existing->second.blocked;
    }

    using WorldTrace = bool (*)(void*, Vector3, Vector3, float, float*, int, const void*);
    static WorldTrace trace = reinterpret_cast<WorldTrace>(
        getAbsoluteAddress("libunity.so", 0x66F2708));
    if (!trace) return false;

    const float distance = std::sqrt(squared);
    const float inv = 1.0f / distance;
    const Vector3 direction(dx * inv, dy * inv, dz * inv);
    float blocker = -1.0f;
    --remainingChecks;
    const bool hit = trace(nullptr, viewOrigin, direction, distance, &blocker, 0, nullptr);
    if (hit && (!std::isfinite(blocker) || blocker < 0.0f)) {
        return existing != records.end() && existing->second.blocked;
    }
    const float tolerance = std::max(0.001f, distance * 0.00005f);
    const bool blocked = hit && blocker + tolerance < distance;
    if (records.size() < 256 || existing != records.end()) {
        records[key] = Record{blocked, frameTime, frameTime};
    }
    return blocked;
}

}
