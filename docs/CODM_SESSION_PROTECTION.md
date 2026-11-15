# CODM session protection

## User-visible behavior

Settings → Protect CODM login is an opt-in preference, off by default. It asks the game's server to keep the current authenticated CODM session from being replaced by another login. It is separate from mod-license enforcement.

The status says Protection request sent (server decides) after the original network sender accepts the correctly typed request. This is a local send/queue confirmation. It is not proof of server acceptance or proof that another device cannot log in.

Turning the option off restores the game's last observed session-control request. When no original request has been observed, it sends an explicit disable request. If the game itself requests protection, disabling the mod option preserves that game request.

JSON and INI persist `ExtraMenu.ProtectCodmLogin`. Old configs default to false. The preference is also editable when the backend is unsupported; status reports that limitation without disabling unrelated controls.

## Request path

The implementation uses `Network.ForbidKickOffHandler.ForbidKickOff(bool)`, which constructs `msg.CSClientForbidKickOffReq`, writes its integer state and calls the original LobbySender.SendToZone method. Protocol constants identify the request and response as 5083/5084.

Disassembly confirms that StageManager.LeaveFrontEnd calls it with true and the BR, DMZ and PVE match-time controller initializers call it with false. The new option uses this existing request; it does not suppress a kick notice, bypass a failed login, change authentication tokens, reconnect a revoked session or launch a second process.

The uploaded Python script only launches/reopens CODM and optionally taps the screen. It implements no account/session locking. Eclipse's claimed antileak callback is still not positively identified in the supplied binary; equivalence with this implementation has not been established.

## Native integration

- Requests run after the original ZoneServerHandler.Tick on its existing game-network execution thread. The render/UI thread only publishes an atomic preference.
- The current handler must have its logged-in flag set and a non-null Apollo connector. A context identity includes the handler, connector and a lifecycle generation.
- Disconnect, disconnect notification, reset and shutdown invalidate context before forwarding the original callback. Return values and arguments are preserved. Pending work from an older connection cannot apply to a later one.
- While enabled and authenticated, normal game session-control calls retain protection by forwarding true. Their original argument is recorded as the game's preference. While disabled, those calls pass through unchanged.
- A scoped, thread-local observer checks the real request class and requested state around the original sender call. Unrelated messages and sender return values pass through unchanged. Missing/type-mismatched observations are marked unconfirmed and are not automatically retried.
- Explicit sender rejection permits at most three attempts, with retry delays of 1 s and 3 s. There is no periodic request loop after success or an unconfirmed outcome. A new toggle intent or authenticated connection can begin a new attempt sequence.
- Disabling restores the observed game value, or sends false if there was no observed value. It does not silently turn off a known game-owned protection request.
- Exceptions from an extra mod-initiated request become an unconfirmed outcome. Exceptions from an original game-initiated request keep their original propagation.
- Baselines come from original request arguments observed in the current authenticated context and are cleared at session boundaries. A previous account/connection's value is not reused. `ForbidKickOffHandler.CurrentStat` is not read as server state: the native sender does not maintain it.
- Cached connection identity is invalidated at disconnect/reset/shutdown. No account identifiers, credentials or authentication tokens are collected or logged.

Installation checks the game build ID and all 13 method/ABI/layout fingerprints before installing any hook. Unsupported builds remain inactive. A partial installation failure leaves already-installed wrappers as pass-throughs with the feature unavailable; it does not retry installing over those hooks.

## Current game mapping

The supplied dump and ELF are version 1.6.59, build ID `056d6390cc712865`. Their SHA-256 hashes match the supplied Hugging Face dataset exactly, as recorded in [ANTILEAK_REFERENCE_REVIEW.md](ANTILEAK_REFERENCE_REVIEW.md).

| Method/data | Current game RVA | Use |
| --- | --- | --- |
| ForbidKickOffHandler.ForbidKickOff | `0xC977A24` | Original request and game intent observation |
| LobbySender.SendToZone(IExtensible,uint,ulong) | `0x5B9ADF0` | Typed local-send observation |
| ZoneServerHandler.Tick | `0xC444A08` | Request execution on network update |
| ZoneServerHandler.Disconnect | `0xC448DD8` | Context invalidation |
| ZoneServerHandler.OnDisconnect | `0xC44AA14` | Server disconnect notification |
| ZoneServerHandler.Reset | `0xC44A988` | Context invalidation |
| ZoneServerHandler.Shutdown | `0xC44A748` | Context invalidation before release |
| ZoneServerHandler.get_IsZoneLogined | `0xC4445E4` | Verified `0x34` flag layout |
| Request-type GOT slot | `0xEA09AA0` | Request class identity, derived from native instructions |

The request's state is at `0x18`; the handler's logged-in flag is at `0x34`, and its connector is at `0x38`. The static managed request method uses the receiver slot present in this game's ABI: dummy receiver, bool, metadata. Sender and update metadata arguments are forwarded as well.

No addresses from the old mod binaries are treated as current game offsets. The current map comes from method signatures, native instruction inspection and byte comparisons against the supplied ELF.

## Changed files

Runtime and profile: `System/Hooks/SessionProtection.h`, `SessionProtection.cpp`, `SessionProtectionProfile.h`. Integration: `Main.cpp`, `Android.mk`, `Hacks/Feature/Init_Hacks.h`, `System/Hooks/Feats.h`, `System/UI/SettingsControls.h`, `System/Core/SaveConfig.h`, `System/Core/ConfigManager.h`, all under `src/main/jni`.

Validation: `tests/session_protection_test.cpp`, `session_protection_native_test.cpp`, `run-session-protection-tests.sh`, `verify-session-profile.py`, `settings_controls_test.cpp` and `hitbox_config_test.cpp`.

## Validation commands

```sh
sh tests/run-session-protection-tests.sh
sh tests/run-settings-controls-tests.sh
sh tests/run-hitbox-tests.sh
sh tests/run-trigger-tests.sh
python3 tests/verify-session-profile.py /workspace/dumps/libunity.so
```

The session policy tests use AddressSanitizer and UndefinedBehaviorSanitizer. The native adapter suite uses a synthetic module, stubbed original callbacks and a fake sender. It verifies login gating, no requests while off, typed request checks, preservation of a game-owned request, explicit enable/disable without a prior observed request, reconnect/reset/shutdown invalidation, unknown outcomes, exception handling, original forwarding, loader retry, incompatible build/prefix rejection and partial-hook failure. It sends no real network messages.

The Settings interaction suite taps the new preference on and off, checks that FPS preferences are unaffected, and verifies Reset Guest remains reachable. The config test checks persistence and the default for old files.

Verified results: the session policy and eight native-adapter modes passed with sanitizers; Settings interaction and existing hitbox/config suites passed; the existing trigger suite passed 111 behavior checks, 23 prepared-input checks, 39 profile checks and 4 permanent-rejection checks. Offline verification matched all 13 session records and the existing 45 trigger fingerprints/7 probes. The final ARM64 Android 21 NDK r22 build passed. The Settings test build retains existing custom-widget compiler warnings.

Android build, from `src/main`:

```sh
/workspace/toolchains/android-ndk-r22/ndk-build \
  NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=jni/Android.mk \
  NDK_APPLICATION_MK=jni/Application.mk \
  NDK_OUT=/workspace/hitbox-reference/android-obj \
  NDK_LIBS_OUT=/workspace/hitbox-reference/android-libs -j2
```

Artifact: `/workspace/hitbox-reference/android-libs/arm64-v8a/libnets.so`. Logs are under `/workspace/hitbox-reference/session-*.log`. Build outputs and reference binaries remain outside git.

## Remaining verification

No Android device, active CODM session or second-device login was available. Runtime hook behavior on Android, actual network delivery, server acceptance, whether another device is rejected, background behavior, timeout behavior and FPS impact remain unverified. Passing host tests and matching addresses do not establish those outcomes.

On a permitted test account, compare the normal second-device login behavior with the option off, then enable it in the lobby and wait for the request status before trying the second device. Confirm the first device's real server connection remains usable. Test the same sequence during and after a match, then toggle off and confirm the normal login policy returns. Include reconnect, logout, a fresh account session and app exit. A genuine server kick/logout must still work normally. Use the observed results to determine whether the server honors this request for the intended purpose; do not mask a failure by ignoring disconnection.
