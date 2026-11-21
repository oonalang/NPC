# Triggerbot follow-up and BR performance handoff

Updated on 2026-11-21 for the NPC native Android C++17 project.

## Current request and result

The user reports a slower second sniper shot while staying scoped. Earlier
reports include intermittent Battle Royale FPS drops around nearby players,
missed shots through glass, and occasional misses with snipers in Both and
ADS Only. The user requested a full Markdown handoff, then asked for the fix
to be implemented.

The current change prepares an acknowledged sniper's next press during an
eligible cooldown update. The firing release can then occur on the first
native-ready update after fresh target validation. Pending release shots also
observe native readiness recovery so a missing callback and unchanged clip
count can recover before the existing 500 ms deadline. Native rejection
handling now respects ammo consumed by the pending shot.

The regression test failed before the implementation and passes with it.
Both variants pass host sanitizer tests and ARM64 native builds. Live Android
shot timing and BR frame time still require device measurements.

## Repository and branch state

The repository is <https://github.com/oonalang/NPC>.

| Variant | Local review branch | Implementation commit | Worktree |
| --- | --- | --- | --- |
| Main-based | `fix/br-trigger-performance` | `78f4930` | `/workspace/NPC-license-hardening` |
| Free Trial | `fix/br-trigger-performance-trial` | `edbce37` | `/dev/shm/npc-br-trigger-trial` |

Remote refs checked on 2026-11-21 are `main` at `c7c0162` and
`free-trial-build` at `03c13b9`. The main ref records the requested rollback
to the tree at `a4e4b27`. These review branches retain their separate bases.
The current fix and this handoff are local commits awaiting publication.
Use `git log -1` to obtain each branch's latest documentation commit.

The main-based branch already contains the following local work.

| Commit | Existing change |
| --- | --- |
| `1695f97` | Bounds BR traces using the camera distance and active weapon range |
| `947cc02` | Adds verified glass fallback and respects the prepared native fire-check gate |
| `050d1a1` | Rejects unloaded colliders during glass fallback |
| `78f4930` | Prepares scoped follow-ups, observes readiness recovery and preserves clip-based acknowledgement during rejection handling |

The corresponding Free Trial commits are `26cdb87`, `82008af`, `d33d206`
and `edbce37`. Free Trial keeps its existing trial lifecycle and diagnostic
surface. Merge resolution retained the shared controller changes and tests.

## Evidence and remaining uncertainty

| Observation | Evidence | Status |
| --- | --- | --- |
| Second sniper shot feels slow while hardscoping | User report | Device reproduction pending |
| Old follow-up press starts after native cooldown | Controller source and failing regression | Reproduced in host simulation |
| Missing confirmation can wait for the 500 ms deadline | `AwaitShot` source and existing timeout tests | Verified controller behavior |
| Pending release previously skipped readiness sampling | Runtime query gates | Verified source behavior |
| Rejection previously ignored clip consumption | Post-update native rejection branch | Verified source inconsistency |
| New prepared follow-up releases at first eligible readiness recovery | Simulated cooldown timelines at 30, 60 and 120 updates per second | Host tests pass |
| BR drops are primarily caused by one particular native function | No device profiler capture | Unresolved |
| Current glass fallback handles every visible glass object | Pinned binary checks and host material/contact tests | Device coverage pending |

An input callback confirms local shot dispatch. Server damage acceptance,
actual weapon rechamber time and the player's visible shot timing require
in-game measurement. No gameplay recording or per-update native trace for
this latest delay has been examined.

## Findings behind the follow-up change

### Preparation started after cooldown

The old release-fire path required `shotReady` before creating a new press.
It then required a separate update for the firing release. When a preceding
shot had already been confirmed during cooldown, the next native-ready
update armed a press and the following update released it.

The new path can prepare a follow-up during cooldown after a real dispatch
or clip acknowledgement. Preparation requires completed ADS, ammo, valid
ownership, accepted native fire checking and current eligible geometry.
The controller holds that press through cooldown and performs fresh body,
world and smoke checks when the weapon becomes ready to release.

This removes one input update from the tested prepared-follow-up path.
At a matching 30, 60 or 120 Hz input rate, one update is approximately
33.3, 16.7 or 8.3 ms. These numbers describe update intervals in the host
model. Actual Android input scheduling needs measurement.

### Missing acknowledgements hid readiness recovery

`AwaitShot` previously waited for a changed shot serial or decreased clip
count. Its target gate also prevented aim/readiness sampling during the
wait. A shot whose two acknowledgement sources remained unchanged could
stay pending until the named 500 ms recovery deadline.

The current controller observes readiness for a pending release while its
input state remains eligible. A sampled native blocked state followed by
a sampled ready state permits a new attempt after fresh geometry checks.
This recovery still uses separate press and release updates.

`Frame::readinessSampled` distinguishes an actual native result from the
default false value used when sampling was skipped. Disabled settings,
incomplete ADS, invalid context, missing ammo, blocked input and input-mode
changes cannot supply a false cooldown observation. Manual input resets
synthetic ownership.

The 500 ms deadline remains a fallback when acknowledgement and readiness
recovery are both unavailable. A continuously ready weapon with no shot
evidence can still reach that deadline.

### Rejection could discard clip evidence

The native post-update path can reject a submitted release when its
`StartFire` result is false and no dispatch callback arrived. Previously,
that path ignored ammo already consumed by the shot.

The updated path reacquires the fire component through the existing verified
weapon getter. It samples clip ammo only when that component still matches
the owner. `Controller::RejectPendingShot()` checks context, serial and clip
evidence before recording rejection. A consumed round can therefore remain
available as acknowledgement on the following update.

## Current release-fire sequence

| Stage | Input | Validation and work |
| --- | --- | --- |
| Initial acquisition | One press/down | Native readiness, full ADS and fresh body/world/smoke checks |
| Initial firing release | One committed up on a later update | Fresh eligibility and geometry, native readiness |
| Pending confirmation | Pass through | Eligible aim/readiness observations, deferred target physics |
| Acknowledged cooldown preparation | One non-firing press/down | Accepted input, full ADS and one fresh target evaluation |
| Prepared cooldown hold | Held with no repeated edges | Eligible aim/readiness observations, deferred target physics |
| Follow-up firing release | One committed up | First native-ready update with fresh body/world/smoke checks |
| Cancellation | Suppressed shooting release and neutral cleanup | Ownership, physical input and native cancellation rules |

The internal `Primed` phase records a follow-up prepared during cooldown.
`NeedsReadinessObservation()` controls inexpensive state observations.
`NeedsFreshTarget()` separately controls geometry work.

If native fire checking blocks every cooldown update, preparation must wait
for an accepted update. The next press and release then occupy separate
eligible updates. Native preparation determines which firing mode is active.

## BR frame-time work already present

`BattleRoyaleTraceDistance()` limits the ray to the smaller of the visible
camera distance and `max(500, weaponRange + 32)`. The 32-unit allowance
accounts for camera separation. An unavailable or invalid native range
retains the camera-derived distance. BR detection is cached for the local
match/pawn lifetime.

An idle automatic weapon waits for readiness before target physics. Held
automatic fire still checks fresh geometry. Pending release shots and
prepared cooldown holds defer repeated body, world and smoke checks.
Each release-fire cycle can run one extra target check when preparing an
acknowledged follow-up during cooldown, followed by its actual release check.

The enemy membership check remains a bounded walk of up to 512 entries after
a candidate is found. Native broad traces, detailed body tests, world checks
and smoke checks still have device-dependent costs. Crowded BR scenes need
profiling to establish how much each contributes.

## Glass behavior already present

The native fallback recognizes material classes `Glass`, `BreakableGlass`
and `PhysicalPenetrateGlass`, with values 1, 12 and 33. It uses bounded
multi-hit queries with the original body/world layer masks when a nearest
contact blocks the path.

The budget is two panes and 32 contacts. The world check starts from the
original camera ray and independently validates its path. Unknown material,
unloaded collider, incomplete results, an exhausted buffer or unavailable
bindings retain the obstruction. The weapon's own native logic determines
glass breakage, penetration and damage.

The current follow-up change uses these existing target checks at preparation
and release. A target that becomes obstructed during cooldown is checked
again before a firing release.

## Native compatibility

The supported target is the supplied ARM64 1.6.59 image with GNU build ID
`056d6390cc712865`. Verification covers 50 core fingerprints, seven optional
glass fingerprints and seven compatibility probes. The supplied image is
`/workspace/dumps/libunity.so`.

Readiness uses the verified native virtual `CanFire` entry and its metadata.
The supported preparation handlers are the existing base and COD sniper
handlers. Native burst and charged/specialized preparation remain outside
the adapter's supported set. Core burst test cases exercise controller
policy only.

The new change introduces internal frame/state fields and reuses existing
verified calls and clip layout. Executable verification, context generations,
native fire-check acceptance, native readiness and full ADS continue to
participate in the firing decision. Manual fire retains priority.

## Changed files for the follow-up fix

| File | Change |
| --- | --- |
| [TriggerCore.h](../src/main/jni/Trigger/TriggerCore.h) | Adds sampled-readiness metadata, prepared cooldown state, readiness recovery and evidence-aware rejection |
| [TriggerRuntime.cpp](../src/main/jni/Trigger/TriggerRuntime.cpp) | Separates readiness observations from target physics and permits freshly checked follow-up preparation |
| [NativeDriver.cpp](../src/main/jni/Trigger/NativeDriver.cpp) | Marks real readiness samples and checks the current owning clip before rejection |
| [trigger_tests.cpp](../tests/trigger_tests.cpp) | Adds consecutive-shot timelines, cooldown work budgets, missing-confirmation recovery and cancellation regressions |
| [sniper_input_test.cpp](../tests/sniper_input_test.cpp) | Verifies prepared native button bytes, separate edges and evidence-aware rejection |

The main-based runtime also reports the prepared sniper cooldown through its
existing cooldown status. The Free Trial port retains its diagnostic API.
Documentation updates are in this file, [triggerbot.md](triggerbot.md) and
the root [compact handoff](../HANDOFF_MEMORY.md).

## Verification completed for this revision

| Check | Main-based | Free Trial |
| --- | --- | --- |
| Trigger behavior under AddressSanitizer and UndefinedBehaviorSanitizer | 4,401 checks passed | 4,392 checks passed |
| Trigger dot appearance | Passed | Passed |
| Prepared native input | 46 checks passed | 46 checks passed |
| Native profile host checks | 1,046 checks passed | 1,046 checks passed |
| Permanent profile rejection | 4 checks passed | 4 checks passed |
| Free Trial window | Variant-specific | 32 checks passed |
| Supplied ELF trigger fingerprints and compatibility probes | 57 and 7 matched | 57 and 7 matched |
| Supplied ELF shared Hitbox records | 17 matched | 17 matched |
| Android NDK r22 ARM64/API 21 build | Passed | Passed |
| Native hardening verifier | Passed | Passed |

The two behavior-check totals reflect the main-based branch's additional
decision-feedback tests. Both variants run the new follow-up regressions.

The added tests include three consecutive scoped shots, Both and ADS Only,
callback and clip acknowledgement, 120 ms and 700 ms simulated cooldowns,
and 30/60/120 Hz update schedules. They verify first-ready release, distinct
press/up edges, duplicate-update suppression, deferred cooldown physics,
missing-confirmation recovery, unavailable readiness, manual takeover,
settings changes, context changes, lost targets, cover and smoke.

The initial failing regression is saved at
`/workspace/license-hardening/logs/sniper-followup-before.log`.
Current host/build logs use `sniper-followup-main-tests.log`,
`sniper-followup-main-android.log`, `sniper-followup-trial-tests.log`
and `sniper-followup-trial-android.log` in the same directory.

Built libraries are at
`/dev/shm/npc-br-trigger-main-libs/arm64-v8a/libnets.so` and
`/dev/shm/npc-br-trigger-trial-libs/arm64-v8a/libnets.so`.
These are native build artifacts. Installed APK and gameplay validation
remain pending.

### Reproduce the checks

Run the host suite from either worktree.

```sh
TMPDIR=/workspace/license-hardening/build-tmp sh tests/run-trigger-tests.sh
python3 tests/verify-trigger-profile.py /workspace/dumps/libunity.so
python3 tests/verify-hitbox-profile.py /workspace/dumps/libunity.so
```

Run the Free Trial lifecycle suite from the Free Trial worktree.

```sh
TMPDIR=/workspace/license-hardening/build-tmp sh tests/run-trial-tests.sh
```

Build the main-based native library from its worktree.

```sh
TMPDIR=/workspace/license-hardening/build-tmp \
/workspace/toolchains/android-ndk-r22/ndk-build \
  NDK_PROJECT_PATH=. \
  APP_BUILD_SCRIPT=src/main/jni/Android.mk \
  NDK_APPLICATION_MK=src/main/jni/Application.mk \
  NDK_OUT=/dev/shm/npc-br-trigger-main-obj \
  NDK_LIBS_OUT=/dev/shm/npc-br-trigger-main-libs -j2
python3 tools/verify-native-hardening.py \
  /dev/shm/npc-br-trigger-main-libs/arm64-v8a/libnets.so
```

For the Free Trial build, run from its worktree and use the existing
`npc-br-trigger-trial-obj` and `npc-br-trigger-trial-libs` directories.

## Device validation still needed

Record the weapon, attachments, input mode, application revision and frame-rate
setting. Compare several consecutive manually fired scoped shots against the
same weapon's trigger-driven shots while staying scoped. Measure the first,
second and third local discharge intervals.

Exercise reload, the final round, scope exit, target exit, weapon switch,
death/respawn and manual fire takeover during preparation and cooldown.
Check real ordinary/breakable glass, solid cover behind glass and smoke.
For BR performance, compare empty terrain and crowded scenes using the same
rendering settings and thermally stable device conditions.

A device trace should correlate input-update time, release submission,
readiness, acknowledgement serial, clip changes and the decision status.
Per-stage body/world/smoke durations and query counts would locate remaining
CPU spikes. Such timing instrumentation has not been added in this revision.

Host sanitizer checks establish controller behavior and the tested memory
operations. Android compilation and ELF checks establish build/profile
consistency. Live hook behavior, actual FPS, thermals, visual timing and
server-side damage remain unmeasured.

## Compact handoff memory

- Work from `/workspace/NPC-license-hardening` on `fix/br-trigger-performance` or the separate Free Trial worktree at `/dev/shm/npc-br-trigger-trial`.
- The latest user problem is a slow second sniper shot while hardscoping. Both and ADS Only are relevant. Earlier BR FPS and glass reports remain part of device validation.
- Runtime fix commits are `78f4930` and `edbce37`. Documentation follows in subsequent local commits. Remote heads observed on 2026-11-21 are main `c7c0162` and Free Trial `03c13b9`.
- An acknowledged release shot can prepare its next non-firing press during an eligible cooldown update. `Primed` holds that press and defers target physics until native readiness returns.
- Release still requires completed ADS, accepted input and fresh body/world/smoke checks. Native bolt, cooldown and weapon input rules determine eligibility.
- Pending release recovery can use a genuinely sampled blocked-to-ready cycle. `readinessSampled` prevents skipped calls from supplying false evidence. The 500 ms fallback remains for uncertain attempts.
- Post-update rejection respects current clip consumption from the same owning fire component. Serial and clip evidence are scoped by context generations.
- Main-based and Free Trial builds, sanitizer suites, prepared input tests, trial window tests and pinned ELF checks pass. Current counts and log paths are in the verification section.
- Device latency, crowded-BR FPS, installed APK behavior and all real glass cases require measurement. A recording and native timing trace for this report are unavailable.
- New source comments, new native offsets and new dependencies were unnecessary for this fix. Preserve artist notices and unrelated source content during future edits.
