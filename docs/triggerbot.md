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

All 50 core fingerprints, seven optional glass fingerprints and seven diagnostic
probes match the supplied ELF when mapped through its program headers. The checked
trigger RVAs map to file offsets at `RVA - 0x4000` in this image.

The C# declaration of a static method does not fully specify this binary's ABI.
Native instructions confirm that generated static query methods retain a dummy
object argument. Removing it shifts integer and object arguments. Camera rays
also use the ARM64 indirect aggregate return convention. Preserve these bindings.

Eight method bodies in the disk image are encoded. Their disk fingerprints are
checked by the offline verifier rather than compared with the loaded bodies.
Runtime validation requires the pinned build ID, readable executable ranges for
all 50 core code locations, and exact prefixes for the 42 stable locations, including all five
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
binding validation is separate from invoking the predicate. Pending release
shots and prepared follow-ups sample valid native readiness to detect cooldown
recovery. Cleanup, blocked input and incomplete required ADS skip that call.

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
weapon's ordinary single-shot flag. Release input requires completed ADS in
either trigger setting. An eligible frame arms the press. A later freshly
validated frame commits its release while the weapon can fire.

An acknowledged shot can prepare the next non-firing press during native
cooldown if full ADS, accepted input, ammo and fresh geometry permit it.
The prepared press holds through cooldown with target queries deferred until
readiness recovers. Its firing release then uses fresh body, world and smoke
checks on the first native-ready update. The native fire-check gate must accept
the preparation update.

Shot acknowledgement comes from local dispatch callbacks or a verified clip
decrease in the owning fire component. Pending release shots also observe
sampled native readiness. An eligible blocked-to-ready cycle permits a freshly
checked retry when both acknowledgement sources remain unchanged. A named
500 ms deadline supplies neutral recovery if neither acknowledgement nor
readiness recovery is observed. An unsampled false readiness value supplies
no recovery evidence. Native rejection handling also checks current clip
consumption before rejecting the pending release.

At a firing release, loss of the target or smoke/world obstruction cancels the
prepared input. Incomplete ADS, disabled trigger, manual takeover, invalid
context and blocked weapon state also cancel preparation. Cancellation uses
neutral cleanup before another press. Physical user input always takes priority.

Press-to-fire weapons retain their native input semantics. A confirmed single
press now releases without another target trace; its next press still needs
fresh geometry and native readiness. Live firing still requires device validation.

The current follow-up fix, branch history, regression results and device
validation gaps are recorded in [the Triggerbot handoff](TRIGGERBOT_HANDOFF.md).

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
- The Trigger tab reports useful decision status: full-aim wait, weapon
  recovery, cover/smoke rejection, owned press, confirmation wait and input
  recovery. These are input states, not claims of server-registered hits.

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
  retains its `MethodInfo`. The predicate runs when the input decision or
  pending release recovery needs readiness. Cleanup updates defer that call.
- Confirmed press-fire cleanup skips body/world/smoke queries. A confirmed
  release-fire shot can prepare a follow-up during an eligible cooldown update
  after fresh target and obstruction checks. Native readiness gates its firing
  release. Manual input, context changes and rejected releases retain their
  cancellation and recovery paths.
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

## Battle Royale frame time

The first native physics ray used the camera's entire visible distance even on
a miss. An enemy candidate adds detail, world and smoke checks on the same input
update. The range now follows the active weapon on the verified BR pawn path.
For a valid native range, the ray stops at the greater of 500 world units and
weapon range plus 32 units of camera allowance, within the camera's visible
distance. An unavailable weapon range uses the camera distance. The BR pawn
type is checked once per match and pawn lifetime, while range is sampled for
each long BR query so a weapon or attachment change can take effect immediately.

An automatic weapon with no held input waits for native readiness before
tracing. It checks and presses on its first ready update. Held input still
checks current geometry through cooldown so losing a target cancels it
immediately. Host tests cover the range bounds, weapon changes and the skipped
cooldown queries. A BR device capture is needed to measure the frame time gain.

## Glass and interrupted sniper input

Ordinary, breakable and physically penetrable glass use the native material
classifier. When a pane is the first hit, a bounded raycast with the same mask
finds the next solid contact. The usual body trace still has to agree with that
contact. The world check starts at the original camera and uses its own native
mask, so a wall behind or touching a pane still blocks the shot. Unknown and
impenetrable materials remain solid. Each fallback considers at most two panes
and 32 contacts. A full buffer or unavailable native binding leaves the blocker
in place. The native weapon still determines whether a shot breaks or penetrates
the pane.

The trigger checks the prepared `CheckFireParam.CheckFire` flag before tracing.
An update the game will ignore cannot consume a synthetic press or release.
Changing mode while idle no longer discards a ready update. Release-fire
snipers still need completed ADS and distinct press and freshly checked release
updates. Host tests cover these transitions and the glass and cover ordering.

## False world obstruction follow-up

The user reports the "Target is behind cover" status with a visible, unobstructed
enemy. The recording alone does not identify the colliding object. The previous
world fallback had a concrete false-rejection path: a world contact with the
target's broad collider could lie before the detailed body point, and any path
without a glass pane was treated as blocked. Failed contact verification was
also reported as confirmed cover.

`NativeTargetQueries` now retains the validated target and broad collider only
for its current query. When the native distance check reports a nearer hit, a
bounded contact query uses the same world mask, original ray, water filter and
range. The validated broad body collider can be excluded as a character contact.
Other contacts mapped to the local pawn or current target require a successful
native detailed body verification. Shield groups 6 and 32 remain solid, including
when mapped to the current target. Other actors and solid world objects still
block. All contacts are checked in distance order, so excluding a body proxy
does not hide a wall before the detailed body point. Glass keeps its existing
material and pane limits.

Unavailable optional bindings, full/incomplete contact buffers, invalid data or
disagreement with the original nearest hit produce an unavailable query. They
cannot authorize a shot or claim that cover was confirmed. Clear native world
checks and endpoint-only hits do not invoke this fallback. The contact list uses
fixed storage with at most 32 entries; character aliases may require additional
native detail calls. No reaction timer, firing cadence, aim requirement, native
compatibility gate or license capability was changed.

The sanitizer suite passes 4,428 behavior checks, including broad-body false
cover, character aliases, intervening walls, separately mapped and target-owned
shields, unavailable queries, malformed contacts, clear-path query budgets, and
acquisition/cancellation for Both and ADS Only with automatic and release-fire
weapons. Prepared-input, native-profile and permanent-rejection suites pass;
all 57 fingerprints and seven compatibility probes still match the supplied
ELF. The ARM64 Lite build and hardening inspection pass. These tests establish
the policy and static binding checks; they do not establish which collider was
hit in the user's recording or prove the behavior on a phone. Device collision
and FPS verification remain outstanding.

## Bullet-passable world material and query budget

The user's `obstruction-3` capture identifies a fresh rejection: detailed body
at 27.6363 m, world/contact at 0.6096 m, material 15, world/unmapped owner,
one contact, age 5 ms and sequence 2203. Material 15 is `PenetrateStone`. The
previous classifier treated every non-glass world material as solid.

The pinned game's `PhysicsUtil.CanIgnoreForBullet` at `0x66F8D74` classifies the
material through `GetMaterialType`, checks the unsigned type is below 16 and
uses mask `0xA800`: types 11, 13 and 15 are ignored by its bullet filter.
`IsBulletIgnoredWorldMaterial` mirrors that rule for unmapped world geometry.
Two additional runtime fingerprints verify the type/mask instructions at
`0x66F8DC4` and the decision at `0x66F8DD4`. They are mandatory for the contact
fallback to run. No new native call or weapon-property mutation is introduced.
Mapped actors do not receive this material exemption. Shield/body checks,
other solid materials, smoke, ADS, native readiness and licensing remain.
This implements the native bullet-ignore category; it does not add arbitrary
weapon penetration through other walls.

`WorldQuery.h` removes a repeated world raycast when the path keeps encountering
contacts. The first nearer world hit still uses the fast check plus its bounded
contact fallback. Subsequent eligible updates query fresh contacts directly,
using the same original ray, range, world mask and water filter. Every contact
is checked before firing. A complete empty query returns to the fast clear-path
route. Incomplete/unavailable contact queries can use one fresh fast check;
they cannot trigger another contact retry in the same update. Only a route
preference is retained, reset on context/weapon changes. Collision results and
positive firing decisions are never cached across updates.

The stable-contact fixture uses 121 world-physics queries over 120 updates,
versus the previous 240. That is a query-count result, not a phone FPS measure.
Body-detail, smoke and actual game simulation costs are not measured by it.

## Device obstruction diagnostics

The current panel tag is `Trace build: obstruction-4`. World errors show the
last reason, body/world/contact distances, owner, material, group, contact count,
sequence and sample age. It also reports world-query duration in microseconds
and fast/contact query counts as `rays fast+contacts`. A compact last-world-query
cost remains visible for fresh samples during ordinary enabled operation.
Samples older than 1,500 ms are explicitly stale. Both clocks are the same
steady clock, separate from the licensing boot clock.

`WorldDiagnostics.h` publishes one coherent 56-byte snapshot behind a mutex.
The panel wraps and scrolls using the existing Lite controls. No raw pointers,
credentials, persistent files, network uploads or per-player scan is added.
If another rejection occurs, capture the revision and all lines while aimed at
the same target. The timing is the world-query stage, not total frame time.

Validation passes: 4,428 trigger behavior, 46 prepared-input, 1,046 native-profile
and four permanent-rejection checks; 4,096 fresh-query equivalence scenarios;
the reported material/distance case and an intervening real wall; unknown,
empty and malformed contact transitions; context reset and query-count bounds.
The supplied ELF matches 59 fingerprints and seven compatibility probes.
Snapshot sanitizer tests cover 20,000 concurrent publications and bounded
formatting. Lite UI sanitizer/GLES, final narrow/fresh/stale captures, ARM64
build/hardening and production-root release verification pass. Logs use
`lite-trigger-world4-*`. The corrected classification is verified against the
capture's inputs; installed-phone behavior and FPS still need a retest.

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
