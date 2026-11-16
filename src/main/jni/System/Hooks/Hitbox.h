#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace hitbox {
inline constexpr float minimumScale = 1.f, maximumScale = 5.f, defaultScale = 1.f;
inline constexpr std::size_t maximumColliders = 128;
inline std::atomic<std::uint32_t> settings{2000};
inline std::atomic<bool> available{false};

inline float NormalizeScale(float value) noexcept {
    return std::isfinite(value) ? std::clamp(value, minimumScale, maximumScale) : defaultScale;
}
inline void Publish(bool enabled, float scale) noexcept {
    const auto units = static_cast<std::uint32_t>(std::lround(NormalizeScale(scale) * 1000.f));
    settings.store((units << 1) | (enabled ? 1u : 0u), std::memory_order_release);
}
struct Options { bool enabled; float scale; };
inline Options Read() noexcept {
    const auto value = settings.load(std::memory_order_acquire);
    return {bool(value & 1u), NormalizeScale(float(value >> 1) * .001f)};
}

struct Point { float x, y, z; };
struct Rotation { float x, y, z, w; };
struct HitResult {
    Point location, normal;
    float distance;
    std::int32_t material;
    void* collider;
};
static_assert(sizeof(Point) == 12);
static_assert(sizeof(Rotation) == 16);
static_assert(offsetof(HitResult, distance) == 0x18);
static_assert(offsetof(HitResult, collider) == 0x20);
static_assert(sizeof(HitResult) == 0x28);

inline bool BodyGroup(unsigned group) noexcept {
    return group >= 1 && group <= 31 && group != 6;
}
inline bool ExpandCapsule(float scale, float& axisLength, float& radius) noexcept {
    scale = NormalizeScale(scale);
    if (scale <= 1.f || !std::isfinite(axisLength) || !std::isfinite(radius) || axisLength < 0.f || radius <= 0.f)
        return false;
    const float expandedAxis = axisLength * scale, expandedRadius = radius * scale;
    if (!std::isfinite(expandedAxis) || !std::isfinite(expandedRadius)) return false;
    axisLength = expandedAxis;
    radius = expandedRadius;
    return true;
}
inline bool ExpandSphere(float scale, float& radius) noexcept {
    scale = NormalizeScale(scale);
    if (scale <= 1.f || !std::isfinite(radius) || radius <= 0.f) return false;
    const float expanded = radius * scale;
    if (!std::isfinite(expanded)) return false;
    radius = expanded;
    return true;
}
inline bool ExpandExtents(float scale, Point& extent) noexcept {
    scale = NormalizeScale(scale);
    if (scale <= 1.f || !std::isfinite(extent.x) || !std::isfinite(extent.y) || !std::isfinite(extent.z) ||
        extent.x < 0.f || extent.y < 0.f || extent.z < 0.f) return false;
    const Point expanded{extent.x*scale,extent.y*scale,extent.z*scale};
    if (!std::isfinite(expanded.x) || !std::isfinite(expanded.y) || !std::isfinite(expanded.z)) return false;
    extent = expanded;
    return true;
}
inline thread_local float primitiveScale = 1.f;
class PrimitiveScope {
    float previous_;
public:
    explicit PrimitiveScope(float scale) noexcept : previous_(primitiveScale) { primitiveScale = NormalizeScale(scale); }
    PrimitiveScope(const PrimitiveScope&) = delete;
    PrimitiveScope& operator=(const PrimitiveScope&) = delete;
    ~PrimitiveScope() { primitiveScale = previous_; }
};

struct TraceContext {
    std::array<const void*, maximumColliders> colliders{};
    std::size_t count = 0;
    float scale = 1.f;
    bool Contains(const void* collider) const noexcept {
        return collider && count <= colliders.size() &&
            std::find(colliders.begin(), colliders.begin() + count, collider) != colliders.begin() + count;
    }
};
inline thread_local const TraceContext* currentTrace = nullptr;
class TraceScope {
    const TraceContext* previous_;
public:
    explicit TraceScope(const TraceContext& context) noexcept : previous_(currentTrace) { currentTrace = &context; }
    TraceScope(const TraceScope&) = delete;
    TraceScope& operator=(const TraceScope&) = delete;
    ~TraceScope() { currentTrace = previous_; }
};

template<class Original>
bool TraceWeapon(TraceContext context, Original original) {
    if (currentTrace || !context.count || context.count > maximumColliders ||
        NormalizeScale(context.scale) <= 1.f) return original();
    const float expandedScale = NormalizeScale(context.scale);
    context.scale = 1.f;
    TraceScope scope(context);
    if (original()) return true;
    context.scale = expandedScale;
    return original();
}

template<class Value> Value Field(const void* object, std::size_t offset) noexcept {
    Value value{};
    if (object) std::memcpy(&value, static_cast<const unsigned char*>(object) + offset, sizeof(value));
    return value;
}
inline bool CopyList(const void* list, const void** output, std::size_t capacity, std::size_t& count) noexcept {
    count = 0;
    if (!list || !output || !capacity) return false;
    const int size = Field<int>(list, 0x18);
    const auto* items = Field<const void*>(list, 0x10);
    if (size <= 0 || std::size_t(size) > capacity || !items) return false;
    const auto length = Field<std::uint64_t>(items, 0x18);
    if (length < std::uint64_t(size) || length > 4096) return false;
    for (int i = 0; i < size; ++i)
        output[i] = Field<const void*>(items, 0x20 + std::size_t(i) * sizeof(void*));
    count = std::size_t(size);
    return true;
}

enum class Status { Waiting, Ready, Unsupported, HookFailed };
inline std::atomic<Status> status{Status::Waiting}, sphereStatus{Status::Waiting}, obbStatus{Status::Waiting};
inline std::atomic<bool> sphereAvailable{false}, obbAvailable{false};
inline bool NeedsProbe() noexcept {
    return !available.load(std::memory_order_acquire) ? status.load() == Status::Waiting :
        (!sphereAvailable.load(std::memory_order_acquire) && sphereStatus.load() == Status::Waiting) ||
        (!obbAvailable.load(std::memory_order_acquire) && obbStatus.load() == Status::Waiting);
}
using HookInstaller = void(*)(void*, void*, void**);
bool Install(std::uintptr_t base, HookInstaller hook) noexcept;
}
