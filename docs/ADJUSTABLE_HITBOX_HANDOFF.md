# Adjustable hitbox work

## Request and baseline

The user wants adjustable enemy collision sizes. Direct headshots and body shots must retain their normal classification. They explicitly rejected an always-headshot feature.

Worktree: `/workspace/NPC-hitboxes`. Branch: `feature/adjustable-hitboxes`. Started from main `306e6cf8b4e423ab7e74d6ff0c3a0803c74b2c49` (Keep the Wiz animation render sheets). The older `feature/esp-3d-runtime-physics` checkout is preserved separately; its character changes were not imported into this branch.

## Reference evidence

The user supplied [this Drive folder](https://drive.google.com/drive/folders/1L8BHYh1C7b3n8mSClkf7oLKheLwZqjIu). Both libraries were downloaded for static inspection, never loaded or executed, and are outside the repository.

| File | SHA-256 | Finding |
| --- | --- | --- |
| libVELCRACKED.so | `836da3250bcb872b90d304d607d95884be91842166a3bdbefaabb682099a493a` | Verified adjustable-hitbox callback and slider |
| libECLIPSESKIN.so | `1353e15f23ce95e3bd3d44e5854fab375c840d67a3a417864b33b629bd41af78` | Located Extend Hitbox UI; its underlying implementation was not fully established |

VEL's `##hitSz` string at RVA `0x36B231` leads to the slider call at `0xFB9A0`. Its limits are 1.0 and 5.0, and its float is at `0x5431EC`. The toggle is at `0x5431E8`. The callback at `0x110CC0` checks hit type 1, obtains the target's avatar-collider list at `0xA8`, multiplies each collider's float at `0x14`, calls the original query, and restores the saved values. It does not rewrite HitGroup. The separate AR/SMG Size and SR/SG Size controls belong to aim assist.

The reference's raw list traversal and shared-object writes were not copied. Its code alone does not establish current-game compatibility or server acceptance.

The previous NPC Hitbox hook returned true immediately whenever enabled. It supplied no new geometry or impact information. This is a plausible explanation for the reported body-only behavior, not a device-confirmed diagnosis.

## Implementation

- Memory → Weapon handling → Hitbox now exposes Hitbox Size from 1.00× to 5.00×. The default and legacy-config fallback are 1.00×, which preserves stock geometry.
- Replaced the unconditional true hook with a normal query followed, only on a miss, by an enlarged detailed query. Existing direct hits take priority across the whole target, preventing an enlarged body region from replacing an existing direct head hit.
- Enlargement applies only to hit type 1 from the local pawn's weapon fire component, against a living pawn in the game's enemy list. Both the base and instant-fire component implementations are hooked. Nested calls do not retry recursively.
- A thread-local scope restricts enlargement to the selected target's avatar colliders. Lists use the managed array's actual `0x20` element offset, with bounds checks and no persistent object cache.
- Uniform shape enlargement uses inverse-scaled query endpoints about each collider's world center. The original collision routine receives the real collider object. Returned positions are transformed back onto the original world-space shot segment. Distance is recalculated from the hit point because the native capsule path can leave its distance field unchanged.
- Collider objects, dimensions, transforms, HitGroup values, damage multipliers and aim direction are never rewritten. Shields are excluded. Normal world obstruction processing remains in the original weapon query.
- Settings publish to one atomic snapshot. After thread-local initialization, the per-query path allocates no heap memory and uses bounded stack storage. Separate threads have separate scopes.
- JSON and INI store `ExtraMenu.HitboxScale`. Values are finite and bounded. JSON loading also accepts the existing saved `KineticArmor` key as well as `Kinetic`; the old mismatch prevented a saved config from completing its load.
- No new source comments were added.

## Native compatibility

Pinned game: 1.6.59, GNU build ID `056d6390cc712865`. Dump and ELF used: `/workspace/dumps/Dump-1.6.59.cs` and `/workspace/dumps/libunity.so`.

| Method | RVA | Purpose |
| --- | --- | --- |
| WeaponFireComponent.SingleLineCheckPhysics | `0xC2BE0C8` | Local weapon query scope |
| WeaponFireComponent_Instant.SingleLineCheckPhysics | `0xD52E8E4` | Instant weapon query scope |
| PhysicsUtil.RaycastAvatarCollider | `0x66FDB60` | Scaled detailed shape queries |
| Transform.TransformPoint | `0x532D29C` | Collider's world-space center |
| GamePlay.get_MatchGame | `0x801191C` | Enemy-list membership |
| GamePlay.get_LocalPawn | `0x8012814` | Local ownership |
| PhysicsUtil.RaycastAvatarCollider_Capsule | `0x66FCA40` | Plaintext layout/ABI anchor |

The first four entries are encoded in the disk ELF. Installation waits until their runtime prefixes differ from the encoded bytes and resemble supported ARM64 stack prologues. Three plaintext anchors and the build ID must match. This is a readiness guard, not complete verification of the decoded method bodies or a proof of the hook ABI. An absent loader entry remains retryable; unsupported builds and partial installation failures disable enlargement. The old force-success fallback is removed.

Relevant layout: fire-component owner `0x28`; enemy pawn alive `0x550`; match enemy list `0x178`; target avatar list `0xA8`; avatar HitGroup `0x20`, local center `0x24`, parent transform `0x40`, active flag `0x50`. `HitResult` has point `0x0`, normal `0xC`, distance `0x18`, material `0x1C`, collider `0x20`, size `0x28`. Native wrappers forward the metadata argument and use the static-method receiver slot present in this game's ABI.

## Validation

Commands:

```sh
sh tests/run-hitbox-tests.sh
sh tests/run-memory-tab-tests.sh
sh tests/run-trigger-tests.sh
python3 tests/verify-hitbox-profile.py /workspace/dumps/libunity.so
```

The new host tests cover enlarged sphere/box/capsule intersections, impact positions and distances, segment limits, malformed results, original head/body priority, real misses, bounded managed lists, nested/reentrant scopes, concurrent scopes, normalization, JSON round trips, legacy defaults, loader/decode retry, build/anchor rejection, one-time installation and partial failure. AddressSanitizer and UndefinedBehaviorSanitizer are enabled for the new suites. Loader tests use a synthetic module and stub hook installer; they do not execute game instructions.

Verified results: the new suites, Memory tab layout/scroll/tap suite, existing trigger suite (111 behavior checks, 23 prepared-input checks, 39 profile checks and 4 permanent-rejection checks), and ARM64 Android 21 NDK r22 build passed. Offline verification matched all 7 hitbox disk records plus the existing 45 trigger fingerprints and 7 probes. Existing custom-widget compiler warnings remain in the Memory UI test build.

The existing trigger test harness had obsolete pending/failure stub signatures. These were updated to the current NativeDriver interface, and that header is now included to catch future signature drift.

ARM64 build command, from `src/main`:

```sh
/workspace/toolchains/android-ndk-r22/ndk-build \
  NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=jni/Android.mk \
  NDK_APPLICATION_MK=jni/Application.mk \
  NDK_OUT=/workspace/hitbox-reference/android-obj \
  NDK_LIBS_OUT=/workspace/hitbox-reference/android-libs -j2
```

Build artifact: `/workspace/hitbox-reference/android-libs/arm64-v8a/libnets.so`. Build/test logs and private analysis scripts are under `/workspace/hitbox-reference/`. Build products and reference libraries are not committed.

## Remaining device checks and limits

No Android device or active game session was available. Runtime instruction decoding, hook ABI, actual damage registration, FPS/latency impact and screenshots have not been verified on a device. Do not describe this branch as proven in-game or merge it solely on the host tests.

The game's original broad-phase target selection and weapon range still apply. Enlargement affects detailed shapes of a target already supplied to the weapon query; a ray that never selects that target cannot become a hit through this feature. Server-side hit validation can reject client-side results, especially at large multipliers. This does not change server hitboxes or damage values.

Next device pass: test disabled and 1× first; compare direct head/body shots at 1.25×, 1.5× and 2×; then near-head and near-body misses with rifles, shotguns and snipers. Check walls, shields, teammates, death/respawn, weapon switching, match changes and toggling off mid-session. Confirm actual target health loss, not only local hit markers. If the UI stays at Waiting for game hit checks, obtain decoded entry bytes and the build ID; do not bypass the readiness guard or guess new offsets.
