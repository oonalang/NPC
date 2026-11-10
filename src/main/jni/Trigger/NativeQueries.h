#pragma once

#include "TriggerRuntime.h"
#include <cstdint>

namespace PhantomTrigger {



bool BuildNativeRay(std::uintptr_t base, ViewRay& ray,
                    float& normalizedX, float& normalizedY) noexcept;



class NativeTargetQueries final : public TargetQueries {
public:
    NativeTargetQueries(std::uintptr_t base, std::uintptr_t match,
                        std::uintptr_t pawn) noexcept
        : base_(base), match_(match), pawn_(pawn) {}

    bool TraceBody(const ContextIdentity&, const ViewRay&, BodyHit&) noexcept override;
    Occlusion CheckWorld(const ContextIdentity&, const ViewRay&, const BodyHit&) noexcept override;
    Occlusion CheckSmoke(const ContextIdentity&, const ViewRay&, const BodyHit&) noexcept override;

private:
    std::uintptr_t base_;
    std::uintptr_t match_;
    std::uintptr_t pawn_;
};
}
