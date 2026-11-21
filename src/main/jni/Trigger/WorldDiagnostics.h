#pragma once
#include "TriggerRuntime.h"
#include <chrono>
#include <cstdio>
#include <mutex>

namespace PhantomTrigger {
inline constexpr const char* WorldDiagnosticRevision="obstruction-3";
inline constexpr std::uint64_t WorldDiagnosticFreshnessMillis=1500;
enum class WorldProbeReason : std::uint8_t {
    None,Clear,Endpoint,ContactsUnavailable,ContactLimit,ContactMismatch,MapUnavailable,
    ColliderUnavailable,CharacterUnavailable,Character,Glass,Solid,Shield,InvalidDistance
};
enum class WorldProbeOwner : std::uint8_t { Unknown,Body,Self,Target,Other,World };
struct WorldDiagnostic {
    Occlusion result=Occlusion::Unavailable;
    WorldProbeReason reason=WorldProbeReason::None;
    WorldProbeOwner owner=WorldProbeOwner::Unknown;
    float targetDistance=0.f,worldDistance=-1.f,contactDistance=-1.f;
    int material=-1,group=-1;
    std::uint32_t contacts=0;
    std::uint64_t sequence=0,timeMillis=0;
};
inline std::uint64_t WorldDiagnosticClock() noexcept {
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}
namespace world_diagnostic_detail {
inline std::mutex mutex;
inline WorldDiagnostic value;
}
inline void PublishWorldDiagnostic(WorldDiagnostic value) noexcept {
    std::lock_guard<std::mutex> lock(world_diagnostic_detail::mutex);
    value.sequence=world_diagnostic_detail::value.sequence+1;
    if(!value.sequence) value.sequence=1;
    if(!value.timeMillis) value.timeMillis=WorldDiagnosticClock();
    world_diagnostic_detail::value=value;
}
inline WorldDiagnostic GetWorldDiagnostic() noexcept {
    std::lock_guard<std::mutex> lock(world_diagnostic_detail::mutex);
    return world_diagnostic_detail::value;
}
inline const char* WorldProbeReasonText(WorldProbeReason reason) noexcept {
    switch(reason) {
        case WorldProbeReason::None:return "No world sample";
        case WorldProbeReason::Clear:return "Clear world ray";
        case WorldProbeReason::Endpoint:return "Target endpoint";
        case WorldProbeReason::ContactsUnavailable:return "Contact query unavailable";
        case WorldProbeReason::ContactLimit:return "Contact buffer limit reached";
        case WorldProbeReason::ContactMismatch:return "Contact distances disagree";
        case WorldProbeReason::MapUnavailable:return "Collider map unavailable";
        case WorldProbeReason::ColliderUnavailable:return "Collider no longer valid";
        case WorldProbeReason::CharacterUnavailable:return "Character hit unverified";
        case WorldProbeReason::Character:return "Verified character contact";
        case WorldProbeReason::Glass:return "Passable glass contact";
        case WorldProbeReason::Solid:return "Solid collider reported";
        case WorldProbeReason::Shield:return "Shield/body group blocks";
        case WorldProbeReason::InvalidDistance:return "Invalid world distance";
    }
    return "Unknown world result";
}
inline const char* WorldProbeOwnerText(WorldProbeOwner owner) noexcept {
    switch(owner) {
        case WorldProbeOwner::Unknown:return "unknown owner";
        case WorldProbeOwner::Body:return "body collider";
        case WorldProbeOwner::Self:return "local character";
        case WorldProbeOwner::Target:return "target character";
        case WorldProbeOwner::Other:return "other actor";
        case WorldProbeOwner::World:return "world/unmapped";
    }
    return "unknown owner";
}
inline bool FormatWorldDiagnostic(char* text,std::size_t capacity,const WorldDiagnostic& value,
                                  std::uint64_t now) noexcept {
    if(!text || !capacity) return false;
    text[0]='\0';
    if(!value.sequence || value.reason==WorldProbeReason::None || value.result==Occlusion::Clear) return false;
    if(now<value.timeMillis) {
        std::snprintf(text,capacity,"Last world ray unavailable: clock changed");
        return true;
    }
    const auto age=now-value.timeMillis;
    if(age>WorldDiagnosticFreshnessMillis) {
        std::snprintf(text,capacity,"No fresh world ray (%llu ms old, #%llu)",
            static_cast<unsigned long long>(age),static_cast<unsigned long long>(value.sequence));
        return true;
    }
    std::snprintf(text,capacity,"Last world ray: %s\nBody %.4f | world %.4f\nContact %.4f | %s\nMaterial %d | group %d | hits %u\n%llu ms old | #%llu",
        WorldProbeReasonText(value.reason),value.targetDistance,value.worldDistance,
        value.contactDistance,WorldProbeOwnerText(value.owner),value.material,value.group,value.contacts,
        static_cast<unsigned long long>(age),
        static_cast<unsigned long long>(value.sequence));
    return true;
}
}
