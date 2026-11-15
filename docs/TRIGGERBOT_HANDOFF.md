# Triggerbot handoff

Prepared on 2026-11-15. Read [compact memory](../HANDOFF_MEMORY.md) first when resuming.

## Current request and implementation status

The user asked what could improve the triggerbot, then requested a Markdown handoff and compact memory. The latest request is documentation. The triggerbot was reviewed; **none of the proposed triggerbot upgrades below has been implemented or tested in this handoff task**.

The recommended implementation order is shot-confirmation recovery and useful diagnostics, followed by weapon-specific validation, optional firing-path improvements and measured optimization. Treat the optional additions as proposals, not completed or universally authorized features.

## Repository and delivery state

- Repository: <https://github.com/oonalang/NPC>.
- Current checkout: `/workspace/NPC-liquid-ui`.
- Native Android C++17, Dear ImGui, OpenGL ES 3.0; NDK r22, ARM64/API 21.
- Working branch: `feature/esp-3d-runtime-physics`.
- Last implementation commit: `3dfd40446e4749c590131e0c75fe37d500f59b85`, already pushed.
- Preceding implementation: `07c9ad6`, runtime Wiz rig physics.
- Main was last verified at `51e03ed1946ac4be5176e35843053f73e4e8e03a`. Recheck before relying on remote state.
- This handoff is a subsequent documentation change; use `git log -1` for the current branch tip.
- Review: <https://github.com/oonalang/NPC/compare/main...feature/esp-3d-runtime-physics>.

Inspect `git status` before editing. Preserve the completed ESP work. The delivery instruction is a feature branch for review; do not merge, reset or push main without a new explicit instruction.

## Existing triggerbot behavior to preserve

Read [triggerbot.md](triggerbot.md), then the actual code; older prose about a displayed status does not describe the current Trigger tab completely.

- The trigger checks a camera-center ray against a detailed living-enemy body hit, world obstruction and smoke. It does not aim the camera.
- Physical/manual firing takes precedence over synthetic input.
- `Both` permits hipfire/ADS for ordinary weapons; `ADS Only` requires completed ADS. Release-to-fire input requires completed ADS in either setting.
- A release-to-fire sequence presses first, revalidates on a later update, then commits a release when eligible. Cancellation must not generate a firing release edge; neutral cleanup/rearm behavior matters.
- Input ownership follows match, pawn, weapon, component and input-controller lifetimes. Duplicate updates must not create extra edges.
- Readiness comes from the native `CanFire` virtual function and its metadata, with executable-address checks.
- The native adapter accepts two verified preparation handlers. Although the core enum includes burst modes, `PreparedFireMode()` currently rejects bursts. Charged/specialized preparation handlers remain unsupported.
- Hitscan and projectile dispatch wrappers notify a shot after the original dispatch returns. These notifications are not proof that a bullet hit or that the server accepted damage.
- The native profile is pinned to the supplied 1.6.59 build, GNU build ID `056d6390cc712865`. An unknown build must not silently bypass verification.

Historically, the user reported rifles and shotguns firing while every tested sniper failed. Those are earlier device reports, **not a reproduced diagnosis of the current commit**. No current device accuracy, latency or FPS benchmark is available.

## Confirmed code observations and proposed next work

### P0: Recover safely from missing shot confirmations

In `Controller::Step()`, `AwaitShot` returns passthrough until `shotSerial` changes. It has no elapsed-time limit. Normal single-shot `Held` also waits on the submitted serial while eligible. `NativeDriver::Update()` can reject a release immediately when the native result reports failure, but it does not implement a general confirmation deadline. A missed notification can therefore leave the controller waiting; this is a code-level risk, not a proven explanation for the user's sniper failures.

Proposed work:

- Add an injectable monotonic time source and named confirmation/recovery limits.
- Associate pending attempts and diagnostics with the owning context/lifetime.
- Distinguish a native rejection, a confirmed discharge, and an uncertain/missing notification using available ammo, readiness and native-result evidence.
- Preserve neutral cleanup and manual takeover. Elapsed time alone must not authorize a duplicate shot or a synthetic release that could fire a canceled sniper.
- Handle late/duplicate notifications, disabled settings, context changes and weapon switches during recovery.
- First confirm that the native dispatch hooks and observed fire-component identity deliver the notifications expected by the controller.

### P1: Explain why firing is blocked

`RuntimeStatus` currently contains broad setup/context states. `NativeDriver` publishes `Ready` before target evaluation; this does not mean a shot is eligible. It computes a `TargetResult` but does not publish that detailed result to the Trigger UI. `TriggerTab.h` exposes controls and setup retry, without a useful firing-reason display.

Proposed work:

- Publish a consistent, bounded diagnostic snapshot from the decision that actually runs.
- Show disabled/manual takeover, setup/context failure, unsupported weapon, waiting for ADS, no ammo, weapon blocked/not ready, no target, world/smoke obstruction, pending confirmation and recovery.
- Include detected fire mode and pending-shot age where helpful. Do not mislabel every blocked weapon state as a reload unless the native state is verified.
- Keep diagnostics observational: they must not alter eligibility or add expensive render-thread game queries.

### P2: Validate separate weapon firing sequences

Verify bolt-action, semiautomatic, press-to-fire and release-to-fire behavior against the matching native implementation. Preserve working rifle/shotgun behavior. Add burst/charged support only after their preparation and cancellation semantics are understood and tested; the existence of enum values is not native support.

### P3: Optional accuracy controls and firing-path checks

Consider a verified muzzle-to-target obstruction check alongside the existing camera ray. Currently `CheckWorld()` traces from the camera ray origin, which can differ from the barrel's path near cover. Use actual native shot/range data where available; do not guess offsets or claim projectile/spread accuracy that has not been measured.

Head/body filters and per-weapon distance limits are optional follow-ups. The current detailed native body-group ID is reduced to a validity flag. Verify group meanings before exposing named hit regions. Never reuse stale visibility or enlarge the acceptance area merely to increase the firing rate.

### P4: Profile and expand timing tests

Measure body/world/smoke query cost, query counts and press-to-dispatch/confirmation latency. Cache only stable verified metadata with correct lifetime invalidation. Existing early-outs already skip many idle/blocked target queries; measure before changing them.

Expand timelines at 30/60/90/120-Hz input schedules for delayed/missing/late/duplicate confirmations, cycling/reload, rapid target entry/exit, ADS transitions, cancellation and weapon/context changes. Preserve fresh validation at release commit. Separate host policy results from live Android integration results.

## Code map

| File | Responsibility |
| --- | --- |
| [TriggerCore.h](../src/main/jni/Trigger/TriggerCore.h) | Settings, frame/context identity, input intent, owned input and confirmation/rearm state machine. |
| [TriggerRuntime.cpp](../src/main/jni/Trigger/TriggerRuntime.cpp), [TriggerRuntime.h](../src/main/jni/Trigger/TriggerRuntime.h) | Atomic settings, target-result types, strict ray/hit validation, query gating and policy evaluation. |
| [NativeDriver.cpp](../src/main/jni/Trigger/NativeDriver.cpp) | Native hooks, physical input sampling, prepared-frame evaluation, component/shot serials, native rejection handling and lifecycle cleanup. |
| [NativeInputStage.h](../src/main/jni/Trigger/NativeInputStage.h), [NativeCancellation.h](../src/main/jni/Trigger/NativeCancellation.h) | Supported preparation, fire-mode classification, merged button edges and release cancellation. |
| [NativeReadiness.h](../src/main/jni/Trigger/NativeReadiness.h) | Verified native `CanFire` dispatch and metadata. |
| [NativeQueries.cpp](../src/main/jni/Trigger/NativeQueries.cpp) | Camera ray, detailed enemy-body trace, world and smoke checks. |
| [NativeProfile.cpp](../src/main/jni/Trigger/NativeProfile.cpp), [TriggerCompatibility.cpp](../src/main/jni/Trigger/TriggerCompatibility.cpp) | Pinned build/entry verification, bounded setup/retry and runtime compatibility guards. |
| [TriggerTab.h](../src/main/jni/System/UI/TriggerTab.h) | Trigger controls and independent dot appearance; no detailed firing diagnostics yet. |
| [DrawESP.h](../src/main/jni/Hacks/Feature/DrawESP.h) | `WeaponFireComponent_Instant_CreateBulletLine` / `CreateBulletProjectile` wrappers call `NotifyNativeShot()` after the original dispatch. Keep unrelated aiming/ESP hooks untouched. |
| [trigger_tests.cpp](../tests/trigger_tests.cpp), [sniper_input_test.cpp](../tests/sniper_input_test.cpp), [native_profile_tests.cpp](../tests/native_profile_tests.cpp) | Existing controller, prepared-input and profile-policy tests. |

The generated native ABI was verified beyond the C# declarations: static query methods retain a dummy object argument, and the ARM64 camera Ray return uses indirect aggregate return. Preserve those conventions and trailing metadata arguments.

## Validation for the implementation session

Run from the repository root:

```sh
sh tests/run-trigger-tests.sh
python3 tests/verify-trigger-profile.py /workspace/dumps/libunity.so
```

The dump is currently `/workspace/dumps/Dump-1.6.59.cs`. These workspace paths may not survive a new environment. Verify the supplied binary's identity rather than assuming it is the latest game version.

The trigger suite covers controller/target policy, dot appearance, prepared sniper input and profile initialization policy under ASan/UBSan. Its native-profile host fixture stubs the driver; it does not execute the actual Android hooks. The ELF verifier checks the supplied disk image, not loaded/decrypted runtime bodies or live weapon behavior.

When source implementation changes, build the native library if the toolchain is available:

```sh
/workspace/toolchains/android-ndk-r22/ndk-build NDK_PROJECT_PATH=. \
  APP_BUILD_SCRIPT=src/main/jni/Android.mk \
  NDK_APPLICATION_MK=src/main/jni/Application.mk \
  NDK_OUT=/workspace/liquid-build/obj NDK_LIBS_OUT=/workspace/liquid-build/libs -j2
```

No trigger tests or builds were rerun for this documentation-only handoff. Establish a baseline when implementing. On-device acceptance still needs representative automatic rifles, shotguns, bolt-action/semiautomatic snipers, manual override, cover/smoke, interrupted ADS, reload/cycling, death/respawn and weapon swaps. Record actual diagnostic reasons and observed shots; do not announce a sniper fix from host tests alone.

## Completed work to retain

The last implementation task upgraded the real native 3D Wiz preview, documented in [ESP_3D_UPGRADE.md](ESP_3D_UPGRADE.md): `WIZSKN03`, 120 bones, 42 spring joints, 48 cloth links, 16 body capsules, corrected folded-hem weights, physical chest/hair/garment response and varied gestures. Physics is 60 Hz; cached rendering is 30 Hz. All three preview suites and the ARM64 library build passed for `3dfd404`, including 48-second CPU/GLES checks. Phone performance/input remain unverified; full mesh self-collision is unsupported.

Local review artifacts, not committed repository files:

- `/workspace/wiz-motion-rework/comparison.mp4`: preceding preview versus revised first 12 seconds.
- `/workspace/wiz-motion-rework/preview.mp4`: full 48-second native GLES capture.
- `/workspace/wiz-motion-rework/`: logs and contact sheets.

The model remains Glayer3d's CC BY-NC 4.0 derivative. Preserve [WIZ_LICENSE.md](../src/main/jni/Assets/WIZ_LICENSE.md), original painted textures and the 3D architecture. Do not replace it with a flat animation, website or unrelated application.

## User preferences

Accuracy and reliable behavior come before firing frequency. Preserve working features and unrelated UI/auth/music/skins/movement behavior. The user dislikes unnecessary confirmation loops and added source comments; avoid new explanatory source comments, while retaining existing attribution/licenses. Be candid about tests actually run and device-only limits. For implementation delivery, commit a reviewable feature branch and update these notes; do not push main based on older superseded requests.
