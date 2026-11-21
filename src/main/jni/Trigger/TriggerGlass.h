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
enum class ContactKind { Unavailable, Solid, Glass };

inline bool IsPassableGlass(int materialType) noexcept {
    return materialType == 1 || materialType == 12 || materialType == 33;
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

template<class Classify>
GlassPath CheckGlassPath(RayContacts& contacts, std::size_t count, RayContact nearest,
                         float distance, Classify classify) noexcept {
    if (!count || count > contacts.size() || !std::isfinite(distance) || distance <= 0.f ||
        !std::isfinite(nearest.distance) || nearest.distance < 0.f || nearest.distance > distance)
        return {};
    auto end = contacts.begin() + count;
    for (auto at = contacts.begin(); at != end; ++at) {
        if (!at->collider || !std::isfinite(at->distance) || at->distance < 0.f ||
            at->distance > distance + ContactTolerance(distance))
            return {};
    }
    std::sort(contacts.begin(), end, [](RayContact a, RayContact b) { return a.distance < b.distance; });
    const float tolerance = ContactTolerance(nearest.distance);
    if (std::fabs(contacts[0].distance - nearest.distance) > tolerance) return {};
    if (nearest.collider && std::none_of(contacts.begin(), end, [&](RayContact contact) {
            return contact.collider == nearest.collider &&
                std::fabs(contact.distance - nearest.distance) <= tolerance;
        })) return {};

    GlassPath path{true, {}, -1.f};
    std::size_t panes = 0;
    for (auto at = contacts.begin(); at != end; ++at) {
        const auto kind = classify(at->collider);
        if (kind == ContactKind::Unavailable) return {};
        if (kind == ContactKind::Solid) {
            path.firstSolid = *at;
            return path;
        }
        if (++panes > MaxGlassPanes) return {};
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
