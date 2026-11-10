#include "../src/main/jni/Trigger/NativeCancellation.h"
#include "../src/main/jni/Trigger/TriggerRange.h"
#include "../src/main/jni/Trigger/TriggerRuntime.h"

#include <cstdlib>
#include <iostream>
#include <limits>

using namespace PhantomTrigger;

namespace {
int checks = 0;

void Require(bool condition, const char* message) {
    ++checks;
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

Frame Ready(FireMode mode = FireMode::Automatic) {
    Frame frame;
    frame.identity = {1, 2, 3, 4, 5};
    frame.updateId = 1;
    frame.contextValid = true;
    frame.bindingsReady = true;
    frame.ammoAvailable = true;
    frame.targetValid = true;
    frame.shotReady = true;
    frame.fireMode = mode;
    return frame;
}

struct Queries final : TargetQueries {
    BodyHit body{true, true, true, true, true, 7, {0, 0, 20}, 20};
    bool available = true;
    Occlusion world = Occlusion::Clear;
    Occlusion smoke = Occlusion::Clear;
    int bodyCalls = 0;
    int worldCalls = 0;
    int smokeCalls = 0;

    bool TraceBody(const ContextIdentity&, const ViewRay&, BodyHit& result) noexcept override {
        ++bodyCalls;
        result = body;
        return available;
    }
    Occlusion CheckWorld(const ContextIdentity&, const ViewRay&, const BodyHit&) noexcept override {
        ++worldCalls;
        return world;
    }
    Occlusion CheckSmoke(const ContextIdentity&, const ViewRay&, const BodyHit&) noexcept override {
        ++smokeCalls;
        return smoke;
    }
};

const Settings enabled{true, Mode::Both, false};
const ViewRay centerRay{{0, 0, 0}, {0, 0, 1}, 100};

void SingleShotAcknowledgement() {
    Controller controller;
    Frame frame = Ready(FireMode::Single);
    auto intent = controller.Step(enabled, frame);
    Require(intent.held && intent.down && !intent.passthrough, "single shot presses once");
    ++frame.updateId;
    frame.shotReady = false;
    intent = controller.Step(enabled, frame);
    Require(intent.held && !intent.down && !intent.up, "single shot waits for acknowledgement");
    ++frame.updateId;
    ++frame.shotSerial;
    intent = controller.Step(enabled, frame);
    Require(intent.up && !intent.held && !intent.down, "confirmed shot releases the press");
    ++frame.updateId;
    intent = controller.Step(enabled, frame);
    Require(intent.passthrough, "cooldown prevents another single shot");
    ++frame.updateId;
    frame.shotReady = true;
    intent = controller.Step(enabled, frame);
    Require(intent.down && intent.held, "ready single shot rearms after release");
}

void OwnershipAndLifecycle() {
    Controller controller;
    Frame frame = Ready();
    auto intent = controller.Step(enabled, frame);
    Require(intent.down && intent.held, "automatic target acquires a press");
    intent = controller.Step(enabled, frame);
    Require(intent.passthrough, "duplicate input update does not press twice");
    ++frame.updateId;
    intent = controller.Step(enabled, frame);
    Require(intent.held && !intent.down, "automatic fire holds across valid frames");
    ++frame.updateId;
    frame.targetValid = false;
    intent = controller.Step(enabled, frame);
    Require(intent.up && !intent.held, "target loss releases owned input");
    ++frame.updateId;
    intent = controller.Step(enabled, frame);
    Require(intent.passthrough, "target absence does not generate repeated release edges");

    ++frame.updateId;
    frame.targetValid = true;
    controller.Step(enabled, frame);
    ++frame.updateId;
    frame.manualActive = true;
    intent = controller.Step(enabled, frame);
    Require(intent.passthrough && !intent.up && !controller.HasOwnedInput(),
            "manual fire takes ownership without an injected release");
    ++frame.updateId;
    frame.manualActive = false;
    intent = controller.Step(enabled, frame);
    Require(intent.down, "manual release allows a fresh automatic press");

    ++frame.identity.weapon;
    frame.updateId = 1;
    intent = controller.Step(enabled, frame);
    Require(intent.down && !intent.up, "weapon change starts a fresh input lifetime");
    ++frame.updateId;
    frame.contextValid = false;
    intent = controller.Step(enabled, frame);
    Require(intent.up, "context invalidation releases input in the same lifetime");
}

void AdsAndModeChanges() {
    Controller controller;
    Frame frame = Ready();
    const Settings ads{true, Mode::ADSOnly, false};
    Require(controller.Step(ads, frame).passthrough, "ADS mode waits for aiming");
    ++frame.updateId;
    frame.aiming = true;
    Require(controller.Step(ads, frame).passthrough, "ADS mode waits for completed aim transition");
    ++frame.updateId;
    frame.fullyAimed = true;
    Require(controller.Step(ads, frame).down, "completed ADS acquires input");
    ++frame.updateId;
    Require(controller.Step(enabled, frame).up, "mode change releases existing input first");
    ++frame.updateId;
    Require(controller.Step(enabled, frame).down, "new mode acquires input on the next update");
    ++frame.updateId;
    Require(controller.Step(Settings{}, frame).up, "disabling releases owned fire");
}

void TargetValidation() {
    const auto identity = Ready().identity;
    Queries queries;
    Require(EvaluateTarget(queries, identity, centerRay) == TargetResult::Eligible,
            "an exact visible body intersection is eligible");
    queries.world = Occlusion::Blocked;
    Require(EvaluateTarget(queries, identity, centerRay) == TargetResult::WorldBlocked,
            "world geometry blocks a valid body");
    Require(queries.smokeCalls == 1, "world obstruction skips the smoke query");
    queries.world = Occlusion::Clear;
    queries.smoke = Occlusion::Blocked;
    Require(EvaluateTarget(queries, identity, centerRay) == TargetResult::SmokeBlocked,
            "smoke blocks an otherwise visible body");
    queries.smoke = Occlusion::Unavailable;
    Require(EvaluateTarget(queries, identity, centerRay) == TargetResult::QueryUnavailable,
            "missing smoke data prevents firing");
    queries.smoke = Occlusion::Clear;
    queries.body.point.x = 0.01f;
    Require(EvaluateTarget(queries, identity, centerRay) == TargetResult::InvalidHit,
            "off-crosshair geometry is rejected");
    queries.body.point.x = 0;
    queries.body.distance = 21;
    Require(EvaluateTarget(queries, identity, centerRay) == TargetResult::InvalidHit,
            "inconsistent intersection distance is rejected");
    queries.body.distance = 20;
    queries.body.enemy = false;
    Require(EvaluateTarget(queries, identity, centerRay) == TargetResult::NoEligibleBody,
            "friendly bodies do not trigger");
    queries.body.enemy = true;
    queries.body.alive = false;
    Require(EvaluateTarget(queries, identity, centerRay) == TargetResult::NoEligibleBody,
            "dead bodies do not trigger");
    queries.body.alive = true;
    queries.available = false;
    Require(EvaluateTarget(queries, identity, centerRay) == TargetResult::QueryUnavailable,
            "failed trace prevents firing");
    ViewRay invalid = centerRay;
    invalid.direction.z = std::numeric_limits<float>::quiet_NaN();
    const auto before = queries.bodyCalls;
    Require(EvaluateTarget(queries, identity, invalid) == TargetResult::InvalidRay &&
            queries.bodyCalls == before, "nonfinite rays are rejected before native queries");
    invalid = centerRay;
    invalid.direction.z = 2;
    Require(EvaluateTarget(queries, identity, invalid) == TargetResult::InvalidRay,
            "unnormalized rays are rejected");
}

void InputQueryBudget() {
    Controller controller;
    Frame frame = Ready();
    Queries queries;
    Require(EvaluateInput(Settings{}, frame, centerRay, queries, controller).passthrough &&
            queries.bodyCalls == 0, "disabled trigger performs no target queries");
    ++frame.updateId;
    frame.manualActive = true;
    Require(EvaluateInput(enabled, frame, centerRay, queries, controller).passthrough &&
            queries.bodyCalls == 0, "manual fire performs no target queries");
    ++frame.updateId;
    frame.manualActive = false;
    Require(EvaluateInput(enabled, frame, centerRay, queries, controller).down &&
            queries.bodyCalls == 1, "eligible frame queries and presses");
    ++frame.updateId;
    queries.body.alive = false;
    Require(EvaluateInput(enabled, frame, centerRay, queries, controller).up,
            "target invalidation propagates to input release");

    Controller single;
    frame = Ready(FireMode::Single);
    frame.shotReady = false;
    const auto before = queries.bodyCalls;
    Require(EvaluateInput(enabled, frame, centerRay, queries, single).passthrough &&
            queries.bodyCalls == before, "single-shot cooldown skips idle target queries");
}

void CancellationAndSettings() {
    NativeCancellation cancellation;
    const auto identity = Ready().identity;
    const Intent up{false, false, true, false};
    auto decision = cancellation.Resolve(identity, false, true, up);
    Require(decision.input.passthrough && decision.stopResult && cancellation.HasPendingCleanup(),
            "release-to-fire cancellation suppresses a dangerous release edge");
    decision = cancellation.Resolve(identity, false, false, Intent{});
    Require(decision.input.up && decision.resetController && !cancellation.HasPendingCleanup(),
            "safe weapon state completes pending input cleanup");
    cancellation.Resolve(identity, false, true, up);
    decision = cancellation.Resolve(identity, true, false, up);
    Require(decision.input.passthrough && !decision.stopResult && !cancellation.HasPendingCleanup(),
            "manual ownership cancels pending synthetic cleanup");
    cancellation.Resolve(identity, false, true, up);
    auto changed = identity;
    ++changed.weapon;
    decision = cancellation.Resolve(changed, false, false, Intent{});
    Require(decision.input.passthrough && !decision.resetController,
            "pending cleanup cannot cross weapon lifetimes");

    SetSettings({true, Mode::ADSOnly, true});
    auto settings = GetSettings();
    Require(settings.enabled && settings.mode == Mode::ADSOnly && settings.showDot,
            "settings round trip as one coherent value");
    SetSettings({true, static_cast<Mode>(255), false});
    Require(!GetSettings().enabled, "invalid mode disables trigger");
    SetSettings({});
    Require(VisibleTraceDistance(0.3f, 300) > 299 &&
            VisibleTraceDistance(10, 5) == 0 &&
            VisibleTraceDistance(0, std::numeric_limits<float>::infinity()) == 0,
            "trace range follows valid camera clipping planes");
}
}

int main() {
    SingleShotAcknowledgement();
    OwnershipAndLifecycle();
    AdsAndModeChanges();
    TargetValidation();
    InputQueryBudget();
    CancellationAndSettings();
    std::cout << "Passed " << checks << " trigger behavior checks\n";
}
