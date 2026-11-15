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
struct HitResult {
    Point location, normal;
    float distance;
    std::int32_t material;
    void* collider;
};
static_assert(sizeof(Point) == 12);
static_assert(offsetof(HitResult, distance) == 0x18);
static_assert(offsetof(HitResult, collider) == 0x20);
static_assert(sizeof(HitResult) == 0x28);

inline bool Finite(Point value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}
inline Point Around(Point point, Point center, float factor) noexcept {
    return {center.x + (point.x - center.x) * factor,
            center.y + (point.y - center.y) * factor,
            center.z + (point.z - center.z) * factor};
}
inline bool BodyGroup(unsigned group) noexcept {
    return group >= 1 && group <= 31 && group != 6;
}

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

template<class Original>
bool ScaledRay(Point center, Point start, Point end, float scale, HitResult& result, Original original) {
    scale = NormalizeScale(scale);
    if (scale == 1.f || !Finite(center) || !Finite(start) || !Finite(end))
        return original(start, end, result);
    const auto mappedStart = Around(start, center, 1.f / scale);
    const auto mappedEnd = Around(end, center, 1.f / scale);
    if (!Finite(mappedStart) || !Finite(mappedEnd)) return false;
    HitResult candidate = result;
    if (std::isfinite(candidate.distance)) candidate.distance /= scale;
    candidate.location = Around(candidate.location, center, 1.f / scale);
    if (!original(mappedStart, mappedEnd, candidate)) return false;
    candidate.location = Around(candidate.location, center, scale);
    const double dx = double(end.x) - start.x, dy = double(end.y) - start.y, dz = double(end.z) - start.z;
    const double lengthSquared = dx * dx + dy * dy + dz * dz;
    const double length = std::sqrt(lengthSquared);
    const double hx = double(candidate.location.x) - start.x;
    const double hy = double(candidate.location.y) - start.y;
    const double hz = double(candidate.location.z) - start.z;
    const double along = length > 0 ? (hx * dx + hy * dy + hz * dz) / length : -1;
    const double distanceSquared = hx * hx + hy * hy + hz * hz;
    const double tolerance = std::max(.0005, length * .00001);
    if (!Finite(candidate.location) || !Finite(candidate.normal) ||
        !std::isfinite(length) || length <= 0 || along < -tolerance || along > length + tolerance ||
        distanceSquared - along * along > tolerance * tolerance) return false;
    candidate.distance = static_cast<float>(std::sqrt(distanceSquared));
    if (!std::isfinite(candidate.distance)) return false;
    result = candidate;
    return true;
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
inline std::atomic<Status> status{Status::Waiting};
using HookInstaller = void(*)(void*, void*, void**);
bool Install(std::uintptr_t base, HookInstaller hook) noexcept;
}
