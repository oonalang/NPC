#include "NativeDriver.h"
#include "NativeCancellation.h"
#include "NativeProfile.h"
#include "NativeReadiness.h"
#include "NativeInputStage.h"
#include "TriggerCompatibility.h"
#include <atomic>
#include <cmath>
#include <cstring>

#if defined(__aarch64__)
#include "NativeQueries.h"
#include "../IL2CppSDKGenerator/Dobby/dobby.h"
#endif

namespace PhantomTrigger {
namespace {
std::atomic<RuntimeStatus> status{RuntimeStatus::WaitingForBinaryVerification};
std::atomic<FireMode> detected{FireMode::Unsupported};
std::atomic<std::uint64_t> viewport{0};
std::atomic<bool> viewportValid{false};
std::atomic<std::uintptr_t> observedComponent{0};
std::atomic<std::uint64_t> committedShots{0};

#if defined(__aarch64__)
std::uintptr_t library = 0;
std::atomic<unsigned> installation{0};
std::atomic<std::uint64_t> lifecycle{1};
using Metadata = const void*;
using UpdateFunction = void(*)(void*, float, Metadata);
using ButtonFunction = void(*)(void*, void*, Metadata);
using StartMatchFunction = void(*)(void*, void*, Metadata);
using EndMatchFunction = void(*)(void*, Metadata);
UpdateFunction originalUpdate = nullptr;
ButtonFunction originalButtons = nullptr;
ButtonFunction originalCheckFire = nullptr;
StartMatchFunction originalStart = nullptr;
EndMatchFunction originalEnd = nullptr;

template<class Function> Function At(std::uintptr_t rva) noexcept {
    return reinterpret_cast<Function>(library + rva);
}
template<class Value> Value Field(std::uintptr_t object, std::size_t offset) noexcept {
    Value value{};
    std::memcpy(&value, reinterpret_cast<const void*>(object + offset), sizeof(value));
    return value;
}
using Getter = std::uintptr_t(*)(void*, Metadata);
using ObjectGetter = std::uintptr_t(*)(std::uintptr_t, Metadata);
using BoolGetter = bool(*)(std::uintptr_t, Metadata);
using FloatGetter = float(*)(std::uintptr_t, Metadata);
using BoolSetter = void(*)(void*, bool, Metadata);

struct LocalState {
    Controller controller;
    std::uintptr_t scope = 0;
    unsigned depth = 0;
    bool seen = false;
    bool buttonsSampled = false;
    bool manualActive = false;
    void* sampledParam = nullptr;
    std::uint64_t update = 0;
    std::uint64_t epoch = 0;
    std::uint64_t generations[5]{1,1,1,1,1};
    std::uintptr_t previous[5]{};
    Frame lastFrame{};
    bool hasFrame = false;
    NativeCancellation cancellation;
    bool stopAfterUpdate = false;
    bool syntheticInput = false;
    bool releaseSequence = false;
    bool committedRelease = false;
};
thread_local LocalState local;


bool InputHotfixActive() noexcept {


    static std::atomic<bool> changed{false};
    static thread_local unsigned frames = 0;
    if (changed.load(std::memory_order_acquire)) return true;
    if ((++frames % 64) != 0) return false;
    struct CodeGuard { std::uintptr_t rva; unsigned char bytes[8]; };
    static constexpr CodeGuard guards[] = {
        {0xC0B6D24, {0xfe,0x0f,0x1e,0xf8,0xf4,0x4f,0x01,0xa9}},
        {0xC914698, {0xfe,0x0f,0x1e,0xf8,0xf4,0x4f,0x01,0xa9}},
        {0xC916900, {0xfe,0x0f,0x1e,0xf8,0xf4,0x4f,0x01,0xa9}},
        {0x547496C, {0x00,0x94,0x49,0x39,0xc0,0x03,0x5f,0xd6}},
        {0x66F17EC, {0xff,0x43,0x02,0xd1,0xee,0x1b,0x00,0xfd}}
    };
    for (const auto& guard : guards) {
        unsigned char actual[8]{};
        if (!CompatibilityDetail::ReadSelf(library + guard.rva, actual, sizeof(actual)) ||
            std::memcmp(actual, guard.bytes, sizeof(actual)) != 0) {
            changed.store(true, std::memory_order_release);
            return true;
        }
    }
    return false;
}

bool HardStateBlocked(std::uintptr_t weapon) noexcept {
    const auto machine = Field<std::uintptr_t>(weapon, 0x170);
    if (!machine) return true;
    const auto current = Field<std::uintptr_t>(machine, 0x18);
    if (!current) return true;
    const int type = Field<int>(current, 0x20);
    switch (type) {


        case 3: case 4: case 6: case 7: case 11: case 12:
        case 16: case 17: case 19: case 20: case 33: case 34:
        case 35: case 36: case 37: case 38: case 39: case 45:
            return false;
        default: return true;
    }
}

bool PhysicalFire(std::uintptr_t param) noexcept {
    constexpr std::size_t fields[]{0x18,0x1a,0x1b,0x1c,0x1e,0x1f,0x20,0x24,0x25};
    for (auto offset : fields) if (Field<bool>(param, offset)) return true;


    return false;
}

ContextIdentity Identity(std::uintptr_t match, std::uintptr_t pawn,
                         std::uintptr_t weapon, std::uintptr_t component,
                         std::uintptr_t controller) noexcept {
    const std::uintptr_t now[]{match,pawn,weapon,component,controller};
    const auto epoch = lifecycle.load(std::memory_order_acquire);
    if (local.epoch != epoch) {
        local.controller.Reset();
        local.cancellation.Reset();
        local.epoch = epoch;
        for (auto& generation : local.generations) ++generation;
    }
    for (unsigned i = 0; i < 5; ++i) {
        if (local.previous[i] != now[i]) ++local.generations[i];
        local.previous[i] = now[i];
    }
    return {local.generations[0],local.generations[1],local.generations[2],
            local.generations[3],local.generations[4]};
}

void Merge(void* param, const Intent& intent) noexcept {
    const auto result = Field<std::uintptr_t>(local.scope, 0x40);
    MergePreparedInput(param, reinterpret_cast<void*>(result), intent,
        [](std::uintptr_t rva, void* object, bool value) noexcept {
            At<BoolSetter>(rva)(object, value, nullptr);
        });
}

bool PrepareViewRay(bool needed, ViewRay& ray) noexcept {
    float centerX = .5f, centerY = .5f;
    const bool ready = needed && BuildNativeRay(library,ray,centerX,centerY);
    if (ready) {
        std::uint32_t x = 0, y = 0;
        std::memcpy(&x,&centerX,sizeof(x)); std::memcpy(&y,&centerY,sizeof(y));
        viewport.store((static_cast<std::uint64_t>(x)<<32)|y,std::memory_order_relaxed);
    }
    viewportValid.store(ready,std::memory_order_release);
    return ready;
}

void Buttons(void* unit, void* param, Metadata metadata) {
    originalButtons(unit, param, metadata);
    if (installation.load(std::memory_order_acquire) != 2 ||
        !local.scope || local.depth != 1 || local.buttonsSampled || !unit || !param) return;
    const auto controller = local.scope;
    if (At<Getter>(0xC918E3C)(nullptr,nullptr) != controller ||
        Field<std::uintptr_t>(controller, 0x50) != reinterpret_cast<std::uintptr_t>(unit)) return;
    local.buttonsSampled = true;
    local.sampledParam = param;
    local.manualActive = PhysicalFire(reinterpret_cast<std::uintptr_t>(param));
}

void DecidePreparedInput(void* owner, void* param) {
    if (installation.load(std::memory_order_acquire) != 2 ||
        !local.scope || local.depth != 1 || local.seen || !local.buttonsSampled ||
        reinterpret_cast<std::uintptr_t>(owner) != local.scope ||
        !param || param != local.sampledParam) return;
    const auto controller = local.scope;
    local.seen = true;
    const auto settings = GetSettings();
    if (!settings.enabled && !local.controller.HasOwnedInput() &&
        !local.controller.HasPendingShot() &&
        !local.cancellation.HasPendingCleanup()) {
        local.controller.Reset();
        local.hasFrame = false;
        status.store(RuntimeStatus::Disabled,std::memory_order_release);
        detected.store(FireMode::Unsupported,std::memory_order_release);
        observedComponent.store(0,std::memory_order_release);
        ViewRay ray;
        PrepareViewRay(settings.showDot,ray);
        return;
    }
    const auto pawn = Field<std::uintptr_t>(controller, 0x70);
    const auto weapon = Field<std::uintptr_t>(controller, 0x68);
    const auto match = At<Getter>(0x801191C)(nullptr,nullptr);
    if (!match || !pawn || !weapon || pawn != At<Getter>(0x8012814)(nullptr,nullptr) ||
        !Field<bool>(pawn,0x550)) {
        local.controller.Reset();
        local.cancellation.Reset();
        detected.store(FireMode::Unsupported, std::memory_order_release);
        status.store(RuntimeStatus::WaitingForContext, std::memory_order_release);
        viewportValid.store(false, std::memory_order_release);
        observedComponent.store(0,std::memory_order_release);
        return;
    }
    if (InputHotfixActive()) {


        const bool sameOwner = local.previous[0] == match && local.previous[1] == pawn &&
            local.previous[2] == weapon && local.previous[4] == controller &&
            local.epoch == lifecycle.load(std::memory_order_acquire);
        if (local.hasFrame && sameOwner) {
            auto stopped = local.lastFrame;
            stopped.updateId = local.update;
            stopped.contextValid = false;
            stopped.manualActive = local.manualActive;
            const auto revoke = local.controller.Step(GetSettings(),stopped);


            local.stopAfterUpdate = !revoke.passthrough && revoke.up;
        }
        local.controller.Reset();
        NativeDriverFailure();
        return;
    }
    const auto stateMachine = Field<std::uintptr_t>(weapon,0x170);
    if (!stateMachine || !Field<std::uintptr_t>(stateMachine,0x18)) {
        local.controller.Reset();
        local.cancellation.Reset();
        observedComponent.store(0,std::memory_order_release);
        viewportValid.store(false,std::memory_order_release);
        status.store(RuntimeStatus::WaitingForContext,std::memory_order_release);
        return;
    }
    const auto component = At<ObjectGetter>(0x5472E48)(weapon, nullptr);
    if (!component) {
        local.controller.Reset();
        local.cancellation.Reset();
        observedComponent.store(0,std::memory_order_release);
        viewportValid.store(false,std::memory_order_release);
        status.store(RuntimeStatus::WaitingForContext,std::memory_order_release);
        return;
    }
    observedComponent.store(component,std::memory_order_release);
    Frame frame;
    frame.identity = Identity(match,pawn,weapon,component,controller);
    frame.updateId = local.update;
    frame.contextValid = component != 0;
    frame.bindingsReady = true;
    frame.manualActive = local.manualActive;
    frame.hardBlocked = !Field<bool>(pawn,0x550) || HardStateBlocked(weapon) ||
        !At<BoolGetter>(0xC914698)(controller,nullptr);
    frame.ammoAvailable = At<BoolGetter>(0x5473F58)(weapon,nullptr);
    frame.aiming = At<BoolGetter>(0x536BAE0)(pawn,nullptr);
    const float percentage = At<FloatGetter>(0x549C504)(weapon,nullptr);
    frame.fullyAimed = frame.aiming &&
        (At<BoolGetter>(0x54B5D84)(weapon,nullptr) ||
         (std::isfinite(percentage) && percentage >= 0.995f));
    const auto klass = Field<std::uintptr_t>(weapon,0);
    const auto prepare = klass ? Field<std::uintptr_t>(klass,0xD70) : 0;
    const bool knownPrepare = IsSupportedPreparation(library, prepare) &&
        IsVerifiedExecutableAddress(prepare);
    const auto readiness = QueryNativeReadiness(weapon, klass, IsVerifiedExecutableAddress);
    const bool knownReadiness = readiness.handlerVerified;
    frame.shotReady = readiness.shotReady;


    frame.shotSerial = committedShots.load(std::memory_order_acquire);
    const bool burst = At<BoolGetter>(0x5474344)(weapon,nullptr);
    const bool preparedRelease = Field<bool>(reinterpret_cast<std::uintptr_t>(param), 0x14) &&
        At<BoolGetter>(0xC916900)(controller,nullptr);
    const bool release = !knownPrepare || preparedRelease;
    const bool single = At<BoolGetter>(0x5474278)(weapon,nullptr);
    frame.fireMode = PreparedFireMode(knownPrepare, knownReadiness, frame.ammoAvailable,
                                     burst, single, preparedRelease);
    const bool allowReleaseSequence = frame.fireMode == FireMode::SingleRelease;
    if (installation.load(std::memory_order_acquire) != 2) {
        local.controller.Reset();
        return;
    }
    detected.store(frame.fireMode,std::memory_order_release);
    status.store(RuntimeStatus::Ready,std::memory_order_release);
    ViewRay ray;
    const bool needTarget = NeedsTargetQuery(settings,frame,local.controller);
    const bool rayReady = PrepareViewRay(settings.showDot || needTarget,ray);
    frame.contextValid = frame.contextValid && rayReady;
    NativeTargetQueries queries(library,match,pawn);
    TargetResult target = TargetResult::InvalidRay;
    frame.targetValid = false;
    if (needTarget && rayReady) {
        target = EvaluateTarget(queries,frame.identity,ray);
        frame.targetValid = target == TargetResult::Eligible;
    }
    const auto intent = local.controller.Step(settings,frame);
    const auto decision = local.cancellation.Resolve(frame.identity,frame.manualActive,release,intent,
                                                     allowReleaseSequence);
    if (decision.resetController) local.controller.Reset();
    local.stopAfterUpdate = decision.stopResult;
    local.syntheticInput = !decision.input.passthrough;
    local.releaseSequence = allowReleaseSequence;
    local.committedRelease = !decision.input.passthrough && decision.input.up &&
        decision.input.commitRelease;
    local.lastFrame = frame;
    local.hasFrame = true;
    if (installation.load(std::memory_order_acquire) == 2) Merge(param,decision.input);
    else local.controller.Reset();
}

void CheckFire(void* controller, void* param, Metadata metadata) {
    DecidePreparedInput(controller, param);
    originalCheckFire(controller, param, metadata);
}

void Update(void* controller, float deltaTime, Metadata metadata) {
    if (installation.load(std::memory_order_acquire) != 2) {
        originalUpdate(controller,deltaTime,metadata);
        return;
    }
    ++local.depth;
    if (local.depth != 1) {
        originalUpdate(controller,deltaTime,metadata);
        --local.depth;
        return;
    }
    local.scope = reinterpret_cast<std::uintptr_t>(controller);
    local.seen = false;
    local.buttonsSampled = false;
    local.sampledParam = nullptr;
    local.stopAfterUpdate = false;
    local.syntheticInput = false;
    local.releaseSequence = false;
    local.committedRelease = false;
    ++local.update;
    originalUpdate(controller,deltaTime,metadata);
    const bool sameOwner = local.seen && local.hasFrame && !local.lastFrame.manualActive &&
        local.previous[1] == Field<std::uintptr_t>(local.scope,0x70) &&
        local.previous[2] == Field<std::uintptr_t>(local.scope,0x68) &&
        local.epoch == lifecycle.load(std::memory_order_acquire);

    if (sameOwner && local.committedRelease &&
        committedShots.load(std::memory_order_acquire) == local.lastFrame.shotSerial) {
        const auto result = Field<std::uintptr_t>(local.scope,0x40);
        if (result && !Field<bool>(result,0x13)) {
            local.controller.RejectPendingShot();
            local.stopAfterUpdate = true;
        }
    }




    if (sameOwner && local.syntheticInput && !local.releaseSequence) {
        const auto param = Field<std::uintptr_t>(local.scope,0x38);
        if (param && Field<bool>(param,0x14) &&
            At<BoolGetter>(0xC916900)(local.scope,nullptr)) {
            const Intent unsafeUp{false,false,true,false};
            local.cancellation.Resolve(local.lastFrame.identity,false,true,unsafeUp);
            local.controller.Reset();
            local.stopAfterUpdate = true;
        }
    }
    if (local.stopAfterUpdate && sameOwner) {


        const auto result = Field<std::uintptr_t>(local.scope,0x40);
        if (result) {
            auto* object = reinterpret_cast<void*>(result);
            At<BoolSetter>(0xBA330CC)(object,false,nullptr);
            At<BoolSetter>(0xBA33178)(object,true,nullptr);
            const bool noEdge = false;
            std::memcpy(reinterpret_cast<unsigned char*>(result)+0x15,&noEdge,sizeof(noEdge));
            std::memcpy(reinterpret_cast<unsigned char*>(result)+0x16,&noEdge,sizeof(noEdge));
        }
    }
    local.scope = 0;
    if (!local.seen) {
        local.controller.Reset();
        observedComponent.store(0,std::memory_order_release);
        detected.store(FireMode::Unsupported,std::memory_order_release);
        viewportValid.store(false,std::memory_order_release);
        status.store(RuntimeStatus::WaitingForContext,std::memory_order_release);
    }
    --local.depth;
}
void Start(void* controller, void* match, Metadata metadata) {
    lifecycle.fetch_add(1,std::memory_order_acq_rel);
    observedComponent.store(0,std::memory_order_release);
    originalStart(controller,match,metadata);
}
void End(void* controller, Metadata metadata) {
    lifecycle.fetch_add(1,std::memory_order_acq_rel);
    viewportValid.store(false,std::memory_order_release);
    observedComponent.store(0,std::memory_order_release);
    detected.store(FireMode::Unsupported,std::memory_order_release);
    if (installation.load(std::memory_order_acquire) == 2)
        status.store(RuntimeStatus::WaitingForContext,std::memory_order_release);
    originalEnd(controller,metadata);
}
#endif
}

bool InstallNativeDriver(std::uintptr_t base) noexcept {
#if defined(__aarch64__)
    unsigned expected = 0;
    if (!base || !installation.compare_exchange_strong(expected,1)) return expected == 2;
    library = base;
    struct Hook { std::uintptr_t rva; void* replacement; void** original; };
    const Hook hooks[]{
        {0xC913748,reinterpret_cast<void*>(&Update),reinterpret_cast<void**>(&originalUpdate)},
        {0xC913FA4,reinterpret_cast<void*>(&Buttons),reinterpret_cast<void**>(&originalButtons)},
        {0xC916D64,reinterpret_cast<void*>(&CheckFire),reinterpret_cast<void**>(&originalCheckFire)},
        {0xC917C78,reinterpret_cast<void*>(&Start),reinterpret_cast<void**>(&originalStart)},
        {0xC917D68,reinterpret_cast<void*>(&End),reinterpret_cast<void**>(&originalEnd)}
    };
    unsigned installed = 0;
    for (const auto& hook : hooks) {
        const int result = DobbyHook(reinterpret_cast<void*>(base+hook.rva),hook.replacement,hook.original);
        if (result != 0 || !*hook.original) {
            if (result == 0) DobbyDestroy(reinterpret_cast<void*>(base+hook.rva));
            while (installed) DobbyDestroy(reinterpret_cast<void*>(base+hooks[--installed].rva));
            installation.store(3,std::memory_order_release);
            NativeDriverFailure();
            return false;
        }
        ++installed;
    }
    installation.store(2,std::memory_order_release);
    status.store(RuntimeStatus::WaitingForContext,std::memory_order_release);
    return true;
#else
    (void)base;
    return false;
#endif
}
RuntimeStatus NativeStatus() noexcept { return status.load(std::memory_order_acquire); }
FireMode NativeDetectedMode() noexcept { return detected.load(std::memory_order_acquire); }
bool NativeViewportCenter(float& x, float& y) noexcept {
    x = y = .5f;
    if (!viewportValid.load(std::memory_order_acquire)) return false;
    const auto bits = viewport.load(std::memory_order_relaxed);
    const auto xBits = static_cast<std::uint32_t>(bits>>32);
    const auto yBits = static_cast<std::uint32_t>(bits);
    std::memcpy(&x,&xBits,sizeof(x)); std::memcpy(&y,&yBits,sizeof(y));
    return true;
}
void NativeDriverFailure() noexcept {
#if defined(__aarch64__)
    installation.store(3,std::memory_order_release);
#endif
    status.store(RuntimeStatus::InvalidNativeState,std::memory_order_release);
    detected.store(FireMode::Unsupported,std::memory_order_release);
    viewportValid.store(false,std::memory_order_release);
    observedComponent.store(0,std::memory_order_release);
}
void NativeDriverPending() noexcept {
    status.store(RuntimeStatus::WaitingForBinaryVerification,std::memory_order_release);
}
void NotifyNativeShot(std::uintptr_t component) noexcept {
    if (component && component == observedComponent.load(std::memory_order_acquire))
        committedShots.fetch_add(1,std::memory_order_acq_rel);
}
}
