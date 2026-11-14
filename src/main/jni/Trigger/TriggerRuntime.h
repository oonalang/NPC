#pragma once

#include <cstddef>
#include <cstdint>
#include "TriggerCore.h"

namespace PhantomTrigger {



Settings GetSettings() noexcept;
void SetSettings(const Settings& settings) noexcept;

enum class RuntimeStatus : std::uint8_t {
    Disabled,
    WaitingForBinaryVerification,
    WaitingForContext,
    Ready,
    InvalidNativeState
};

RuntimeStatus GetRuntimeStatus() noexcept;
FireMode GetDetectedFireMode() noexcept;



bool GetViewportCenter(float& normalizedX, float& normalizedY) noexcept;

struct Point3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct ViewRay {
    Point3 origin{};
    Point3 direction{};
    float maxDistance = 0.0f;
};

struct BodyHit {
    bool hit = false;
    bool enemy = false;
    bool alive = false;
    bool detailed = false;
    bool nearest = false;
    std::uint64_t targetGeneration = 0;
    Point3 point{};
    float distance = 0.0f;
};

enum class Occlusion : std::uint8_t { Unavailable, Clear, Blocked };



class TargetQueries {
public:
    virtual ~TargetQueries() = default;
    virtual bool TraceBody(const ContextIdentity& context, const ViewRay& ray,
                           BodyHit& result) noexcept = 0;
    virtual Occlusion CheckWorld(const ContextIdentity& context,
                                 const ViewRay& ray, const BodyHit& hit) noexcept = 0;


    virtual Occlusion CheckSmoke(const ContextIdentity& context,
                                 const ViewRay& ray, const BodyHit& hit) noexcept = 0;
};

enum class TargetResult : std::uint8_t {
    InvalidRay,
    QueryUnavailable,
    NoEligibleBody,
    InvalidHit,
    WorldBlocked,
    SmokeBlocked,
    Eligible
};

TargetResult EvaluateTarget(TargetQueries& queries,
                            const ContextIdentity& context,
                            const ViewRay& ray) noexcept;




bool NeedsTargetQuery(const Settings& settings, const Frame& frame,
                      const Controller& controller) noexcept;






Intent EvaluateInput(const Settings& settings, const Frame& frame,
                     const ViewRay& ray, TargetQueries& queries,
                     Controller& controller) noexcept;

}
