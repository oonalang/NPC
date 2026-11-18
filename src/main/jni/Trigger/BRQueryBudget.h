#pragma once
#include "TriggerCore.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace PhantomTrigger {

inline float BRTraceDistance(float distance, bool isBR) noexcept {
    if (!std::isfinite(distance) || distance <= 0.0f) return 0.0f;
    return isBR ? std::min(distance, 600.0f) : distance;
}

class BRQueryBudget final {
public:
    static constexpr std::uint64_t idlePollMilliseconds = 32;

    bool ShouldTrace(const ContextIdentity& context, std::uint64_t nowMillis,
                     bool isBR, bool needsTarget, bool ownedInput) noexcept {
        if (!needsTarget) return false;
        if (!isBR) return true;
        if (!hasIdentity_ || identity_ != context) Reset(context);
        if (ownedInput || lastEligible_ || !hasProbe_ || nowMillis < lastProbeMillis_)
            return true;
        return nowMillis - lastProbeMillis_ >= idlePollMilliseconds;
    }

    void Observed(const ContextIdentity& context, std::uint64_t nowMillis,
                  bool eligible, bool isBR) noexcept {
        if (!isBR) return;
        if (!hasIdentity_ || identity_ != context) Reset(context);
        lastProbeMillis_ = nowMillis;
        hasProbe_ = true;
        lastEligible_ = eligible;
    }

    void Clear() noexcept { *this = BRQueryBudget{}; }

private:
    void Reset(const ContextIdentity& context) noexcept {
        identity_ = context;
        hasIdentity_ = true;
        hasProbe_ = false;
        lastEligible_ = false;
        lastProbeMillis_ = 0;
    }

    ContextIdentity identity_{};
    std::uint64_t lastProbeMillis_ = 0;
    bool hasIdentity_ = false;
    bool hasProbe_ = false;
    bool lastEligible_ = false;
};
}
