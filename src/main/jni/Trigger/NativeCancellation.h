#pragma once
#include "TriggerCore.h"

namespace PhantomTrigger {
struct NativeMergeDecision {
    Intent input{};
    bool stopResult = false;
    bool resetController = false;
};




class NativeCancellation {
public:
    bool HasPendingCleanup() const noexcept { return pending_; }
    NativeMergeDecision Resolve(const ContextIdentity& context, bool manual,
                                bool releaseFire, const Intent& intent,
                                bool allowReleaseSequence = false) noexcept {
        if (manual) { Reset(); return {}; }
        if (pending_ && owner_ != context) Reset();
        if (pending_ && (sequenceCleanup_ || allowReleaseSequence)) {
            Reset();
            return {{false, false, false, false}, true, true};
        }
        if (allowReleaseSequence) {
            if (!intent.passthrough && intent.up && !intent.commitRelease) {
                pending_ = true;
                sequenceCleanup_ = true;
                owner_ = context;
                return {{}, true, false};
            }
            return {intent, false, false};
        }
        if (!intent.passthrough && releaseFire && (intent.up || intent.down || intent.held)) {
            pending_ = true;
            owner_ = context;
            return {{}, true, false};
        }
        if (pending_ && !releaseFire) {
            pending_ = false;
            return {{false,false,true,false}, false, true};
        }
        return {intent, false, false};
    }
    void Reset() noexcept { pending_ = false; sequenceCleanup_ = false; }
private:
    ContextIdentity owner_{};
    bool pending_ = false;
    bool sequenceCleanup_ = false;
};
}
