#include "NativeQueries.h"
#include "NativeProfile.h"
#include "NativeRay.h"
#include "TriggerGlass.h"

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
constexpr int WorldTraceFlag = 0x12009;
constexpr std::size_t MaximumPawnCount = 512;
constexpr std::uint64_t MaximumRaycastBuffer = 256;
using BroadTrace = bool(*)(Object, Point3, Point3, float, int, Object, int, Method);

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

bool IsGlassMaterial(Address base, Object material) noexcept {
    if (!material) return false;
    using MaterialType = int(*)(Object, Object, Method);
    return IsPassableGlass(Bind<MaterialType>(base, 0x66F1E00)(nullptr, material, nullptr));
}

GlassPath TraceGlassPath(Address base, const ViewRay& ray, float distance, RayContact nearest,
                         int traceFlag) noexcept {
    if (!NativeGlassQueriesReady(base)) return {};
    const auto scratch = Bind<Object(*)(Object, Method)>(base, 0x66F0820)(nullptr, nullptr);
    const auto array = reinterpret_cast<Address>(scratch);
    const auto capacity = Read<std::uint64_t>(array, 0x18);
    if (capacity < 2 || capacity > MaximumRaycastBuffer) return {};
    using GetLayerMask = std::int64_t(*)(Object, int, Method);
    using RemoveWaterLayer = std::int64_t(*)(Object, std::int64_t, Method);
    auto mask = Bind<GetLayerMask>(base, 0x66ECB64)(nullptr, traceFlag, nullptr);
    if (traceFlag == WorldTraceFlag)
        mask = Bind<RemoveWaterLayer>(base, 0x66F2694)(nullptr, mask, nullptr);
    using RaycastNonAlloc = int(*)(Object, Point3, Point3, Object, float, std::int64_t, Method);
    const int count = Bind<RaycastNonAlloc>(base, 0x531EA00)(
        nullptr, ray.origin, ray.direction, scratch, distance, mask, nullptr);
    if (!CompleteContactCount(count, capacity)) return {};

    RayContacts contacts{};
    for (int i = 0; i < count; ++i) {
        const auto at = 0x20 + static_cast<std::size_t>(i) * 0x30;
        contacts[i] = {Read<Address>(array, at + 0x28), Read<float>(array, at + 0x1C)};
    }
    using IsLiveObject = bool(*)(Object, Object, Object, Method);
    using SharedMaterial = Object(*)(Object, Method);
    return CheckGlassPath(contacts, static_cast<std::size_t>(count), nearest, distance,
        [base](Address address) noexcept {
            auto collider = reinterpret_cast<Object>(address);
            if (!Bind<IsLiveObject>(base, 0x52B7CE4)(nullptr, collider, nullptr, nullptr)) return false;
            return IsGlassMaterial(base, Bind<SharedMaterial>(base, 0x52D98B8)(collider, nullptr));
        });
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
    const auto trace = Bind<BroadTrace>(base_, 0x66F17EC);
    if (!trace(nullptr, ray.origin, ray.direction, ray.maxDistance, LocalBulletTrace,
               impact.object, 0, nullptr)) return true;
    result.hit = true;
    result.nearest = true;
    const auto impactAddress = reinterpret_cast<Address>(impact.object);
    auto collider = Read<Address>(impactAddress, 0x18);
    if (!collider) return false;
    const Object map = Bind<Object(*)(Object, Method)>(base_, 0x8011E2C)(nullptr, nullptr);
    if (!map) return false;
    const auto lookup = Bind<Object(*)(Object, Object, Method)>(base_, 0x78AF19C);
    auto target = lookup(map, reinterpret_cast<Object>(collider), nullptr);
    auto targetAddress = reinterpret_cast<Address>(target);
    if (targetAddress == pawn_) return true;
    ViewRay detailRay = ray;
    float advance = 0.f;
    if (!EnemyMember(match_, targetAddress)) {
        if (!IsGlassMaterial(base_, Read<Object>(impactAddress, 0x78))) return true;
        const RayContact surface{collider, Read<float>(impactAddress, 0x5C)};
        const auto path = TraceGlassPath(base_, ray, ray.maxDistance, surface, LocalBulletTrace);
        if (!AdvancePastGlass(ray, path, detailRay, advance)) return true;

        collider = path.firstSolid.collider;
        target = lookup(map, reinterpret_cast<Object>(collider), nullptr);
        targetAddress = reinterpret_cast<Address>(target);
        if (targetAddress == pawn_ || !EnemyMember(match_, targetAddress)) return true;
        const float tolerance = GlassRetraceTolerance(ray, path.firstSolid.distance);
        const float retraceDistance = std::min(detailRay.maxDistance, path.firstSolid.distance - advance + tolerance);
        if (!trace(nullptr, detailRay.origin, detailRay.direction, retraceDistance,
                   LocalBulletTrace, impact.object, 0, nullptr)) return true;
        const float retraced = Read<float>(impactAddress, 0x5C);
        if (Read<Address>(impactAddress, 0x18) != collider || !std::isfinite(retraced) ||
            retraced < 0.f || std::fabs(advance + retraced - path.firstSolid.distance) > tolerance)
            return false;
    }
    result.enemy = true;
    const Address info = Read<Address>(targetAddress, 0x78);
    const float health = Read<float>(info, 0x34);
    result.alive = Read<unsigned char>(targetAddress, 0x550) != 0 && info &&
                   std::isfinite(health) && health > 0;
    if (!result.alive) return true;




    using DetailTrace = bool(*)(Object, Object, Object, Point3, Point3,
                               float, int, Object*, Method);
    const bool detailed = Bind<DetailTrace>(base_, 0x66FE6B4)(
        nullptr, target, reinterpret_cast<Object>(collider), detailRay.origin,
        detailRay.direction, detailRay.maxDistance, 0, &impact.object, nullptr);
    if (!detailed || !impact.object) return true;
    const Address address = reinterpret_cast<Address>(impact.object);
    const int group = Read<int>(address, 0x70);
    const bool avatar = Read<unsigned char>(address, 0x8D) != 0;
    result.detailed = BodyGroup(group, avatar);
    if (!result.detailed) return true;
    result.point = Read<Point3>(address, 0x28);
    const float detailDistance = Read<float>(address, 0x5C);
    if (!std::isfinite(detailDistance) || detailDistance <= 0.f || detailDistance > detailRay.maxDistance)
        return false;
    result.distance = advance + detailDistance;
    const auto actor = Read<std::uint32_t>(info, 0x30);
    if (actor == 0) return false;


    result.targetGeneration = (context.match * 0x100000001b3ULL) ^ actor;
    if (!result.targetGeneration) result.targetGeneration = actor;
    return true;
}

Occlusion NativeTargetQueries::CheckWorld(const ContextIdentity&,
                                         const ViewRay& ray, const BodyHit& hit) noexcept {
    if (!base_ || !std::isfinite(hit.distance) || hit.distance <= 0) return Occlusion::Unavailable;
    float blockerDistance = 0;
    using WorldTrace = bool(*)(Object, Point3, Point3, float, float*, int, Method);
    const bool blocked = Bind<WorldTrace>(base_, 0x66F2708)(
        nullptr, ray.origin, ray.direction, hit.distance, &blockerDistance, 0, nullptr);
    if (!blocked) return Occlusion::Clear;
    if (!std::isfinite(blockerDistance) || blockerDistance < 0) return Occlusion::Unavailable;
    const float tolerance = ContactTolerance(hit.distance);
    if (blockerDistance + tolerance >= hit.distance) return Occlusion::Clear;

    const auto path = TraceGlassPath(base_, ray, hit.distance, {0, blockerDistance}, WorldTraceFlag);
    if (!path.verified || path.lastPaneDistance < 0.f) return Occlusion::Blocked;
    if (path.firstSolid.collider && path.firstSolid.distance + tolerance < hit.distance) return Occlusion::Blocked;
    return Occlusion::Clear;
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
