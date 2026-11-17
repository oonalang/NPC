#include "../src/main/jni/Trigger/NativeCancellation.h"
#include "../src/main/jni/Trigger/NativeReadiness.h"
#include "../src/main/jni/Trigger/NativeRay.h"
#include "../src/main/jni/Trigger/TriggerRange.h"
#include "../src/main/jni/Trigger/TriggerRuntime.h"

#include <cstdlib>
#include <array>
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

struct ReadinessWeapon {
    const void* expectedMetadata;
    int calls = 0;
};

bool BaseReadiness(std::uintptr_t object, const void* metadata) {
    auto& weapon = *reinterpret_cast<ReadinessWeapon*>(object);
    ++weapon.calls;
    return metadata == weapon.expectedMetadata;
}

bool RestrictedReadiness(std::uintptr_t object, const void* metadata) {
    auto& weapon = *reinterpret_cast<ReadinessWeapon*>(object);
    ++weapon.calls;
    Require(metadata == weapon.expectedMetadata, "virtual readiness receives its MethodInfo");
    return false;
}

void VirtualWeaponReadiness() {
    std::array<unsigned char, 0x1200> klass{};
    const int methodInfo = 274;
    const void* metadata = &methodInfo;
    ReadinessWeapon weapon{metadata};
    auto entry = reinterpret_cast<std::uintptr_t>(&BaseReadiness);
    std::memcpy(klass.data() + 0x11F0, &entry, sizeof(entry));
    std::memcpy(klass.data() + 0x11F8, &metadata, sizeof(metadata));
    const auto object = reinterpret_cast<std::uintptr_t>(&weapon);
    const auto table = reinterpret_cast<std::uintptr_t>(klass.data());
    const auto executable = [](std::uintptr_t address) noexcept {
        return address == reinterpret_cast<std::uintptr_t>(&BaseReadiness) ||
               address == reinterpret_cast<std::uintptr_t>(&RestrictedReadiness);
    };
    auto binding = BindNativeReadiness(table, executable);
    Require(binding.IsVerified() && weapon.calls == 0,
            "binding native readiness does not run its gameplay predicate");
    Require(binding.Sample(object) && weapon.calls == 1,
            "base readiness uses its native virtual entry and metadata");

    entry = reinterpret_cast<std::uintptr_t>(&RestrictedReadiness);
    std::memcpy(klass.data() + 0x11F0, &entry, sizeof(entry));
    binding = BindNativeReadiness(table, executable);
    Require(binding.IsVerified() && !binding.Sample(object) && weapon.calls == 2,
            "subtype readiness can block a shot that base readiness permits");

    binding = BindNativeReadiness(table, [](std::uintptr_t) noexcept { return false; });
    Require(!binding.IsVerified() && !binding.Sample(object) && weapon.calls == 2,
            "an unverified virtual entry is never invoked");
    binding = BindNativeReadiness(0, executable);
    Require(!binding.IsVerified() && !binding.Sample(object) && weapon.calls == 2,
            "missing class metadata leaves readiness unavailable");
    binding = BindNativeReadiness(table, executable);
    Require(!binding.Sample(0) && weapon.calls == 2,"null weapon cannot invoke a bound method");
}

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

Frame ReleaseReady();

void AcknowledgedShotWork() {
    for (const bool callback : {false,true}) {
        Controller controller;
        Queries queries;
        auto frame=ReleaseReady();frame.clipAmmo=5;
        EvaluateInput(enabled,frame,centerRay,queries,controller);
        ++frame.updateId;
        EvaluateInput(enabled,frame,centerRay,queries,controller);
        const auto before=queries.bodyCalls;
        ++frame.updateId;
        if(callback) ++frame.shotSerial; else --frame.clipAmmo;
        const auto intent=EvaluateInput(enabled,frame,centerRay,queries,controller);
        Require(intent.down&&intent.held&&!intent.up&&queries.bodyCalls==before+1,
                "confirmed release rearms on the first ready update with fresh geometry");
        Require(!EvaluateInput(enabled,frame,centerRay,queries,controller).down&&queries.bodyCalls==before+1,
                "confirmed rearm is idempotent within one update");
    }
    for (const auto mode : {FireMode::Single,FireMode::BurstSingle}) {
        Controller controller;Queries queries;auto frame=Ready(mode);frame.clipAmmo=5;
        EvaluateInput(enabled,frame,centerRay,queries,controller);
        const auto before=queries.bodyCalls;
        ++frame.updateId;--frame.clipAmmo;
        const auto intent=EvaluateInput(enabled,frame,centerRay,queries,controller);
        Require(intent.up&&!intent.down&&!intent.held&&queries.bodyCalls==before,
                "confirmed press-fire releases without an unnecessary target trace");
        ++frame.updateId;queries.world=Occlusion::Blocked;
        Require(EvaluateInput(enabled,frame,centerRay,queries,controller).passthrough&&queries.bodyCalls==before+1,
                "press-fire rearming still checks current obstruction");
    }
}

void BothModeCadence() {
    for (const auto rate : {30u,60u,90u,120u,144u}) {
        for (const auto mode : {FireMode::Automatic,FireMode::Single,FireMode::SingleRelease,
                                FireMode::BurstAutomatic,FireMode::BurstSingle}) {
            for (const auto setting : {Mode::Both,Mode::ADSOnly}) {
                Controller controller;Queries queries;
                auto frame=mode==FireMode::SingleRelease?ReleaseReady():Ready(mode);
                frame.fireMode=mode;frame.aiming=frame.fullyAimed=setting==Mode::ADSOnly||mode==FireMode::SingleRelease;
                frame.nowMillis=7000;frame.clipAmmo=5;
                const Settings settings{true,setting,false};
                queries.body.alive=false;
                Require(EvaluateInput(settings,frame,centerRay,queries,controller).passthrough,
                        "weapon does not prearm before crosshair entry");
                ++frame.updateId;frame.nowMillis+=1000/rate;queries.body.alive=true;
                auto intent=EvaluateInput(settings,frame,centerRay,queries,controller);
                Require(intent.down&&intent.held&&!intent.up,
                        "every supported input mode presses on the first eligible update without a reaction timer");
                const auto scans=queries.bodyCalls;
                Require(EvaluateInput(settings,frame,centerRay,queries,controller).passthrough&&queries.bodyCalls==scans,
                        "first eligible update cannot duplicate input or target work");
                ++frame.updateId;frame.nowMillis+=1000/rate;
                if(mode==FireMode::SingleRelease) {
                    intent=EvaluateInput(settings,frame,centerRay,queries,controller);
                    Require(intent.commitRelease&&queries.bodyCalls==scans+1,
                            "release-fire keeps distinct press and freshly checked release updates");
                    ++frame.updateId;frame.nowMillis+=1000/rate;--frame.clipAmmo;
                    Require(EvaluateInput(settings,frame,centerRay,queries,controller).down,
                            "ready release-fire rearming has no acknowledgement-only idle update");
                } else if(mode==FireMode::Single||mode==FireMode::BurstSingle) {
                    --frame.clipAmmo;
                    intent=EvaluateInput(settings,frame,centerRay,queries,controller);
                    Require(intent.up&&queries.bodyCalls==scans,
                            "press-fire acknowledgement releases without body/world/smoke work");
                    ++frame.updateId;frame.nowMillis+=1000/rate;
                    Require(EvaluateInput(settings,frame,centerRay,queries,controller).down,
                            "ready press-fire reacquires immediately after its required up update");
                } else {
                    intent=EvaluateInput(settings,frame,centerRay,queries,controller);
                    Require(intent.held&&!intent.down&&queries.bodyCalls==scans+1,
                            "automatic input remains continuous with fresh geometry every update");
                }
            }
        }
    }
}

void AcknowledgedReleaseGuards() {
    for (const bool callback : {false,true}) {
        for (int failure=0;failure<11;++failure) {
            Controller controller;Queries queries;auto frame=ReleaseReady();frame.clipAmmo=5;
            auto settings=enabled;
            EvaluateInput(settings,frame,centerRay,queries,controller);++frame.updateId;
            EvaluateInput(settings,frame,centerRay,queries,controller);++frame.updateId;
            if(callback) ++frame.shotSerial; else --frame.clipAmmo;
            const auto before=queries.bodyCalls;
            switch(failure) {
                case 0: queries.body.alive=false; break;
                case 1: queries.world=Occlusion::Blocked; break;
                case 2: queries.smoke=Occlusion::Blocked; break;
                case 3: frame.fullyAimed=false; break;
                case 4: frame.shotReady=false; break;
                case 5: frame.manualActive=true; break;
                case 6: settings.enabled=false; break;
                case 7: frame.contextValid=false; break;
                case 8: frame.hardBlocked=true; break;
                case 9: frame.ammoAvailable=false; break;
                case 10: queries.available=false; break;
            }
            const auto intent=EvaluateInput(settings,frame,centerRay,queries,controller);
            Require(!intent.down&&!intent.up&&!intent.commitRelease&&!controller.HasPendingShot(),
                    "acknowledgement cannot bypass current aim/cooldown/input/context/target checks");
            if(failure>=3&&failure<=9)
                Require(queries.bodyCalls==before,"blocked acknowledgement does not run target traces");
            else Require(queries.bodyCalls==before+1,"ready acknowledgement tests current geometry rather than a cached hit");
        }
    }
}

void WeaponStateWorkBudget() {
    Controller controller;auto frame=Ready();
    Require(!NeedsAimState(enabled,frame,controller)&&NeedsWeaponReadiness(enabled,frame,controller),
            "Both-mode automatic input samples native readiness without unused ADS queries");
    frame.fireMode=FireMode::Single;
    Require(!NeedsAimState(enabled,frame,controller)&&NeedsWeaponReadiness(enabled,frame,controller),
            "Both-mode press-fire samples native readiness without unused ADS queries");
    const Settings ads{true,Mode::ADSOnly,false};
    Require(NeedsAimState(ads,frame,controller)&&!NeedsWeaponReadiness(ads,frame,controller),
            "ADS Only samples aim before invoking native readiness");
    frame=ReleaseReady();
    Require(NeedsAimState(enabled,frame,controller)&&NeedsWeaponReadiness(enabled,frame,controller),
            "Both-mode release-fire retains its required native aim and readiness checks");
    controller.Step(enabled,frame);++frame.updateId;
    controller.Step(enabled,frame);++frame.updateId;
    Require(!NeedsAimState(enabled,frame,controller)&&!NeedsWeaponReadiness(enabled,frame,controller),
            "pending shot confirmation samples neither aim nor native readiness");
    ++frame.shotSerial;
    Require(NeedsAimState(enabled,frame,controller)&&NeedsWeaponReadiness(enabled,frame,controller),
            "new confirmation permits fresh native state checks without a timer delay");
    for(int failure=0;failure<7;++failure) {
        Controller fresh;auto sample=ReleaseReady();auto settings=enabled;
        switch(failure) {
            case 0: settings.enabled=false; break;
            case 1: sample.contextValid=false; break;
            case 2: sample.bindingsReady=false; break;
            case 3: sample.manualActive=true; break;
            case 4: sample.ammoAvailable=false; break;
            case 5: sample.hardBlocked=true; break;
            case 6: sample.fireMode=FireMode::Unsupported; break;
        }
        Require(!NeedsAimState(settings,sample,fresh)&&!NeedsWeaponReadiness(settings,sample,fresh),
                "ineligible input avoids unused aim and readiness calls");
    }
}

struct CameraFixture {
    bool valid=true;
    float nearPlane=.3f,farPlane=300.f;
    int width=2400,height=1080;
    NativeCameraRay native{{1,2,3},{0,0,1}};
    Point3 screen{1200,540,0};
    int nearCalls=0,farCalls=0,rayCalls=0,widthCalls=0,heightCalls=0,screenCalls=0;
    bool IsValid() const { return valid; }
    float NearPlane() { ++nearCalls;return nearPlane; }
    float FarPlane() { ++farCalls;return farPlane; }
    NativeCameraRay ViewportRay(Point3 point) {
        ++rayCalls;
        Require(point.x==.5f&&point.y==.5f&&point.z==0,"camera ray uses the exact viewport center");
        return native;
    }
    int Width() { ++widthCalls;return width; }
    int Height() { ++heightCalls;return height; }
    Point3 ScreenPoint(Point3) { ++screenCalls;return screen; }
};

void CameraWorkBudget() {
    CameraFixture camera;ViewRay ray;float x=0,y=0;
    Require(SampleNativeViewRay(camera,ray,x,y,false)&&ray.origin.x==1&&ray.direction.z==1,
            "hidden overlay still builds a valid fresh native camera ray");
    Require(camera.widthCalls==0&&camera.heightCalls==0&&camera.screenCalls==0&&
            camera.nearCalls==1&&camera.farCalls==1&&camera.rayCalls==1,
            "hidden overlay avoids all three screen-coordinate calls");
    Require(SampleNativeViewRay(camera,ray,x,y,true)&&x==.5f&&y==.5f&&
            camera.widthCalls==1&&camera.heightCalls==1&&camera.screenCalls==1,
            "visible overlay preserves projected screen coordinates");
    camera.native.origin={4,5,6};camera.native.direction={0,1.01f,0};
    Require(SampleNativeViewRay(camera,ray,x,y,false)&&ray.origin.x==4&&
            std::fabs(ray.direction.y-1.f)<1e-6f&&camera.rayCalls==3,
            "successive updates sample a changed camera and normalize direction without reusing a ray");
    camera.width=0;
    Require(!SampleNativeViewRay(camera,ray,x,y,true)&&ray.maxDistance==0&&camera.screenCalls==1,
            "visible overlay rejects invalid screen bounds before projection");
    Require(SampleNativeViewRay(camera,ray,x,y,false),"world ray does not depend on hidden overlay dimensions");
    camera.native.direction.z=std::numeric_limits<float>::quiet_NaN();
    const auto screens=camera.screenCalls;
    Require(!SampleNativeViewRay(camera,ray,x,y,true)&&ray.maxDistance==0&&camera.screenCalls==screens,
            "invalid native rays are rejected before any projection calls");
    camera=CameraFixture{};camera.screen.x=3000;
    Require(!SampleNativeViewRay(camera,ray,x,y,true)&&ray.maxDistance==0,
            "off-screen projected crosshair remains invalid");
    camera=CameraFixture{};camera.farPlane=.1f;
    Require(!SampleNativeViewRay(camera,ray,x,y,false)&&camera.rayCalls==0,
            "invalid clipping planes do not invoke the camera ray getter");
    camera=CameraFixture{};camera.valid=false;
    Require(!SampleNativeViewRay(camera,ray,x,y,false)&&camera.nearCalls==0&&camera.rayCalls==0,
            "missing camera does no dependent native work");
}

void MissingAcknowledgementRecovery() {
    for (const auto mode : {FireMode::Single,FireMode::SingleRelease}) {
        Controller controller;
        Queries queries;
        auto frame = mode==FireMode::SingleRelease?ReleaseReady():Ready(mode);
        frame.nowMillis=1000;frame.clipAmmo=5;
        Require(EvaluateInput(enabled,frame,centerRay,queries,controller).down,"recovery sequence starts with a press");
        if(mode==FireMode::SingleRelease) {
            ++frame.updateId;frame.nowMillis=1010;
            Require(EvaluateInput(enabled,frame,centerRay,queries,controller).commitRelease,"recovery test commits one release");
        }
        const auto submitted=frame.nowMillis;
        for(unsigned i=1;i<500;++i) {
            ++frame.updateId;frame.nowMillis=submitted+i;
            const auto intent=EvaluateInput(enabled,frame,centerRay,queries,controller);
            Require(!intent.down&&!intent.up,"missing callback cannot replay a firing edge before its deadline");
        }
        const auto calls=queries.bodyCalls;
        if(mode==FireMode::SingleRelease) Require(calls==2,"pending release does no repeated body/world/smoke scans");
        ++frame.updateId;frame.nowMillis=submitted+Controller::shotAcknowledgementTimeoutMillis;
        auto intent=EvaluateInput(enabled,frame,centerRay,queries,controller);
        Require(!intent.passthrough&&!intent.down&&!intent.held&&!intent.commitRelease,
                "expired acknowledgement clears synthetic input without committing a release");
        Require(intent.up==(mode==FireMode::Single),"only a press-fire weapon gets a safe release on timeout");
        Require(!controller.HasOwnedInput()&&!controller.HasPendingShot(),"missing callback cannot deadlock the controller");
        Require(EvaluateInput(enabled,frame,centerRay,queries,controller).passthrough,"timeout cleanup is not repeated in the same update");
        ++frame.updateId;++frame.nowMillis;
        Require(EvaluateInput(enabled,frame,centerRay,queries,controller).down,"ready weapon reacquires using a fresh target after neutral recovery");
    }
}

void AmmoAcknowledgementAndQueryBudget() {
    Controller controller;
    Queries queries;
    auto frame=ReleaseReady();frame.clipAmmo=6;frame.nowMillis=5000;
    EvaluateInput(enabled,frame,centerRay,queries,controller);
    const auto first=queries.bodyCalls;
    EvaluateInput(enabled,frame,centerRay,queries,controller);
    Require(queries.bodyCalls==first,"duplicate input update performs no duplicate target scan");
    ++frame.updateId;++frame.nowMillis;
    EvaluateInput(enabled,frame,centerRay,queries,controller);
    const auto calls=queries.bodyCalls;
    ++frame.updateId;++frame.nowMillis;frame.clipAmmo=5;frame.shotReady=false;
    const auto acknowledged=EvaluateInput(enabled,frame,centerRay,queries,controller);
    Require(acknowledged.passthrough&&!controller.HasPendingShot()&&queries.bodyCalls==calls,
            "native clip consumption confirms a missing callback without a target scan or firing edge");
    ++frame.updateId;++frame.nowMillis;frame.shotReady=false;
    Require(EvaluateInput(enabled,frame,centerRay,queries,controller).passthrough&&queries.bodyCalls==calls,
            "ammo acknowledgement still respects the weapon cooldown");
    ++frame.updateId;++frame.nowMillis;frame.shotReady=true;queries.body.alive=false;
    Require(EvaluateInput(enabled,frame,centerRay,queries,controller).passthrough&&queries.bodyCalls==calls+1,
            "rearming never reuses an earlier enemy result");
    controller.Reset();queries=Queries{};frame=ReleaseReady();frame.clipAmmo=5;
    EvaluateInput(enabled,frame,centerRay,queries,controller);++frame.updateId;
    EvaluateInput(enabled,frame,centerRay,queries,controller);++frame.updateId;frame.clipAmmo=6;
    EvaluateInput(enabled,frame,centerRay,queries,controller);
    Require(controller.HasPendingShot(),"an ammo increase is not mistaken for a shot confirmation");
    ++frame.updateId;frame.clipAmmo=-1;
    EvaluateInput(enabled,frame,centerRay,queries,controller);
    Require(controller.HasPendingShot(),"invalid ammo does not acknowledge a shot");
}

void SniperReactionCadence() {
    for (const auto rate : {30u,60u,120u}) {
        Controller controller;
        Queries queries;
        auto frame=ReleaseReady();frame.nowMillis=10000;frame.clipAmmo=5;
        queries.body.alive=false;
        Require(EvaluateInput(enabled,frame,centerRay,queries,controller).passthrough,
                "sniper does not prearm without an eligible enemy");
        ++frame.updateId;frame.nowMillis=10000+1000/rate;queries.body.alive=true;
        auto intent=EvaluateInput(enabled,frame,centerRay,queries,controller);
        Require(intent.down&&intent.held,"sniper acquires on the first eligible input update at every tested frame rate");
        const auto before=queries.bodyCalls;
        ++frame.updateId;frame.nowMillis=10000+2000/rate;
        intent=EvaluateInput(enabled,frame,centerRay,queries,controller);
        Require(intent.commitRelease&&queries.bodyCalls==before+1,
                "sniper release uses the immediately following freshly checked input update");
        const auto submitted=frame.nowMillis;
        for(unsigned tick=1;tick<=rate/2;++tick) {
            ++frame.updateId;frame.nowMillis=submitted+tick*1000/rate;
            intent=EvaluateInput(enabled,frame,centerRay,queries,controller);
        }
        Require(!intent.down&&!intent.up&&!intent.held&&!controller.HasPendingShot(),
                "missing-confirmation recovery is time based rather than frame-rate dependent");
        ++frame.updateId;++frame.nowMillis;frame.manualActive=true;
        Require(EvaluateInput(enabled,frame,centerRay,queries,controller).passthrough,
                "manual takeover has priority immediately after a timeout");
        ++frame.identity.weapon;frame.updateId=1;frame.manualActive=false;frame.nowMillis=1;
        Require(EvaluateInput(enabled,frame,centerRay,queries,controller).down,
                "new weapon lifetime cannot inherit an expired deadline or shot evidence");
    }
    Controller controller;Queries queries;auto frame=ReleaseReady();frame.nowMillis=1000;
    EvaluateInput(enabled,frame,centerRay,queries,controller);++frame.updateId;
    EvaluateInput(enabled,frame,centerRay,queries,controller);++frame.updateId;
    frame.nowMillis=1500;frame.shotSerial=1;
    Require(EvaluateInput(enabled,frame,centerRay,queries,controller).down,
            "real acknowledgement permits ready rearming at the timeout boundary");
}

Frame ReleaseReady() {
    auto frame = Ready(FireMode::SingleRelease);
    frame.aiming = true;
    frame.fullyAimed = true;
    return frame;
}

void ReleaseShotAcknowledgement() {
    Controller controller;
    NativeCancellation cancellation;
    Queries queries;
    auto frame = ReleaseReady();
    auto intent = EvaluateInput(enabled, frame, centerRay, queries, controller);
    auto decision = cancellation.Resolve(frame.identity, false, true, intent, true);
    Require(decision.input.down && decision.input.held && !decision.input.up &&
            !decision.input.commitRelease && !decision.stopResult,
            "release-shot weapon starts with an allowed press");
    Require(EvaluateInput(enabled, frame, centerRay, queries, controller).passthrough,
            "duplicate update cannot commit a release shot");

    ++frame.updateId;
    const auto bodyCalls = queries.bodyCalls;
    intent = EvaluateInput(enabled, frame, centerRay, queries, controller);
    decision = cancellation.Resolve(frame.identity, false, true, intent, true);
    Require(queries.bodyCalls == bodyCalls + 1 && decision.input.up &&
            decision.input.commitRelease && !decision.input.held &&
            !decision.stopResult && !controller.HasOwnedInput() && controller.HasPendingShot(),
            "fresh eligible target commits release before shot acknowledgement");
    ++frame.updateId;
    intent = EvaluateInput(enabled, frame, centerRay, queries, controller);
    Require(intent.passthrough && !intent.up && !intent.down,
            "unacknowledged release cannot submit another shot");

    ++frame.updateId;
    frame.aiming = false;
    frame.fullyAimed = false;
    intent = EvaluateInput(Settings{}, frame, centerRay, queries, controller);
    Require(intent.passthrough && !intent.up && !intent.down,
            "disabled ADS loss does not replay an already committed release");
    ++frame.updateId;
    frame.aiming = true;
    frame.fullyAimed = true;
    intent = EvaluateInput(enabled, frame, centerRay, queries, controller);
    Require(intent.passthrough, "target recovery still waits for shot acknowledgement");
    ++frame.updateId;
    ++frame.shotSerial;
    intent = EvaluateInput(enabled, frame, centerRay, queries, controller);
    Require(intent.down && !intent.up && !controller.HasPendingShot() && controller.HasOwnedInput(),
            "release-shot acknowledgement rearms immediately when native readiness allows it");
    ++frame.updateId;
    Require(EvaluateInput(enabled, frame, centerRay, queries, controller).commitRelease,
            "newly acknowledged release shot still uses a separate freshly checked release update");
}

void ReleaseShotReadiness() {
    Controller controller;
    Queries queries;
    auto frame = ReleaseReady();
    frame.fullyAimed = false;
    Require(EvaluateInput(enabled, frame, centerRay, queries, controller).passthrough &&
            queries.bodyCalls == 0,
            "release-shot weapon requires completed ADS even in Both mode");
    ++frame.updateId;
    frame.fullyAimed = true;
    frame.shotReady = false;
    Require(EvaluateInput(enabled, frame, centerRay, queries, controller).passthrough &&
            queries.bodyCalls == 0,
            "idle release-shot cooldown skips target queries and presses");
    ++frame.updateId;
    frame.shotReady = true;
    Require(EvaluateInput(enabled, frame, centerRay, queries, controller).down,
            "ready fully aimed release-shot weapon acquires input");
    ++frame.updateId;
    frame.shotReady = false;
    const auto bodyCalls = queries.bodyCalls;
    auto intent = EvaluateInput(enabled, frame, centerRay, queries, controller);
    Require(intent.held && !intent.up && !intent.commitRelease &&
            queries.bodyCalls == bodyCalls + 1,
            "held release-shot weapon keeps checking target while readiness recovers");
    ++frame.updateId;
    frame.shotReady = true;
    intent = EvaluateInput(enabled, frame, centerRay, queries, controller);
    Require(intent.up && intent.commitRelease,
            "release-shot readiness recovery permits one committed release");
}

void ReleaseShotLifetimes() {
    Controller controller;
    Queries queries;
    auto frame = ReleaseReady();
    EvaluateInput(enabled, frame, centerRay, queries, controller);
    ++frame.updateId;
    EvaluateInput(enabled, frame, centerRay, queries, controller);
    Require(controller.HasPendingShot(), "committed release owns an acknowledgement lifetime");
    ++frame.updateId;
    frame.manualActive = true;
    auto intent = EvaluateInput(enabled, frame, centerRay, queries, controller);
    Require(intent.passthrough && !intent.up && !controller.HasPendingShot(),
            "manual input takes over while a release shot awaits acknowledgement");

    frame = ReleaseReady();
    controller.Reset();
    EvaluateInput(enabled, frame, centerRay, queries, controller);
    ++frame.updateId;
    EvaluateInput(enabled, frame, centerRay, queries, controller);
    ++frame.identity.weapon;
    frame.updateId = 1;
    intent = EvaluateInput(enabled, frame, centerRay, queries, controller);
    Require(intent.down && !intent.up && !intent.commitRelease && !controller.HasPendingShot(),
            "new weapon lifetime cannot inherit an earlier release acknowledgement wait");
    ++frame.updateId;
    frame.manualActive = true;
    intent = EvaluateInput(enabled, frame, centerRay, queries, controller);
    Require(intent.passthrough && !intent.up && !controller.HasOwnedInput(),
            "manual takeover of a held release weapon never emits a synthetic release");
}

void ReleaseShotCancellation() {
    for (int failure = 0; failure < 10; ++failure) {
        Controller controller;
        NativeCancellation cancellation;
        Queries queries;
        auto frame = ReleaseReady();
        auto settings = enabled;
        auto intent = EvaluateInput(settings, frame, centerRay, queries, controller);
        cancellation.Resolve(frame.identity, false, true, intent, true);
        ++frame.updateId;
        switch (failure) {
            case 0: queries.body.alive = false; break;
            case 1: settings.enabled = false; break;
            case 2: frame.aiming = false; frame.fullyAimed = false; break;
            case 3: frame.fullyAimed = false; break;
            case 4: queries.world = Occlusion::Blocked; break;
            case 5: queries.smoke = Occlusion::Blocked; break;
            case 6: frame.ammoAvailable = false; break;
            case 7: frame.hardBlocked = true; break;
            case 8: frame.fireMode = FireMode::Unsupported; break;
            case 9: settings.mode = Mode::ADSOnly; break;
        }
        intent = EvaluateInput(settings, frame, centerRay, queries, controller);
        Require(intent.up && !intent.commitRelease,
                "lost release-shot eligibility requests cancellation without firing");
        auto decision = cancellation.Resolve(frame.identity, false, true, intent, true);
        Require(decision.input.passthrough && !decision.input.up &&
                !decision.input.commitRelease && decision.stopResult &&
                cancellation.HasPendingCleanup(),
                "uncommitted release is suppressed and cancels the prepared result");

        const auto update = frame.updateId;
        frame = ReleaseReady();
        frame.updateId = update + 1;
        queries = Queries{};
        intent = EvaluateInput(enabled, frame, centerRay, queries, controller);
        decision = cancellation.Resolve(frame.identity, false, true, intent, true);
        Require(!decision.input.passthrough && !decision.input.held &&
                !decision.input.down && !decision.input.up &&
                !decision.input.commitRelease && decision.stopResult &&
                decision.resetController && !cancellation.HasPendingCleanup(),
                "cancelled release shot receives one neutral cleanup frame");
        controller.Reset();
        ++frame.updateId;
        intent = EvaluateInput(enabled, frame, centerRay, queries, controller);
        decision = cancellation.Resolve(frame.identity, false, true, intent, true);
        Require(decision.input.down && !decision.input.up && !decision.stopResult,
                "release-shot sequence can restart after neutral cleanup");
    }

    NativeCancellation cancellation;
    const auto identity = ReleaseReady().identity;
    const Intent cancel{false, false, true, false};
    cancellation.Resolve(identity, false, true, cancel, true);
    auto decision = cancellation.Resolve(identity, true, true, cancel, true);
    Require(decision.input.passthrough && !decision.stopResult &&
            !cancellation.HasPendingCleanup(),
            "manual fire takes over a cancelled release-shot sequence");
    cancellation.Resolve(identity, false, true, cancel, true);
    auto changed = identity;
    ++changed.weapon;
    decision = cancellation.Resolve(changed, false, true, Intent{}, true);
    Require(decision.input.passthrough && !decision.stopResult &&
            !cancellation.HasPendingCleanup(),
            "release-shot cleanup cannot affect a new weapon lifetime");
    cancellation.Resolve(identity, false, true, cancel, true);
    decision = cancellation.Resolve(identity, false, false, Intent{}, false);
    Require(!decision.input.up && !decision.input.down && !decision.input.held &&
            decision.stopResult && decision.resetController,
            "pending release-shot cleanup stays neutral if the input handler changes");
    const Intent committed{false, false, true, false, true};
    decision = cancellation.Resolve(identity, false, true, committed);
    Require(decision.input.passthrough && decision.stopResult,
            "release commitment never bypasses rejection for unsupported handlers");
    decision = cancellation.Resolve(identity, false, true, Intent{}, true);
    Require(!decision.input.passthrough && !decision.input.held &&
            !decision.input.down && !decision.input.up && decision.stopResult &&
            decision.resetController && !cancellation.HasPendingCleanup(),
            "newly verified release handler clears prior unsupported cancellation neutrally");
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
    VirtualWeaponReadiness();
    SingleShotAcknowledgement();
    AcknowledgedShotWork();
    BothModeCadence();
    AcknowledgedReleaseGuards();
    WeaponStateWorkBudget();
    CameraWorkBudget();
    ReleaseShotAcknowledgement();
    ReleaseShotReadiness();
    MissingAcknowledgementRecovery();
    AmmoAcknowledgementAndQueryBudget();
    SniperReactionCadence();
    ReleaseShotLifetimes();
    ReleaseShotCancellation();
    OwnershipAndLifecycle();
    AdsAndModeChanges();
    TargetValidation();
    InputQueryBudget();
    CancellationAndSettings();
    std::cout << "Passed " << checks << " trigger behavior checks\n";
}
