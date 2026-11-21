#include "NativeQueries.h"
#include "NativeProfile.h"
#include "NativeRay.h"
#include "TriggerGlass.h"
#include "WorldDiagnostics.h"

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

using DetailTrace = bool(*)(Object,Object,Object,Point3,Point3,float,int,Object*,Method);

ContactKind VerifyWorldCharacter(Address base,Object actor,Object collider,const ViewRay& ray,float distance,int& group) noexcept {
    OwnedImpact impact{base,Bind<Object(*)(Object,Method)>(base,0x62588E4)(nullptr,nullptr)};
    if(!impact.object) return ContactKind::Unavailable;
    const bool detailed=Bind<DetailTrace>(base,0x66FE6B4)(nullptr,actor,collider,ray.origin,
        ray.direction,distance,0,&impact.object,nullptr);
    if(!impact.object) return ContactKind::Unavailable;
    const auto address=reinterpret_cast<Address>(impact.object);
    group=Read<int>(address,0x70);
    const auto kind=VerifiedCharacterContact(detailed,group,Read<unsigned char>(address,0x8D)!=0);
    if(kind!=ContactKind::Character) return kind;
    const float hitDistance=Read<float>(address,0x5C);
    if(!Read<Address>(address,0x18) || !Finite(Read<Point3>(address,0x28)) ||
        !std::isfinite(hitDistance) || hitDistance<=0.f || hitDistance>distance+ContactTolerance(distance))
        return ContactKind::Unavailable;
    return kind;
}

int PhysicalMaterialType(Address base,Object material) noexcept {
    if(!material) return -1;
    using MaterialType = int(*)(Object, Object, Method);
    return Bind<MaterialType>(base,0x66F1E00)(nullptr,material,nullptr);
}

bool IsGlassMaterial(Address base, Object material) noexcept {
    return IsPassableGlass(PhysicalMaterialType(base,material));
}

bool TraceContacts(Address base,const ViewRay& ray,float distance,int traceFlag,
                    RayContacts& contacts,int& count) noexcept {
    count=0;
    if (!NativeGlassQueriesReady(base)) return false;
    const auto scratch = Bind<Object(*)(Object, Method)>(base, 0x66F0820)(nullptr, nullptr);
    const auto array = reinterpret_cast<Address>(scratch);
    const auto capacity = Read<std::uint64_t>(array, 0x18);
    if (capacity < 2 || capacity > MaximumRaycastBuffer) return false;
    using GetLayerMask = std::int64_t(*)(Object, int, Method);
    using RemoveWaterLayer = std::int64_t(*)(Object, std::int64_t, Method);
    auto mask = Bind<GetLayerMask>(base, 0x66ECB64)(nullptr, traceFlag, nullptr);
    if (traceFlag == WorldTraceFlag)
        mask = Bind<RemoveWaterLayer>(base, 0x66F2694)(nullptr, mask, nullptr);
    using RaycastNonAlloc = int(*)(Object, Point3, Point3, Object, float, std::int64_t, Method);
    count = Bind<RaycastNonAlloc>(base, 0x531EA00)(
        nullptr, ray.origin, ray.direction, scratch, distance, mask, nullptr);
    if (!CompleteContactCount(count, capacity)) return false;

    for (int i = 0; i < count; ++i) {
        const auto at = 0x20 + static_cast<std::size_t>(i) * 0x30;
        contacts[i] = {Read<Address>(array, at + 0x28), Read<float>(array, at + 0x1C)};
    }
    return true;
}

GlassPath TraceGlassPath(Address base, const ViewRay& ray, float distance, RayContact nearest,
                         int traceFlag) noexcept {
    RayContacts contacts{};
    int count=0;
    if(!TraceContacts(base,ray,distance,traceFlag,contacts,count)) return {};
    using IsLiveObject = bool(*)(Object, Object, Object, Method);
    using SharedMaterial = Object(*)(Object, Method);
    return CheckGlassPath(contacts, static_cast<std::size_t>(count), nearest, distance,
        [base](Address address) noexcept {
            auto collider = reinterpret_cast<Object>(address);
            if (!Bind<IsLiveObject>(base, 0x52B7CE4)(nullptr, collider, nullptr, nullptr))
                return ContactKind::Unavailable;
            return IsGlassMaterial(base, Bind<SharedMaterial>(base, 0x52D98B8)(collider, nullptr))
                ? ContactKind::Glass : ContactKind::Solid;
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
    tracedTarget_=tracedCollider_=0;
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




    const bool detailed = Bind<DetailTrace>(base_, 0x66FE6B4)(
        nullptr, target, reinterpret_cast<Object>(collider), detailRay.origin,
        detailRay.direction, detailRay.maxDistance, 0, &impact.object, nullptr);
    if (!detailed || !impact.object) return true;
    const Address address = reinterpret_cast<Address>(impact.object);
    const int group = Read<int>(address, 0x70);
    const bool avatar = Read<unsigned char>(address, 0x8D) != 0;
    result.detailed = EligibleBodyGroup(group, avatar);
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
    tracedTarget_=targetAddress;
    tracedCollider_=collider;
    return true;
}

Occlusion NativeTargetQueries::CheckWorld(const ContextIdentity&,
                                         const ViewRay& ray, const BodyHit& hit) noexcept {
    if (!base_ || !std::isfinite(hit.distance) || hit.distance <= 0) return Occlusion::Unavailable;
    float blockerDistance = 0;
    using WorldTrace = bool(*)(Object, Point3, Point3, float, float*, int, Method);
    const bool blocked = Bind<WorldTrace>(base_, 0x66F2708)(
        nullptr, ray.origin, ray.direction, hit.distance, &blockerDistance, 0, nullptr);
    using IsLiveObject=bool(*)(Object,Object,Object,Method);
    using SharedMaterial=Object(*)(Object,Method);
    const auto lookup=Bind<Object(*)(Object,Object,Method)>(base_,0x78AF19C);
    Object map=nullptr;
    WorldDiagnostic diagnostic;
    diagnostic.targetDistance=hit.distance;diagnostic.worldDistance=blocked?blockerDistance:-1.f;
    diagnostic.reason=blocked?WorldProbeReason::InvalidDistance:WorldProbeReason::Clear;
    const RayContacts* sampled=nullptr;
    std::size_t classification=0;
    const auto result=ResolveWorldOcclusion(hit.distance,blocked,blockerDistance,
        [&](RayContacts& contacts,std::size_t& size) noexcept {
            diagnostic.reason=WorldProbeReason::ContactsUnavailable;
            if(!tracedTarget_ || !tracedCollider_) return false;
            int count=0;
            const bool complete=TraceContacts(base_,ray,hit.distance,WorldTraceFlag,contacts,count);
            if(count>0) diagnostic.contacts=static_cast<std::uint32_t>(count);
            if(!complete) {
                if(count>static_cast<int>(MaxGlassContacts)) diagnostic.reason=WorldProbeReason::ContactLimit;
                return false;
            }
            map=Bind<Object(*)(Object,Method)>(base_,0x8011E2C)(nullptr,nullptr);
            size=static_cast<std::size_t>(count);
            for(std::size_t index=0;index<size;++index) {
                const auto distance=contacts[index].distance;
                if(std::isfinite(distance) && distance>=0.f &&
                    (diagnostic.contactDistance<0.f || distance<diagnostic.contactDistance))
                    diagnostic.contactDistance=distance;
            }
            sampled=&contacts;
            diagnostic.reason=map?WorldProbeReason::ContactMismatch:WorldProbeReason::MapUnavailable;
            return map!=nullptr;
        },
        [&](Address address) noexcept {
            if(sampled && classification<diagnostic.contacts) diagnostic.contactDistance=(*sampled)[classification++].distance;
            diagnostic.owner=WorldProbeOwner::Unknown;diagnostic.material=diagnostic.group=-1;
            const auto collider=reinterpret_cast<Object>(address);
            if(!Bind<IsLiveObject>(base_,0x52B7CE4)(nullptr,collider,nullptr,nullptr)) {
                diagnostic.reason=WorldProbeReason::ColliderUnavailable;return ContactKind::Unavailable;
            }
            if(IsCharacterWorldContact(address,tracedCollider_,0,pawn_,tracedTarget_)) {
                diagnostic.owner=WorldProbeOwner::Body;diagnostic.reason=WorldProbeReason::Character;
                return ContactKind::Character;
            }
            const auto actor=reinterpret_cast<Address>(lookup(map,collider,nullptr));
            diagnostic.owner=actor==pawn_?WorldProbeOwner::Self:actor==tracedTarget_?WorldProbeOwner::Target:
                actor?WorldProbeOwner::Other:WorldProbeOwner::World;
            if(IsCharacterWorldContact(address,tracedCollider_,actor,pawn_,tracedTarget_)) {
                const auto kind=VerifyWorldCharacter(base_,reinterpret_cast<Object>(actor),collider,ray,hit.distance,diagnostic.group);
                diagnostic.reason=kind==ContactKind::Character?WorldProbeReason::Character:
                    kind==ContactKind::Solid?WorldProbeReason::Shield:WorldProbeReason::CharacterUnavailable;
                return kind;
            }
            diagnostic.material=PhysicalMaterialType(base_,Bind<SharedMaterial>(base_,0x52D98B8)(collider,nullptr));
            const bool glass=IsPassableGlass(diagnostic.material);
            diagnostic.reason=glass?WorldProbeReason::Glass:WorldProbeReason::Solid;
            return glass?ContactKind::Glass:ContactKind::Solid;
        });
    diagnostic.result=result;
    if(result==Occlusion::Clear) diagnostic.reason=blocked?WorldProbeReason::Endpoint:WorldProbeReason::Clear;
    PublishWorldDiagnostic(diagnostic);
    return result;
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
