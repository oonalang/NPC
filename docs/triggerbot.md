# Triggerbot integration

The NPC source already contains the trigger input controller, menu controls,
native adapter and target queries. Purple-Phantom's referenced `main` branch
contains an unregistered autofire hook rather than a working input integration,
so this change repairs NPC's existing path.

## Verified target

- Dataset: <https://huggingface.co/datasets/jeudhehs/dump/tree/main>
- Dump: `Dump 1.6.59.cs`, ARM64 `libunity.so`
- GNU build ID: `056d6390cc712865`
- NPC base: `bd3fb7187311d2e2abfa7306aca425954f176771`
- Purple reference: `fd895e5f1023712a331fcdf530371e1332e557cc`

All 46 pinned code fingerprints and seven diagnostic probes match the supplied
ELF when virtual addresses are mapped through its program headers. The checked
trigger RVAs map to file offsets at `RVA - 0x4000` in this image.

The C# declaration of a static method does not fully specify this binary's ABI.
Native instructions confirm that generated static query methods retain a dummy
object argument. Removing it shifts integer and object arguments. Camera rays
also use the ARM64 indirect aggregate return convention. Preserve these bindings.

Eight method bodies in the disk image are encoded. Their disk fingerprints are
checked by the offline verifier rather than compared with the loaded bodies.
Runtime validation requires the pinned build ID, readable executable ranges for
all 46 code locations, and exact prefixes for the 38 stable locations, including all five
input hook entry points. The encoded methods must be readable and no longer
match their known encoded disk prefixes before hooks can be installed. Unknown
builds and changed stable entry points leave automatic input disabled. Live
device compatibility remains unverified.

Reading validated module segments no longer requires `/proc/self/mem` access.
Transient loader-readiness failures retry in a startup worker for up to ten
seconds; **Retry trigger setup** in the Trigger tab starts another bounded
attempt if the loader is still pending. Concurrent retry workers are suppressed.
Build-ID and stable-prefix mismatches are terminal. The initialization result is
also written to the native log.

## Behavior and changes

The **Triggerbot** checkbox is in the Trigger tab. **Both** permits hipfire
and ADS; **ADS Only** requires completed ADS. **Dot Crosshair** is independent of
automatic firing. The trigger uses the camera-center body intersection and
requires a living enemy, a valid detailed hit, clear world geometry and no smoke
obstruction. It does not adjust aim.

Hitscan and projectile dispatch now each acknowledge a shot once, after the
original dispatch returns. This lets the single-shot controller release and
rearm when a projectile shot is confirmed. Manual press, hold and release frames
take precedence over synthetic input. Invalid gameplay context updates the
displayed status instead of leaving a stale firing message.

The adapter supports ordinary automatic and single-shot weapons with the base
or verified COD sniper input-preparation handler. Charged and other specialized
preparation handlers remain excluded. Shot readiness uses the
actual `CanFire` virtual function and its metadata, matching the native game's
dispatch, and requires that function to lie in the verified module's executable
segments. This preserves subtype restrictions without rejecting every override.
Burst modes remain unsupported. The ammo query is reused within each input
decision to avoid a second native call and inconsistent samples. Readiness
binding validation is separate from invoking the predicate: confirmation waits,
cleanup, blocked input and incomplete required ADS do not call `CanFire`.

## Sniper input

Physical button state is sampled before weapon preparation, while synthetic
input is merged at `CheckFireBtnAimAndFire` (`0xC916D64`) after the native virtual
preparation runs. This uses the current `CheckFireParam.IsReleaseToFire` and the
native controller's release decision, rather than stale flags from an earlier
update. The native result's previously copied button edges are refreshed too.

`WeaponCODSniper` uses preparation at `0xC38D790`; traditional `WeaponSniper`
inherits the base handler. The client permits these two known handlers without
calling or replacing their preparation routines.

A prepared release input follows the native release/up flags regardless of the
weapon's ordinary single-shot flag. Release input requires completed ADS in either trigger
setting. An eligible frame arms the press; a later freshly validated frame
commits its release while the weapon can fire. The controller then waits for
shot acknowledgement before another press. Acknowledgement can rearm on that
same update if native readiness, full ADS and fresh geometry all permit it;
it no longer reserves an extra idle update for acknowledgement alone. A verified decrease in
the owning fire component's native clip count can acknowledge a shot whose
callback was missed. If neither acknowledgement source changes, a named
500 ms deadline clears the owned input neutrally and permits a new attempt
only after a fresh target query and the original readiness checks. This
deadline starts after submission; it adds no delay before initial acquisition. Loss of the target,
smoke/world obstruction, incomplete ADS, disabled trigger or a blocked weapon
cancels an armed press without generating a shooting release edge. One neutral
cleanup frame precedes any new press. Physical user input always takes priority.

Press-to-fire weapons retain their native input semantics. A confirmed single
press now releases without another target trace; its next press still needs
fresh geometry and native readiness. Live firing still requires device validation.

## Reaction and input-work revision

The user reports intermittent frame drops, delayed shots and occasional missed
shots with snipers in Both mode. No recording or device profiler capture has
been received yet, so these edits address reproducible source-level faults
rather than claiming a device diagnosis.

- A committed release previously waited forever if its callback never arrived.
  It also kept tracing targets while waiting, although it could not submit input.
  The native clip count at component offset `0xA8` now supplies a second shot
  acknowledgement. The dump declaration and the getter
  `WeaponFireComponent.get_ClipAmmoCount` were checked against the pinned ELF;
  the native load at `0xC2B4B98` is the added 16-byte compatibility anchor.
- `Controller::shotAcknowledgementTimeoutMillis` is 500 ms. Missing evidence
  leads to neutral cleanup for release-fire, or an ordinary owned release for
  press-fire. It does not replay a shooting up-edge. Manual ownership,
  weapon/match changes, ammo, native cooldown, completed ADS, world geometry
  and smoke still control whether a fresh attempt may fire.
- Unacknowledged confirmation waits, rejection-cleanup and duplicate updates skip body,
  world and smoke traces. Ordinary target acquisition and every committed
  sniper release still use a fresh exact ray, detailed body hit and obstruction
  checks. Target results are not cached across updates.
- The five periodic input fingerprints now read current bytes through already
  verified executable ranges. They retain the 64-update cadence and fail-closed
  behavior, without repeated loader-list walks or proc-file fallbacks on the
  input thread. Only segment bounds are cached; instruction contents are read
  anew.
- Physical fire takes a fast passthrough after compatibility/context checks.
  Hidden dot overlays skip screen-coordinate work; visible overlays validate
  screen dimensions and projection. Fresh camera rays are preserved. No managed impact object is retained between
  queries.

Host scenarios at 30, 60 and 120 updates/sec verify an owned press on the first
eligible update, a release on the next freshly checked update, and time-based
recovery with missing callbacks. The release path deliberately keeps distinct
press/up updates and native weapon timing. This is not a forced fire-rate
change. Current test/build logs are under
`/workspace/license-hardening/logs/trigger-latency-*.log`. Actual reaction time
and frame-time improvement still require phone/video verification.

## Both mode and other weapons

The follow-up applies the shared work budget to all supported native weapon
input modes, in Both and ADS Only. Initial acquisition submits a press on the
first eligible input update, without an added reaction timer. Release-fire
weapons retain separate press/release updates and native aim/cooldown rules.
Zero elapsed time is not possible: input update cadence, native shot processing
and server timing remain outside this controller's scheduling.

- Both mode skips ADS getters for ordinary press-fire and automatic weapons.
  ADS Only and release-fire weapons still sample required aim state. The partial
  ADS percentage getter runs only if the native fully-aimed flag is false.
- Binding the native `CanFire` method still verifies its executable address and
  retains its `MethodInfo`. The predicate is invoked when an input decision can
  use it, rather than during confirmation-only or cleanup updates.
- Confirmed press-fire cleanup skips body/world/smoke queries. A confirmed
  release-fire shot can rearm immediately when ready, using a fresh target and
  obstruction check. Native cooldown, manual input, context changes and rejected
  releases still prevent new input; timeout/cancellation cleanup stays neutral.
- A hidden dot performs no screen-width, screen-height or viewport projection
  calls. Camera origin, direction and clipping planes are sampled afresh;
  visible dot validation remains unchanged. The camera sampler has no heap
  allocation or virtual dispatch.

Host tests cover acquisition at 30/60/90/120/144 updates per second, callback
and clip-based acknowledgement, readiness/aim work suppression, fresh geometry
on acknowledgement, prepared native button edges and camera call counts.
Core burst enums are exercised by the state-machine tests; this does not enable
the excluded native burst or specialized handlers. No FPS gain or device
reaction time has been measured. Logs are
`/workspace/license-hardening/logs/trigger-all-weapons-*.log`.

## Validation

Host checks need a C++17 compiler with AddressSanitizer and UBSan:

```sh
sh tests/run-trigger-tests.sh
python3 tests/verify-trigger-profile.py /path/to/libunity.so
```

The host suite covers single-shot acknowledgement, ownership and release,
context changes, ADS, target geometry, world and smoke obstruction, settings,
cancellation and skipped queries while idle. The profile verifier reads the
supplied ELF locally; it does not download or modify the library.

The native library builds with Android NDK `22.0.7026061` (r22):

```sh
"$ANDROID_NDK_ROOT/ndk-build" \
  NDK_PROJECT_PATH="$PWD" \
  APP_BUILD_SCRIPT="$PWD/src/main/jni/Android.mk" \
  NDK_APPLICATION_MK="$PWD/src/main/jni/Application.mk" \
  NDK_OUT=/tmp/npc-native/obj \
  NDK_LIBS_OUT=/tmp/npc-native/libs -j4
```

Two include filename corrections allow this build on case-sensitive filesystems.
APK packaging requires the Android Gradle/SDK setup used by the project; this
repository does not include a Gradle wrapper or root plugin configuration.

Device acceptance should check manual fire release while aiming at a target,
automatic target entry/exit, single-shot rearming, ADS transitions, reload,
death/respawn and weapon changes. Record the runtime status if verification
rejects the loaded library. No live accuracy or performance benchmark has been
performed in this environment.
