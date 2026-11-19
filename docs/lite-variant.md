# SHAI ETHNIR Lite

An independent reduced-feature edition of the current SHAI ETHNIR source.

## Branch and app identity

- Branch: `variant/shai-ethnir-lite`
- Display name: SHAI ETHNIR LITE
- Android application ID: `com.android.support.lite`
- Version name: `2.9-lite`
- Java classes keep the existing `com.android.support` package for source compatibility.

## Included controls

The Lite sidebar has three pages:

- Home: basic status and frame-time summary
- ESP: Player/bot boxes, lines, names, health bars, BR armor bars, distance, enemy count, basic box/line/health placement, and player/bot colors
- Settings: license information and existing config save/load controls

The ESP preview renders only on wider displays to preserve a usable menu on Android phones.

## Omitted features

Lite does not initialize the extra licensed hook collection, native triggerbot, aimbot, skins, memory tools, advanced world/item ESP, throwable tracking, or BR fast pickup/range hooks. Those features remain in the full `main` edition.

The `LiteProfile.h` allowlist normalizes hidden settings each rendered frame, including settings loaded from earlier config files. The native menu/login and license checks remain in place. Lite still contains inherited shared source dependencies, so the APK size may not shrink much until a separate source-pruning pass.

## Native library loading

The main edition's Java launcher fetches a remotely signed `libZANEV1.so`. The Lite launcher instead loads its own bundled `libnets.so` via `System.loadLibrary("nets")`. Building the Lite application must package that native library. Lite does not automatically use the main edition's native distribution endpoint.

The Android app launcher still references the existing game activity. If that activity is unavailable to the current class loader, launch testing will need a separate launcher integration adjustment. A successful ARM64 native library build by itself is not proof the APK launches or attaches to the game.

## Build and verification

The repository uses Android NDK r22 and arm64-v8a. Use the existing AIDE project workflow and confirm the resulting APK includes `lib/arm64-v8a/libnets.so`.

```sh
sh tests/run-lite-tests.sh
```

GitHub Actions also compiles and publishes the native test library as `shai-ethnir-lite-arm64`.

Verify the three-tab menu, native loading, menu licensing, player and bot colors, health bar placement, MP/BR behavior, frame times, and saving/restoring Lite ESP configuration. The original main edition stays unchanged.
