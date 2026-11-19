#include "NativeQueries.h"
#include "NativeProfile.h"
#include "NativeRay.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace PhantomTrigger {
namespace {
using Address = std::uintptr_t;
using Object = void*;
using Method = const void*;


template<class Function>
Function Bind(Address base, Address rva) noexcept {
    return reinterpret_cast<Function>(base + rva);
}

template<class Value>
Value Read(Address object, std::size_t offset) noexcept {
    Value value{};
    if (object && offset <= std::numeric_limits<Address>::max() - object &&
        sizeof(value) <= std::numeric_limits<Address>::max() - object - offset)
        std::memcpy(&value, reinterpret_cast<const void*>(object + offset), sizeof(value));
    return value;
}

struct NativeCamera {
    Address base;
    Object object;
    explicit NativeCamera(Address library) noexcept : base(library),
        object(base ? Bind<Object(*)(Object, Method)>(base,0x52D5CDC)(nullptr,nullptr) : nullptr) {}
    bool IsValid() const noexcept { return object != nullptr; }
    float NearPlane() const noexcept {
        return Bind<float(*)(Object, Method)>(base,0x52D3C60)(object,nullptr);
    }
    float FarPlane() const noexcept {
        return Bind<float(*)(Object, Method)>(base,0x52D3CA8)(object,nullptr);
    }
    NativeCameraRay ViewportRay(Point3 center) const noexcept {
        return Bind<NativeCameraRay(*)(Object, Point3, Method)>(base,0x52D5B44)(object,center,nullptr);
    }
    int Width() const noexcept {
        return Bind<int(*)(Object, Method)>(base,0x530005C)(nullptr,nullptr);
    }
    int Height() const noexcept {
        return Bind<int(*)(Object, Method)>(base,0x53000C0)(nullptr,nullptr);
    }
    Point3 ScreenPoint(Point3 center) const noexcept {
        return Bind<Point3(*)(Object, Point3, Method)>(base,0x52D5AC8)(object,center,nullptr);
    }
};

constexpr int LocalBulletTrace = 0x800A;
constexpr std::size_t MaximumPawnCount = 512;

bool Finite(const Point3& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}



bool EnemyMember(Address match, Address pawn) noexcept {
    if (!match || !pawn) return false;
    const Address list = Read<Address>(match, 0x178);
    if (!list) return false;
    const int count = Read<int>(list, 0x18);
    const Address items = Read<Address>(list, 0x10);
    if (count <= 0 || static_cast<std::size_t>(count) > MaximumPawnCount || !items)
        return false;
    const auto capacity = Read<std::uint64_t>(items, 0x18);
    if (capacity < static_cast<std::uint64_t>(count) || capacity > 4096) return false;
    for (int i = 0; i < count; ++i) {
        if (Read<Address>(items, 0x20 + static_cast<std::size_t>(i) * sizeof(Address)) == pawn)
            return true;
    }
    return false;
}

struct OwnedImpact {
    Address base;
    Object object;
    OwnedImpact(Address library, Object value) noexcept : base(library), object(value) {}
    OwnedImpact(const OwnedImpact&) = delete;
    OwnedImpact& operator=(const OwnedImpact&) = delete;
    ~OwnedImpact() noexcept {
        if (object)
            Bind<void(*)(Object, Object, Method)>(base, 0x6258AA0)(nullptr, object, nullptr);
    }
};

bool BodyGroup(int group, bool avatar) noexcept {
    return group != 6 && group != 32 &&
           ((group >= 1 && group <= 31) || (group == 0 && avatar));
}

bool BulletIgnorable(Address base, const OwnedImpact& impact) noexcept {
    if (!base || !impact.object) return false;
    const Address material = Read<Address>(reinterpret_cast<Address>(impact.object), 0x78);
    if (!material) return false;
    constexpr Address CanIgnoreForBullet = 0x66F8D74;
    if (!IsVerifiedExecutableAddress(base + CanIgnoreForBullet)) return false;
    using Predicate = bool(*)(Object, Method);
    return Bind<Predicate>(base, CanIgnoreForBullet)(reinterpret_cast<Object>(material), nullptr);
}

Point3 AdvancePoint(const ViewRay& ray, float distance) noexcept {
    return {
        ray.origin.x + ray.direction.x * distance,
        ray.origin.y + ray.direction.y * distance,
        ray.origin.z + ray.direction.z * distance
    };
}
}

bool BuildNativeRay(Address base, ViewRay& ray,
                    float& normalizedX, float& normalizedY, bool sampleViewport) noexcept {
    NativeCamera camera(base);
    return SampleNativeViewRay(camera,ray,normalizedX,normalizedY,sampleViewport);
}

bool NativeTargetQueries::TraceBody(const ContextIdentity& context,
                                   const ViewRay& ray, BodyHit& result) noexcept {
    result = {};
    if (!base_ || !match_ || !pawn_ || context.match == 0 ||
        !Finite(ray.origin) || !Finite(ray.direction) ||
        !std::isfinite(ray.maxDistance) || ray.maxDistance <= 0) return false;
    OwnedImpact impact{base_, Bind<Object(*)(Object, Method)>(base_, 0x62588E4)(nullptr, nullptr)};
    if (!impact.object) return false;
    using BroadTrace = bool(*)(Object, Point3, Point3, float, int, Object, int, Method);
    Point3 traceOrigin = ray.origin;
    float advanced = 0.0f;
    float firstSurface = -1.0f;
    Address collider = 0;
    for (int restarts = 0; ; ++restarts) {
        const float remaining = ray.maxDistance - advanced;
        if (remaining <= 0.0f) return true;
        const bool broadHit = Bind<BroadTrace>(base_, 0x66F17EC)(
            nullptr, traceOrigin, ray.direction, remaining, LocalBulletTrace,
            impact.object, 0, nullptr);
        if (!broadHit) return true;
        collider = Read<Address>(reinterpret_cast<Address>(impact.object), 0x18);
        if (!collider) return false;
        if (!BulletIgnorable(base_, impact)) break;
        if (restarts >= MaxPassthroughRestarts) return true;
        const float impactDistance = Read<float>(reinterpret_cast<Address>(impact.object), 0x5C);
        if (!std::isfinite(impactDistance) || impactDistance < 0.0f ||
            impactDistance >= remaining) return false;
        const float surfaceDistance = advanced + impactDistance;
        if (firstSurface < 0.0f) firstSurface = surfaceDistance;
        if (!WithinPassthroughDepth(surfaceDistance, firstSurface)) return true;
        const float step = impactDistance + PassthroughStep;
        if (step >= remaining) return true;
        advanced += step;
        traceOrigin = AdvancePoint(ray, advanced);
    }
    result.hit = true;
    result.nearest = true;
    result.passthroughAdvance = advanced;
    const Object map = Bind<Object(*)(Object, Method)>(base_, 0x8011E2C)(nullptr, nullptr);
    if (!map) return false;
    const Object target = Bind<Object(*)(Object, Object, Method)>(base_, 0x78AF19C)(
        map, reinterpret_cast<Object>(collider), nullptr);
    const Address targetAddress = reinterpret_cast<Address>(target);

    if (!target || targetAddress == pawn_ || !EnemyMember(match_, targetAddress)) return true;
    result.enemy = true;
    const Address info = Read<Address>(targetAddress, 0x78);
    const float health = Read<float>(info, 0x34);
    result.alive = Read<unsigned char>(targetAddress, 0x550) != 0 && info &&
                   std::isfinite(health) && health > 0;
    if (!result.alive) return true;




    using DetailTrace = bool(*)(Object, Object, Object, Point3, Point3,
                               float, int, Object*, Method);
    const bool detailed = Bind<DetailTrace>(base_, 0x66FE6B4)(
        nullptr, target, reinterpret_cast<Object>(collider), traceOrigin,
        ray.direction, ray.maxDistance - advanced, 0, &impact.object, nullptr);
    if (!detailed || !impact.object) return true;
    const Address address = reinterpret_cast<Address>(impact.object);
    const int group = Read<int>(address, 0x70);
    const bool avatar = Read<unsigned char>(address, 0x8D) != 0;
    result.detailed = BodyGroup(group, avatar);
    if (!result.detailed) return true;
    result.point = Read<Point3>(address, 0x28);
    result.distance = advanced + Read<float>(address, 0x5C);
    const auto actor = Read<std::uint32_t>(info, 0x30);
    if (actor == 0) return false;


    result.targetGeneration = (context.match * 0x100000001b3ULL) ^ actor;
    if (!result.targetGeneration) result.targetGeneration = actor;
    return true;
}

Occlusion NativeTargetQueries::CheckWorld(const ContextIdentity&,
                                         const ViewRay& ray, const BodyHit& hit) noexcept {
    if (!base_ || !std::isfinite(hit.distance) || hit.distance <= 0) return Occlusion::Unavailable;
    const float advance = hit.passthroughAdvance;
    if (!std::isfinite(advance) || advance < 0.0f || advance >= hit.distance)
        return Occlusion::Unavailable;
    const Point3 start = advance > 0.0f ? AdvancePoint(ray, advance) : ray.origin;
    const float distance = hit.distance - advance;
    float blockerDistance = 0.0f;
    using WorldTrace = bool(*)(Object, Point3, Point3, float, float*, int, Method);
    const bool blocked = Bind<WorldTrace>(base_, 0x66F2708)(
        nullptr, start, ray.direction, distance, &blockerDistance, 0, nullptr);
    if (!blocked) return Occlusion::Clear;
    if (!std::isfinite(blockerDistance) || blockerDistance < 0) return Occlusion::Unavailable;
    const float tolerance = std::max(0.0001f,
        8.0f * std::numeric_limits<float>::epsilon() * distance);
    if (blockerDistance + tolerance >= distance) return Occlusion::Clear;
    OwnedImpact impact{base_, Bind<Object(*)(Object, Method)>(base_, 0x62588E4)(nullptr, nullptr)};
    if (impact.object) {
        using BroadTrace = bool(*)(Object, Point3, Point3, float, int, Object, int, Method);
        if (Bind<BroadTrace>(base_, 0x66F17EC)(nullptr, start, ray.direction, distance,
                                              LocalBulletTrace, impact.object, 0, nullptr) &&
            BulletIgnorable(base_, impact))
            return Occlusion::Clear;
    }
    return Occlusion::Blocked;
}

Occlusion NativeTargetQueries::CheckSmoke(const ContextIdentity&,
                                         const ViewRay& ray, const BodyHit& hit) noexcept {
    if (!base_ || !match_ || !Finite(ray.origin) || !Finite(hit.point))
        return Occlusion::Unavailable;
    const auto match = reinterpret_cast<Object>(match_);
    using PointQuery = bool(*)(Object, Point3, Method);
    using SegmentQuery = bool(*)(Object, Point3, Point3, Method);
    const auto inSmoke = Bind<PointQuery>(base_, 0x5E5FA20);
    if (inSmoke(match, ray.origin, nullptr) || inSmoke(match, hit.point, nullptr) ||
        Bind<SegmentQuery>(base_, 0x5E5FBE8)(match, ray.origin, hit.point, nullptr) ||
        Bind<SegmentQuery>(base_, 0x5E5FDB4)(match, ray.origin, hit.point, nullptr))
        return Occlusion::Blocked;
    return Occlusion::Clear;
}
}
