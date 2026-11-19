#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace ThrowAlertCfg {
    constexpr uintptr_t PosRva = 0xB4724B4;
    constexpr uintptr_t TickRva = 0xB48214C;
    constexpr uintptr_t Exploded = 0xA1;
    constexpr uintptr_t DamageRadius = 0xA8;
    constexpr uintptr_t ItemId = 0xE4;
    constexpr uintptr_t LethalRadius = 0x200;
    constexpr float MaxDist = 70.0f;
    constexpr float WarnDist = 35.0f;
    constexpr size_t MaxTracked = 96;
    constexpr size_t MaxDrawn = 12;
    constexpr size_t MaxTrailPoints = 24;
}

struct ThrowInfo {
    std::vector<Vector3> path;
    Vector3 pos;
    float radius = 0.0f;
    int itemId = 0;
    std::chrono::steady_clock::time_point lastSeen{};
};

inline void (*orig_WeaponProjectileTick)(void*, float) = nullptr;
inline std::unordered_map<uintptr_t, ThrowInfo> g_Throws;
inline std::mutex g_ThrowMutex;

inline bool ThrowAlertOn() {
    return Config.ESPMenu.GrenadeWarn;
}

inline const char* ThrowName(int itemId) {
    return itemId > 0 ? "THROWABLE" : "PROJECTILE";
}

inline Vector3 GetThrowPos(void* proj) {
    using Fn = Vector3 (*)(void*);
    static Fn fn = (Fn)getAbsoluteAddress("libunity.so", ThrowAlertCfg::PosRva);
    return fn ? fn(proj) : Vector3::zero();
}

inline void TrackThrowable(void* proj) {
    if (!ThrowAlertOn() || !Tools::IsPtrValid(proj)) return;
    const uintptr_t key = reinterpret_cast<uintptr_t>(proj);
    const bool exploded = *reinterpret_cast<bool*>(key + ThrowAlertCfg::Exploded);
    if (exploded) {
        std::lock_guard<std::mutex> lock(g_ThrowMutex);
        g_Throws.erase(key);
        return;
    }

    float radius = *reinterpret_cast<float*>(key + ThrowAlertCfg::LethalRadius);
    if (!std::isfinite(radius) || radius <= 0.25f)
        radius = *reinterpret_cast<float*>(key + ThrowAlertCfg::DamageRadius);
    if (!std::isfinite(radius) || radius <= 0.25f || radius > 80.0f) return;

    const Vector3 pos = GetThrowPos(proj);
    if (!std::isfinite(pos.x) || !std::isfinite(pos.y) || !std::isfinite(pos.z) ||
        pos == Vector3::zero()) return;

    const int itemId = *reinterpret_cast<int*>(key + ThrowAlertCfg::ItemId);
    std::lock_guard<std::mutex> lock(g_ThrowMutex);
    const auto it = g_Throws.find(key);
    if (it == g_Throws.end() && g_Throws.size() >= ThrowAlertCfg::MaxTracked) return;
    ThrowInfo& info = g_Throws[key];
    info.pos = pos;
    info.radius = radius;
    info.itemId = itemId;
    info.lastSeen = std::chrono::steady_clock::now();

    if (info.path.empty() || Vector3::Distance(info.path.back(), pos) > 0.35f) {
        if (info.path.size() >= ThrowAlertCfg::MaxTrailPoints)
            info.path.erase(info.path.begin());
        info.path.push_back(pos);
    }
}

inline bool ThrowToScreen(Camera* cam, const Vector3& world, int height, ImVec2& out) {
    if (!cam) return false;
    const Vector3 screen = cam->WorldToScreenPoint(world);
    if (!std::isfinite(screen.x) || !std::isfinite(screen.y) ||
        !std::isfinite(screen.z) || screen.z <= 0.01f) return false;
    out = ImVec2(screen.x, height - screen.y);
    return true;
}

inline void DrawThrowRadius(ImDrawList* draw, Camera* cam,
                            const Vector3& center, float radius, int height,
                            bool urgent) {
    constexpr int segments = 30;
    std::vector<ImVec2> points;
    points.reserve(segments);
    const float r = ImClamp(radius, 1.0f, 18.0f);
    for (int i = 0; i < segments; ++i) {
        const float angle = (2.0f * IM_PI * i) / segments;
        ImVec2 screen;
        if (!ThrowToScreen(cam, Vector3(center.x + std::cos(angle) * r,
            center.y + 0.04f, center.z + std::sin(angle) * r), height, screen))
            return;
        points.push_back(screen);
    }

    const ImU32 line = urgent ? IM_COL32(255, 64, 64, 220) : IM_COL32(255, 191, 74, 185);
    const ImU32 fill = urgent ? IM_COL32(255, 64, 64, 35) : IM_COL32(255, 191, 74, 18);
    draw->AddConvexPolyFilled(points.data(), segments, fill);
    draw->AddPolyline(points.data(), segments, line, true, 1.8f);
}

inline void DrawThrowableAlert(ImDrawList* draw, int width, int height,
                              const Vector3& localPos) {
    if (!draw) return;
    if (!ThrowAlertOn()) {
        std::lock_guard<std::mutex> lock(g_ThrowMutex);
        if (!g_Throws.empty()) g_Throws.clear();
        return;
    }
    Camera* cam = Camera::get_main();
    if (!cam) return;

    const auto now = std::chrono::steady_clock::now();
    std::vector<ThrowInfo> nearby;
    {
        std::lock_guard<std::mutex> lock(g_ThrowMutex);
        for (auto it = g_Throws.begin(); it != g_Throws.end();) {
            if (now - it->second.lastSeen > std::chrono::seconds(3)) {
                it = g_Throws.erase(it);
                continue;
            }
            if (Vector3::Distance(localPos, it->second.pos) <= ThrowAlertCfg::MaxDist)
                nearby.push_back(it->second);
            ++it;
        }
    }
    std::sort(nearby.begin(), nearby.end(), [&](const ThrowInfo& a, const ThrowInfo& b) {
        return Vector3::Distance(localPos, a.pos) < Vector3::Distance(localPos, b.pos);
    });
    if (nearby.size() > ThrowAlertCfg::MaxDrawn)
        nearby.resize(ThrowAlertCfg::MaxDrawn);

    float closestDanger = ThrowAlertCfg::WarnDist + 1.0f;
    int urgentCount = 0;
    int warningCount = 0;
    for (const ThrowInfo& info : nearby) {
        const float distance = Vector3::Distance(localPos, info.pos);
        const bool urgent = distance <= std::max(12.0f, info.radius + 4.0f);
        if (distance <= ThrowAlertCfg::WarnDist) {
            ++warningCount;
            if (urgent) ++urgentCount;
            closestDanger = std::min(closestDanger, distance);
        }
        for (size_t i = 1; i < info.path.size(); ++i) {
            ImVec2 a, b;
            if (ThrowToScreen(cam, info.path[i - 1], height, a) &&
                ThrowToScreen(cam, info.path[i], height, b))
                draw->AddLine(a, b, IM_COL32(255, 255, 255, 160), 1.5f);
        }
        ImVec2 screen;
        if (!ThrowToScreen(cam, info.pos, height, screen)) continue;
        DrawThrowRadius(draw, cam, info.pos, info.radius, height, urgent);
        char label[92];
        std::snprintf(label, sizeof(label), "%s | %.0fm | R%.0fm",
                      ThrowName(info.itemId), distance, info.radius);
        const ImU32 color = urgent ? IM_COL32(255, 84, 84, 255) : IM_COL32(255, 206, 94, 255);
        DrawTextWithBorder1(draw, label, {screen.x - 45.0f, screen.y - 18.0f},
                            color, IM_COL32(0, 0, 0, 240), 16.0f);
    }

    if (warningCount > 0) {
        char alert[100];
        std::snprintf(alert, sizeof(alert), "%s | %.0fm | %d nearby",
                      urgentCount ? "DANGER: THROWABLE" : "THROWABLE NEARBY",
                      closestDanger, warningCount);
        const ImU32 color = urgentCount ? IM_COL32(255, 94, 94, 255) : IM_COL32(255, 221, 97, 255);
        DrawTextWithBorder1(draw, alert, {width * 0.5f - 120.0f, 125.0f},
                            color, IM_COL32(0, 0, 0, 255), 19.0f);
    }
}

namespace world_esp { inline void TrackPlaced(void*); }

inline void hook_WeaponProjectileTick(void* instance, float deltaTime) {
    world_esp::TrackPlaced(instance);
    if (orig_WeaponProjectileTick)
        orig_WeaponProjectileTick(instance, deltaTime);
    TrackThrowable(instance);
}
