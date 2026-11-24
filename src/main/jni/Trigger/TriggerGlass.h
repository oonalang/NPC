#pragma once

#include "TriggerRuntime.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace PhantomTrigger {

inline constexpr std::size_t MaxGlassContacts = 32;
inline constexpr std::size_t MaxGlassPanes = 2;

struct RayContact {
    std::uintptr_t collider = 0;
    float distance = 0.f;
};

struct GlassPath {
    bool verified = false;
    RayContact firstSolid{};
    float lastPaneDistance = -1.f;
};

using RayContacts = std::array<RayContact, MaxGlassContacts>;
enum class ContactKind { Unavailable, Solid, Glass, Character, BulletIgnored };

inline bool EligibleBodyGroup(int group,bool avatar) noexcept {
    return group!=6 && group!=32 && ((group>=1 && group<=31) || (group==0 && avatar));
}

inline ContactKind VerifiedCharacterContact(bool detailed,int group,bool avatar) noexcept {
    if(!detailed) return ContactKind::Unavailable;
    return EligibleBodyGroup(group,avatar)?ContactKind::Character:ContactKind::Solid;
}

inline bool IsPassableGlass(int materialType) noexcept {
    return materialType == 1 || materialType == 12 || materialType == 33;
}

inline bool IsBulletIgnoredWorldMaterial(int materialType,std::uintptr_t actor=0) noexcept {
    return !actor && materialType>=0 && materialType<16 && ((0xA800u>>materialType)&1u)!=0;
}

inline bool CompleteContactCount(int count, std::uint64_t capacity) noexcept {
    return count > 0 && static_cast<std::uint64_t>(count) < capacity &&
        static_cast<std::size_t>(count) <= MaxGlassContacts;
}

inline float ContactTolerance(float distance) noexcept {
    return std::max(.0001f, 8.f * std::numeric_limits<float>::epsilon() * distance);
}

inline float GlassRetraceTolerance(const ViewRay& ray, float distance) noexcept {
    return ContactTolerance(std::max({distance, std::fabs(ray.origin.x),
                                     std::fabs(ray.origin.y), std::fabs(ray.origin.z)}));
}

inline bool PrepareRayContacts(RayContacts& contacts,std::size_t count,RayContact nearest,float distance) noexcept {
    if (!count || count > contacts.size() || !std::isfinite(distance) || distance <= 0.f ||
        !std::isfinite(nearest.distance) || nearest.distance < 0.f || nearest.distance > distance)
        return false;
    auto end = contacts.begin() + count;
    for (auto at = contacts.begin(); at != end; ++at) {
        if (!at->collider || !std::isfinite(at->distance) || at->distance < 0.f ||
            at->distance > distance + ContactTolerance(distance))
            return false;
    }
    std::sort(contacts.begin(), end, [](RayContact a, RayContact b) { return a.distance < b.distance; });
    const float tolerance = ContactTolerance(nearest.distance);
    if (std::fabs(contacts[0].distance - nearest.distance) > tolerance) return false;
    if (nearest.collider && std::none_of(contacts.begin(), end, [&](RayContact contact) {
            return contact.collider == nearest.collider &&
                std::fabs(contact.distance - nearest.distance) <= tolerance;
        })) return false;
    return true;
}

inline bool IsCharacterWorldContact(std::uintptr_t collider,std::uintptr_t bodyCollider,
                                    std::uintptr_t actor,std::uintptr_t self,std::uintptr_t target) noexcept {
    return (collider && bodyCollider && collider==bodyCollider) ||
        (actor && ((self && actor==self) || (target && actor==target)));
}

template<class Classify>
Occlusion CheckWorldContacts(RayContacts& contacts,std::size_t count,RayContact nearest,
                              float distance,Classify classify) noexcept {
    if(!PrepareRayContacts(contacts,count,nearest,distance)) return Occlusion::Unavailable;
    std::size_t panes=0;
    const float tolerance=ContactTolerance(distance);
    for(std::size_t index=0;index<count;++index) {
        const auto contact=contacts[index];
        if(contact.distance+tolerance>=distance) return Occlusion::Clear;
        const auto kind=classify(contact.collider);
        if(kind==ContactKind::Unavailable) return Occlusion::Unavailable;
        if(kind==ContactKind::Solid) return Occlusion::Blocked;
        if(kind==ContactKind::Character || kind==ContactKind::BulletIgnored) continue;
        if(kind!=ContactKind::Glass || ++panes>MaxGlassPanes) return Occlusion::Unavailable;
    }
    return Occlusion::Clear;
}

template<class Trace,class Classify>
Occlusion ResolveWorldOcclusion(float distance,bool blocked,float blockerDistance,Trace trace,Classify classify) noexcept {
    if(!std::isfinite(distance) || distance<=0.f) return Occlusion::Unavailable;
    if(!blocked) return Occlusion::Clear;
    if(!std::isfinite(blockerDistance) || blockerDistance<0.f ||
        blockerDistance>distance+ContactTolerance(distance)) return Occlusion::Unavailable;
    if(blockerDistance+ContactTolerance(distance)>=distance) return Occlusion::Clear;
    RayContacts contacts{};
    std::size_t count=0;
    if(!trace(contacts,count)) return Occlusion::Unavailable;
    return CheckWorldContacts(contacts,count,{0,blockerDistance},distance,classify);
}

template<class Classify>
GlassPath CheckGlassPath(RayContacts& contacts, std::size_t count, RayContact nearest,
                         float distance, Classify classify) noexcept {
    if(!PrepareRayContacts(contacts,count,nearest,distance)) return {};
    const auto end=contacts.begin()+count;

    GlassPath path{true, {}, -1.f};
    std::size_t panes = 0;
    for (auto at = contacts.begin(); at != end; ++at) {
        const auto kind = classify(at->collider);
        if (kind == ContactKind::Unavailable) return {};
        if (kind == ContactKind::Solid) {
            path.firstSolid = *at;
            return path;
        }
        if (kind == ContactKind::BulletIgnored) {
            path.lastPaneDistance = at->distance;
            continue;
        }
        if (kind!=ContactKind::Glass || ++panes > MaxGlassPanes) return {};
        path.lastPaneDistance = at->distance;
    }
    return path;
}

inline bool AdvancePastGlass(const ViewRay& ray, const GlassPath& path, ViewRay& out,
                             float& advance) noexcept {
    if (!path.verified || !path.firstSolid.collider || path.lastPaneDistance < 0.f) return false;
    const float separation = path.firstSolid.distance - path.lastPaneDistance;
    const float tolerance = GlassRetraceTolerance(ray, path.firstSolid.distance);
    if (separation <= tolerance || path.firstSolid.distance > ray.maxDistance) return false;
    advance = path.lastPaneDistance + std::min(std::max(.001f, tolerance), separation * .5f);
    if (advance <= path.lastPaneDistance || advance >= path.firstSolid.distance) return false;
    out = {{ray.origin.x + ray.direction.x * advance,
            ray.origin.y + ray.direction.y * advance,
            ray.origin.z + ray.direction.z * advance}, ray.direction, ray.maxDistance - advance};
    return true;
}
}
