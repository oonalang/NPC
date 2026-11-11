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

All 44 pinned entry fingerprints and seven diagnostic probes match the supplied
ELF when virtual addresses are mapped through its program headers. The checked
trigger RVAs map to file offsets at `RVA - 0x4000` in this image.

The C# declaration of a static method does not fully specify this binary's ABI.
Native instructions confirm that generated static query methods retain a dummy
object argument. Removing it shifts integer and object arguments. Camera rays
also use the ARM64 indirect aggregate return convention. Preserve these bindings.

Eight method bodies in the disk image are encoded. Their disk fingerprints are
checked by the offline verifier rather than compared with the loaded bodies.
Runtime validation requires the pinned build ID, readable executable ranges for
all 44 methods, and exact prefixes for the 36 stable methods, including all four
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

The adapter supports ordinary automatic and single-shot weapons whose input
preparation matches the verified base implementation. Shot readiness uses the
actual `CanFire` virtual function and its metadata, matching the native game's
dispatch, and requires that function to lie in the verified module's executable
segments. This preserves subtype restrictions without rejecting every override.
Burst and release-to-fire modes remain unsupported. The ammo query is reused
within each input decision to avoid a second native call and inconsistent samples.

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
