#include "TriggerRuntime.h"

#include <atomic>
#include <algorithm>
#include <cmath>
#include <limits>
#if defined(__ANDROID__) && !defined(STUDIO_STANDALONE_PREVIEW)
#include "NativeDriver.h"
#endif

namespace PhantomTrigger {
namespace {

constexpr std::uint32_t EnabledBit = 1u;
constexpr std::uint32_t AdsOnlyBit = 2u;
constexpr std::uint32_t DotBit = 4u;
std::atomic<std::uint32_t> requestedSettings{0u};

bool Finite(const Point3& point) noexcept {
    return std::isfinite(point.x) && std::isfinite(point.y) &&
           std::isfinite(point.z);
}

bool ValidRay(const ViewRay& ray) noexcept {
    if (!Finite(ray.origin) || !Finite(ray.direction) ||
        !std::isfinite(ray.maxDistance) || ray.maxDistance <= 0.0f) {
        return false;
    }
    const double lengthSquared = static_cast<double>(ray.direction.x) * ray.direction.x +
                                 static_cast<double>(ray.direction.y) * ray.direction.y +
                                 static_cast<double>(ray.direction.z) * ray.direction.z;
    return std::fabs(lengthSquared - 1.0) <= 1e-5;
}

bool ValidHit(const ViewRay& ray, const BodyHit& hit) noexcept {
    if (!Finite(hit.point) || !std::isfinite(hit.distance) ||
        hit.distance <= 0.0f || hit.distance > ray.maxDistance) {
        return false;
    }


    const double dx = static_cast<double>(hit.point.x) - ray.origin.x;
    const double dy = static_cast<double>(hit.point.y) - ray.origin.y;
    const double dz = static_cast<double>(hit.point.z) - ray.origin.z;
    const double projection = dx * ray.direction.x + dy * ray.direction.y +
                              dz * ray.direction.z;

    const double scale = std::max({1.0, static_cast<double>(hit.distance),
        std::fabs(static_cast<double>(ray.origin.x)),
        std::fabs(static_cast<double>(ray.origin.y)),
        std::fabs(static_cast<double>(ray.origin.z)),
        std::fabs(static_cast<double>(hit.point.x)),
        std::fabs(static_cast<double>(hit.point.y)),
        std::fabs(static_cast<double>(hit.point.z))});
    const double tolerance = std::max(0.0001,
        8.0 * std::numeric_limits<float>::epsilon() * scale);
    if (projection <= 0.0 || std::fabs(projection - hit.distance) > tolerance) {
        return false;
    }
    const double ex = dx - static_cast<double>(ray.direction.x) * hit.distance;
    const double ey = dy - static_cast<double>(ray.direction.y) * hit.distance;
    const double ez = dz - static_cast<double>(ray.direction.z) * hit.distance;
    return ex * ex + ey * ey + ez * ez <= tolerance * tolerance;
}

bool NeedsWeaponState(const Settings& settings, const Frame& frame,
                      const Controller& controller) noexcept {
    return controller.NeedsFreshTarget(frame) && settings.enabled &&
        (settings.mode == Mode::Both || settings.mode == Mode::ADSOnly) &&
        frame.contextValid && frame.bindingsReady && !frame.manualActive &&
        frame.ammoAvailable && !frame.hardBlocked && frame.fireMode != FireMode::Unsupported;
}

}

Settings GetSettings() noexcept {
    const auto bits = requestedSettings.load(std::memory_order_acquire);
    Settings result{};
    result.enabled = (bits & EnabledBit) != 0u;
    result.mode = (bits & AdsOnlyBit) != 0u ? Mode::ADSOnly : Mode::Both;
    result.showDot = (bits & DotBit) != 0u;
    return result;
}

void SetSettings(const Settings& settings) noexcept {
    const bool validMode = settings.mode == Mode::Both || settings.mode == Mode::ADSOnly;
    const auto bits = (settings.enabled && validMode ? EnabledBit : 0u) |
                      (settings.mode == Mode::ADSOnly ? AdsOnlyBit : 0u) |
                      (settings.showDot ? DotBit : 0u);
    requestedSettings.store(bits, std::memory_order_release);
}

RuntimeStatus GetRuntimeStatus() noexcept {
#if defined(__ANDROID__) && !defined(STUDIO_STANDALONE_PREVIEW)
    return NativeStatus();
#else
    return RuntimeStatus::WaitingForBinaryVerification;
#endif
}

FireMode GetDetectedFireMode() noexcept {
#if defined(__ANDROID__) && !defined(STUDIO_STANDALONE_PREVIEW)
    return NativeDetectedMode();
#else
    return FireMode::Unsupported;
#endif
}

DecisionStatus GetDecisionStatus() noexcept {
#if defined(__ANDROID__) && !defined(STUDIO_STANDALONE_PREVIEW)
    return NativeDecisionStatus();
#else
    return DecisionStatus::Setup;
#endif
}

const char* DecisionStatusText(DecisionStatus value) noexcept {
    switch (value) {
        case DecisionStatus::Disabled: return "Disabled";
        case DecisionStatus::Setup: return "Waiting for game verification";
        case DecisionStatus::Context: return "Waiting for a live player and weapon";
        case DecisionStatus::Manual: return "Manual fire has priority";
        case DecisionStatus::Unsupported: return "Weapon input mode is unsupported";
        case DecisionStatus::Empty: return "Waiting for ammo";
        case DecisionStatus::Blocked: return "Weapon is busy";
        case DecisionStatus::Aiming: return "Waiting for full aim";
        case DecisionStatus::Cooldown: return "Weapon is recovering";
        case DecisionStatus::NoTarget: return "No enemy under the crosshair";
        case DecisionStatus::QueryUnavailable: return "Target could not be verified";
        case DecisionStatus::WorldBlocked: return "Target is behind cover";
        case DecisionStatus::SmokeBlocked: return "Smoke blocks the target";
        case DecisionStatus::Preparing: return "Preparing the shot";
        case DecisionStatus::Waiting: return "Waiting for shot confirmation";
        case DecisionStatus::Firing: return "Fire input active";
        case DecisionStatus::Recovering: return "Resetting fire input";
    }
    return "Waiting for game verification";
}

DecisionStatus ExplainDecision(const Settings& settings, const Frame& frame,
        TargetResult target, const Intent& intent, const Controller& controller) noexcept {
    if (!settings.enabled) return DecisionStatus::Disabled;
    if (frame.manualActive) return DecisionStatus::Manual;
    if (!frame.contextValid || !frame.bindingsReady) return DecisionStatus::Context;
    if (controller.HasPendingShot()) return DecisionStatus::Waiting;
    if (controller.IsRearming()) return DecisionStatus::Recovering;
    if (!intent.passthrough && !intent.down && !intent.held && !intent.commitRelease)
        return DecisionStatus::Recovering;
    if (!frame.ammoAvailable) return DecisionStatus::Empty;
    if (frame.hardBlocked) return DecisionStatus::Blocked;
    if (frame.fireMode == FireMode::Unsupported) return DecisionStatus::Unsupported;
    if (((settings.mode == Mode::ADSOnly || frame.fireMode == FireMode::SingleRelease)) &&
        (!frame.aiming || !frame.fullyAimed)) return DecisionStatus::Aiming;
    if (!frame.shotReady && !controller.HasOwnedInput()) return DecisionStatus::Cooldown;
    if (target == TargetResult::WorldBlocked) return DecisionStatus::WorldBlocked;
    if (target == TargetResult::SmokeBlocked) return DecisionStatus::SmokeBlocked;
    if (target == TargetResult::QueryUnavailable) return DecisionStatus::QueryUnavailable;
    if (intent.down && frame.fireMode == FireMode::SingleRelease) return DecisionStatus::Preparing;
    if (intent.down || intent.held || intent.commitRelease) return DecisionStatus::Firing;
    return DecisionStatus::NoTarget;
}

bool GetViewportCenter(float& normalizedX, float& normalizedY) noexcept {
#if defined(__ANDROID__) && !defined(STUDIO_STANDALONE_PREVIEW)
    return NativeViewportCenter(normalizedX, normalizedY);
#else
    normalizedX = 0.5f;
    normalizedY = 0.5f;
    return false;
#endif
}

TargetResult EvaluateTarget(TargetQueries& queries,
                            const ContextIdentity& context,
                            const ViewRay& ray) noexcept {
    if (!ValidRay(ray)) return TargetResult::InvalidRay;
    BodyHit hit{};
    if (!queries.TraceBody(context, ray, hit)) return TargetResult::QueryUnavailable;
    if (!hit.hit || !hit.enemy || !hit.alive || !hit.detailed || !hit.nearest ||
        hit.targetGeneration == 0) {
        return TargetResult::NoEligibleBody;
    }
    if (!ValidHit(ray, hit)) return TargetResult::InvalidHit;
    const auto world = queries.CheckWorld(context, ray, hit);
    if (world == Occlusion::Unavailable) return TargetResult::QueryUnavailable;
    if (world != Occlusion::Clear) return TargetResult::WorldBlocked;
    const auto smoke = queries.CheckSmoke(context, ray, hit);
    if (smoke == Occlusion::Unavailable) return TargetResult::QueryUnavailable;
    if (smoke != Occlusion::Clear) return TargetResult::SmokeBlocked;
    return TargetResult::Eligible;
}

bool NeedsAimState(const Settings& settings, const Frame& frame,
                    const Controller& controller) noexcept {
    return NeedsWeaponState(settings,frame,controller) &&
        (settings.mode == Mode::ADSOnly || frame.fireMode == FireMode::SingleRelease);
}

bool NeedsWeaponReadiness(const Settings& settings, const Frame& frame,
                          const Controller& controller) noexcept {
    const bool modeReady = settings.mode == Mode::Both ||
                           (settings.mode == Mode::ADSOnly && frame.aiming && frame.fullyAimed);
    const bool releaseAdsReady = frame.fireMode != FireMode::SingleRelease ||
                                 (frame.aiming && frame.fullyAimed);
    return NeedsWeaponState(settings,frame,controller) && modeReady && releaseAdsReady;
}

bool NeedsTargetQuery(const Settings& settings, const Frame& frame,
                      const Controller& controller) noexcept {
    const bool supported = frame.fireMode == FireMode::Automatic ||
                           frame.fireMode == FireMode::BurstAutomatic ||
                           frame.fireMode == FireMode::Single ||
                           frame.fireMode == FireMode::BurstSingle ||
                           frame.fireMode == FireMode::SingleRelease;
    return supported && NeedsWeaponReadiness(settings,frame,controller) &&
        (frame.shotReady || controller.HasOwnedInputFor(frame.identity));
}

Intent EvaluateInput(const Settings& settings, const Frame& input,
                     const ViewRay& ray, TargetQueries& queries,
                     Controller& controller) noexcept {
    Frame frame = input;


    frame.targetValid = false;
    if (NeedsTargetQuery(settings,frame,controller)) {
        frame.targetValid = EvaluateTarget(queries, frame.identity, ray) == TargetResult::Eligible;
    }
    return controller.Step(settings, frame);
}

}
