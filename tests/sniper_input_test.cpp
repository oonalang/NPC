#include "../src/main/jni/Trigger/NativeInputStage.h"
#include "../src/main/jni/Trigger/NativeCancellation.h"
#include <array>
#include <cstdlib>
#include <iostream>

using namespace PhantomTrigger;

namespace {
int checks = 0;
void Require(bool condition, const char* message) {
    ++checks;
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

struct InputStage {
    std::array<unsigned char, 0x30> param{};
    std::array<unsigned char, 0x20> result{};
    void Prepare() {
        // Native button processing first resets/copies physical edges, then the
        // sniper override marks this parameter as release-to-fire.
        param.fill(0);
        result.fill(0);
        param[0x14] = 1;
    }
    void Merge(const Intent& intent) {
        MergePreparedInput(param.data(), result.data(), intent,
            [](std::uintptr_t rva, void* object, bool value) noexcept {
                auto* bytes = static_cast<unsigned char*>(object);
                switch (rva) {
                    case 0xBA32E68: bytes[0x1C] = value; break;
                    case 0xBA32E30: bytes[0x18] = value; break;
                    case 0xBA32EE0: bytes[0x24] = value; break;
                    case 0xBA32EF0: bytes[0x25] = value; break;
                    default: std::abort();
                }
            });
    }
    bool NativeReleaseShot() const { return param[0x14] && param[0x1A]; }
};

Frame Ready() {
    Frame f;
    f.identity = {1,2,3,4,5};
    f.updateId = 1;
    f.contextValid = f.bindingsReady = f.ammoAvailable = f.shotReady = true;
    f.aiming = f.fullyAimed = f.targetValid = true;
    f.fireMode = FireMode::SingleRelease;
    return f;
}
}

int main() {
    constexpr std::uintptr_t base = 0x10000000;
    Require(IsSupportedPreparation(base, base + 0x54A373C), "base preparation rejected");
    Require(IsSupportedPreparation(base, base + 0xC38D790), "COD sniper preparation rejected");
    Require(!IsSupportedPreparation(base, base + 0xC07D0C4), "charged Tempest admitted");
    Require(!IsSupportedPreparation(base, base - 4), "invalid entry admitted");
    Require(PreparedFireMode(true,true,true,false,true,true) == FireMode::SingleRelease,
            "prepared sniper release mode not recognized");
    Require(PreparedFireMode(true,true,true,false,true,false) == FireMode::Single,
            "press-to-fire single mode changed");
    Require(PreparedFireMode(true,true,true,false,false,false) == FireMode::Automatic,
            "automatic rifle mode changed");
    Require(PreparedFireMode(true,true,true,true,true,true) == FireMode::Unsupported,
            "burst admitted");
    Require(PreparedFireMode(false,true,true,false,true,true) == FireMode::Unsupported,
            "unknown preparation admitted");
    Require(PreparedFireMode(true,true,true,false,false,true) == FireMode::SingleRelease,
            "prepared release incorrectly depends on ordinary single-shot flag");

    const Settings enabled{true, Mode::Both, false};
    Controller controller;
    NativeCancellation cancellation;
    InputStage native;
    auto frame = Ready();
    native.Prepare();
    auto intent = controller.Step(enabled, frame);
    auto decision = cancellation.Resolve(frame.identity,false,true,intent,true);
    native.Merge(decision.input);
    Require(native.param[0x14] && native.param[0x1C] && native.param[0x18],
            "prepared sniper press was not merged");
    Require(native.result[0x15] && !native.result[0x16],
            "cached native down edge was not refreshed");
    Require(!native.NativeReleaseShot(), "arming press fired without release");

    native.Prepare();
    ++frame.updateId;
    intent = controller.Step(enabled, frame);
    Require(intent.commitRelease, "fresh eligible frame did not commit release");
    decision = cancellation.Resolve(frame.identity,false,true,intent,true);
    native.Merge(decision.input);
    Require(native.NativeReleaseShot(), "native release consumer did not receive up edge");
    Require(!native.result[0x15] && native.result[0x16],
            "cached native release edge was not refreshed");
    ++frame.updateId;
    Require(controller.Step(enabled, frame).passthrough,
            "release shot rearmed without acknowledgement");

    controller.Reset();
    cancellation.Reset();
    frame = Ready();
    controller.Step(enabled,frame);
    ++frame.updateId;
    frame.targetValid = false;
    native.Prepare();
    intent = controller.Step(enabled,frame);
    decision = cancellation.Resolve(frame.identity,false,true,intent,true);
    native.Merge(decision.input);
    Require(decision.stopResult && !native.NativeReleaseShot(),
            "target loss generated a sniper release shot");
    Require(!native.result[0x16], "target loss exposed a release result edge");

    // Passthrough must retain real user edges, including a manual release.
    native.param[0x1A] = native.param[0x25] = native.result[0x16] = 1;
    native.Merge(Intent{});
    Require(native.NativeReleaseShot() && native.result[0x16],
            "passthrough clobbered physical release");

    // A late native gate may refuse the release. There will be no shot ack;
    // the feedback must insert a neutral frame and allow a later valid retry.
    controller.Reset();
    frame = Ready();
    controller.Step(enabled,frame);
    ++frame.updateId;
    Require(controller.Step(enabled,frame).commitRelease, "release not submitted for rejection test");
    controller.RejectPendingShot();
    ++frame.updateId;
    intent = controller.Step(enabled,frame);
    Require(intent.passthrough && !intent.up && !intent.down && !controller.HasPendingShot(),
            "native rejection failed to rearm neutrally without an ack");
    ++frame.updateId;
    Require(controller.Step(enabled,frame).down,
            "native rejection left the sniper permanently awaiting an ack");
    std::cout << "Passed " << checks << " prepared input checks\n";
}
