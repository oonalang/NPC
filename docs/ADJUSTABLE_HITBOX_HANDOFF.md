# Adjustable hitbox work

## Request and baseline

The user wants adjustable enemy collision sizes. Direct headshots and body shots must retain their normal classification. They explicitly rejected an always-headshot feature.

Current worktree: `/workspace/NPC-license-hardening`, branch `feature/license-session-hardening`, based on main `fdd0c16`. The original implementation was published in `c04aa18`; main's capsule crash correction is retained. Other worktrees and the newer Wiz work on main are preserved.

## Reference evidence

The user supplied [this Drive folder](https://drive.google.com/drive/folders/1L8BHYh1C7b3n8mSClkf7oLKheLwZqjIu). Both libraries were downloaded for static inspection, never loaded or executed, and are outside the repository.

| File | SHA-256 | Finding |
| --- | --- | --- |
| libVELCRACKED.so | `836da3250bcb872b90d304d607d95884be91842166a3bdbefaabb682099a493a` | Verified adjustable-hitbox callback and slider |
| libECLIPSESKIN.so | `1353e15f23ce95e3bd3d44e5854fab375c840d67a3a417864b33b629bd41af78` | Located Extend Hitbox UI; its underlying implementation was not fully established |

VEL's `##hitSz` string at RVA `0x36B231` leads to the slider call at `0xFB9A0`. Its limits are 1.0 and 5.0, and its float is at `0x5431EC`. The toggle is at `0x5431E8`. The callback at `0x110CC0` checks hit type 1, obtains the target's avatar-collider list at `0xA8`, multiplies each collider's float at `0x14`, calls the original query, and restores the saved values. It does not rewrite HitGroup. The separate AR/SMG Size and SR/SG Size controls belong to aim assist.

The reference's raw list traversal and shared-object writes were not copied. Its code alone does not establish current-game compatibility or server acceptance.

The previous NPC Hitbox hook returned true immediately whenever enabled. It supplied no new geometry or impact information. This is a plausible explanation for the reported body-only behavior, not a device-confirmed diagnosis.

## Reported firing crash

The user enabled Hitbox at 1.21× and crashed while shooting just beside an enemy. No weapon name or native crash log was supplied, and the crash was not reproduced on an Android device.

The old enlarged-hit callback called `Transform.TransformPoint` itself before the original shape query, skipping the original capsule method's Unity destroyed-parent check. It also mapped ray endpoints and handed the native dispatcher a temporary copied `HitResult`. The correction removes that callback and both behaviors. These were unsafe assumptions on the reported near-miss path; without a stack trace, they are not a proven crash diagnosis.

The user subsequently confirmed that the capsule correction works without crashing and direct headshots work. They reported that changing the slider appears ineffective and asked whether head geometry also grows. This device report concerns the prior capsule-only build, not the new sphere/OBB extension below.

## Implementation

- Memory → Weapon → Hitbox now exposes Hitbox Size from 1.00× to 5.00×. The default and legacy-config fallback are 1.00×, which preserves stock geometry.
- The weapon query is widened on its single, primary call, matching the reference: one native query against the scaled geometry, with no stock attempt first, no retry on a miss, no enemy-list membership test and no fire-component ownership test. A living target whose region list can be read is widened wherever it appears. Both the base and instant-fire component implementations are hooked, and a re-entrant query runs under the existing scope instead of widening twice.
- Every region in the traced pawn's list is scaled; there is no HitGroup or active-flag filter. Direct hits keep the game's own result, rays, metadata and `HitResult` construction.
- A thread-local scope restricts enlargement to the selected target's avatar colliders. Lists use the managed array's actual `0x20` element offset, with bounds checks and no persistent object cache.
- The original avatar-capsule method validates its parent and computes world-space geometry. A nested thread-local scope scales only the scalar axis length and radius passed to its original capsule primitive. The incoming ray, rotation, center, result pointer and metadata are forwarded unchanged. The game constructs the hit result; no result is copied, remapped or synthesized. Recursive primitive calls use neutral scale to prevent double enlargement.
- The regular capsule, sphere and capsule-as-OBB paths use their original native primitive methods with enlarged scalar radii/axis lengths or local half-extents. Standard `RaycastAvatarCollider_Box` and legacy `Box_Old_Error` remain stock. Head group `1` is eligible on supported shapes; the actual head shape selected by a live game has not been observed. Collider objects, shared dimensions, transforms, HitGroup values, damage multipliers and aim direction are never rewritten. Shields are excluded. Normal world obstruction processing remains in the original weapon query.
- Sphere and OBB support install independently after the working core. An encoded sphere avatar method remains pending until it decodes; extension failure does not disable regular capsules. The UI reports pending/unavailable sphere support, and enabled Hitbox retries pending installation once per second. Primitive wrappers neutralize nested primitive calls so capsule end-spheres cannot be enlarged twice.
- Settings publish to one atomic snapshot. After thread-local initialization, the per-query path allocates no heap memory and uses bounded stack storage. Separate threads have separate scopes.
- JSON and INI store `ExtraMenu.HitboxScale`. Values are finite and bounded. JSON loading also accepts the existing saved `KineticArmor` key as well as `Kinetic`; the old mismatch prevented a saved config from completing its load.
- No new source comments were added.

## Native compatibility

Pinned game: 1.6.59, GNU build ID `056d6390cc712865`. Dump and ELF used: `/workspace/dumps/Dump-1.6.59.cs` and `/workspace/dumps/libunity.so`.

| Method | RVA | Purpose |
| --- | --- | --- |
| WeaponFireComponent.SingleLineCheckPhysics | `0xC2BE0C8` | Local weapon query scope |
| WeaponFireComponent_Instant.SingleLineCheckPhysics | `0xD52E8E4` | Instant weapon query scope |
| PhysicsUtil.RaycastAvatarCollider_Capsule | `0x66FCA40` | Original parent validation and target-specific scalar scope |
| PhysicsUtil.RaycastCapsule(Vector3, Quaternion, float, float, Vector3, Vector3, ref HitResult) | `0x66F2A78` | Scale capsule arguments, preserve native result handling |
| PhysicsUtil.RaycastAvatarCollider_Sphere | `0x66FD9B0` | Target-specific sphere scope; encoded on disk |
| PhysicsUtil.RaycastSphere(Vector3, float, Vector3, Vector3, float, ref HitResult) | `0x66F286C` | Scale radius; preserve origin/start/direction/length and result |
| PhysicsUtil.RaycastAvatarColliderOBB_Capsule | `0x66FC548` | Native validation, local transform and native HitResult construction |
| PhysicsUtil.RaycastAABB(Vector3, Vector3, Vector3, Vector3, out float) | `0x66F82A8` | Scale only local half-extents; preserve native hit fraction |
| GamePlay.get_MatchGame | `0x801191C` | Enemy-list membership |
| GamePlay.get_LocalPawn | `0x8012814` | Local ownership |

The two weapon entries and optional sphere avatar entry are encoded in the disk ELF. Installation waits until their runtime prefixes differ from the encoded bytes and resemble supported ARM64 stack prologues. The core build ID and seven plaintext anchors must match; the optional sphere and OBB paths add three and four plaintext anchors respectively, for 17 disk records total (14 plaintext, three encoded). Unsupported core builds and partial core failures disable enlargement; an absent loader entry or undecoded entry remains retryable. Extension failures stay isolated from the core.

The capsule caller and callee were disassembled from the pinned ELF. An Android NDK r22 compiler probe matches the observed AArch64 layout: center in s0–s2, quaternion in s3–s6, axis length in s7, radius at incoming SP+0, start at SP+8, end at SP+24, result in x1, metadata in x2, with the static receiver slot in x0. The probe and assembly are under `/workspace/hitbox-reference/capsule-abi.*`. This establishes a static ABI match, not execution of a live game trampoline. The decoded weapon bodies still require runtime verification.

The sphere primitive's callee was disassembled and checked with NDK r22: origin s0–s2, radius s3, start s4–s6, direction incoming SP+0/4/8, length SP+16, result x1, metadata x2, receiver x0. Its avatar signature is established by the dump; its encoded live body remains device-dependent. The OBB avatar's complete plaintext call sequence and AABB callee establish center s0–s2, half-extents s3–s5, start incoming SP+0/4/8, end SP+16/20/24, hit-fraction pointer x1, metadata x2, receiver x0. Independent NDK compiler probes are at `/workspace/license-hardening/{sphere,aabb}-abi.*`.

Collider size field: the reference multiplies a float at `+0x14` on the entries it walks from `0xA8`, but that walk starts at the list's item pointer with no managed-array header, which does not match this game's `0x20` element offset, so the offset cannot be carried over as-is. The size field of the pinned build is not established by anything in this repository: `/workspace/dumps/libunity.so` and `Dump-1.6.59.cs` are outside it and absent here, and the documented layout stops at HitGroup `0x20`, local center `0x24`, parent `0x40`, active `0x50`. Rather than write an unverified offset into live objects, sizing is applied to the scalar axis length, radius and half-extents that the game's own avatar-collider methods pass to `RaycastCapsule`, `RaycastSphere` and `RaycastAABB`, which is the point where the engine provably consumes the geometry it derived from the collider. Settling the field offset needs `libunity.so` for build ID `056d6390cc712865` (or the dump): disassemble `RaycastAvatarCollider_Capsule` at `0x66FCA40` and follow the axis-length/radius loads back to their collider offsets, then the stored-field path can be added and the primitive path demoted to a fallback.

Relevant layout: fire-component owner `0x28`; enemy pawn alive `0x550`; match enemy list `0x178`; target avatar list `0xA8`; avatar HitGroup `0x20`, local center `0x24`, parent transform `0x40`, active flag `0x50`. `HitResult` has point `0x0`, normal `0xC`, distance `0x18`, material `0x1C`, collider `0x20`, size `0x28`. Native wrappers forward the metadata argument and use the static-method receiver slot present in this game's ABI.

## Validation

Commands:

```sh
sh tests/run-hitbox-tests.sh
sh tests/run-memory-tab-tests.sh
sh tests/run-home-layout-tests.sh
sh tests/run-trigger-tests.sh
python3 tests/verify-hitbox-profile.py /workspace/dumps/libunity.so
```

The hitbox suite runs with AddressSanitizer and UndefinedBehaviorSanitizer. It covers capsule/sphere/half-extent sizing, invalid/overflow dimensions, direct head/body priority, misses, bounded managed lists, nested/concurrent scopes, normalization, JSON round trips, legacy defaults, loader/decode retry, build/anchor rejection, one-time installation and partial failure. Production wrapper tests prove that 1.00×, 1.21×, 2.00× and 5.00× slider settings reach sphere/OBB math, alter side-miss outcomes, retain head group `1`, and leave direct hits as one original stock query. Optional extension failure preserves capsule behavior; nested capsule/sphere calls do not double-scale.

`hitbox_native_test.cpp` exercises the actual C++ replacement callbacks with stubbed game methods. It covers the reported 1.21× side-shot path for base and instant fire, direct hits without retry, destroyed/missing parents, inactive/shield/unselected regions, local ownership and live-enemy gating, unchanged rays/result pointers/metadata, and reentry/unwind restoration. These tests do not execute the game library or native hook trampolines. The previous inverse-ray-only geometry tests were removed with that implementation.

Current extension results: expanded hitbox suite and ARM64 Android 21 NDK r22 build passed. Offline verification now matches all 17 hitbox records plus 46 trigger fingerprints and seven compatibility probes, including the trigger's read-only clip-count anchor. Current Memory-tab layout/input and trigger regression suites also passed; results are recorded in the task logs. Existing custom-widget compiler warnings remain in the UI tests.

Current ARM64 build command, from the worktree root:

```sh
/workspace/toolchains/android-ndk-r22/ndk-build \
  NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=src/main/jni/Android.mk \
  NDK_APPLICATION_MK=src/main/jni/Application.mk \
  NDK_OUT=/workspace/license-hardening/android-obj \
  NDK_LIBS_OUT=/workspace/license-hardening/android-libs -j2
```

Current build artifact: `/workspace/license-hardening/android-libs/arm64-v8a/libnets.so`, included with its signed manifest in the complete private panel ZIP. Current build/test logs and analysis scripts are under `/workspace/license-hardening/`. Build products and reference libraries are not committed.

## Remaining device checks and limits

No Android device or active game session was available for the extension. The user confirmed the prior capsule correction stopped their crash and retained direct headshots; live sphere decoding, new hook/trampoline execution, near-head enlargement, actual damage registration, FPS/latency impact and screenshots still need device verification. This branch includes an incompatible license protocol update that requires coordinated server/client deployment; do not merge it to main as though rollout were complete.

The game's original broad-phase target selection and weapon range still apply. Enlargement affects detailed shapes of a target already supplied to the weapon query; a ray that never selects that target cannot become a hit through this feature. Server-side hit validation can reject client-side results, especially at large multipliers. This does not change server hitboxes or damage values.

Next device pass: test disabled and 1× first, then repeat side shots at 1.21× with the new sphere/OBB paths. Capture any signal, fault address, build ID and top native frames if the crash returns. Compare direct head/body shots at 1.25×, 1.5× and 2×; then near-head and near-body misses with rifles, shotguns and snipers. Check walls, shields, teammates, death/respawn, weapon switching, match changes and toggling off mid-session. Confirm actual target health loss, not only local hit markers. If the UI stays at Waiting for game hit checks, obtain decoded entry bytes and the build ID; do not bypass the readiness guard or guess new offsets.
