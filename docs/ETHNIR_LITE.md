# ETHNIR LITE

ETHNIR LITE is the native Android edition on `feature/ethnir-lite`, based on main `f1f363e`. The user explicitly requested a new branch for this edition. Main and Free Trial are unchanged by this work. This branch builds Lite by default; it is not a runtime theme switch for the full edition.

## UI port

The implementation uses the supplied Celerity ImGui drawing code rather than a website or desktop executable. See [source provenance and font notices](../src/main/jni/Assets/LITE_UI_SOURCES.md).

The dark sidebar, centered section titles, outlined boxes, checkbox geometry, compact dropdowns, slider tracks/knobs, font glyphs and sidebar icons derive from the archive. The 705×500 layout scales to landscape displays and changes to one content column on narrow displays or with enlarged text. Panels and the sidebar support touch scrolling with hidden scrollbars. Touch gestures commit checkboxes, buttons and dropdowns on release; vertical swipes cancel activation and slider edits. Window placement remains centered through rotation/resizing. Nested ImGui font scales are normalized so text scales once.

The existing Home, ESP, aim, trigger, Memory, cosmetic and configuration controls use their existing NPC Config fields/runtime APIs. Memory keeps its existing Movement/Weapon/world functions; skins reuse the original category/application code. The aim trigger dropdown contains exactly the three enum values NPC supports. Backend compatibility and license checks remain in place. No desktop game logic, offsets or Windows dependencies were imported.

Login uses a compact centered matte card, a centered ETHNIR title and spaced LITE label, quiet secondary buttons and a white Authenticate button. The existing asynchronous signed-license request is unchanged; pending authentication shows three moving dots in the button. Loading uses four orbiting geometric tiles, moving ring segments and a small progress line, then folds into a drawn checkmark and fades into the menu. All graphics use ImGui geometry and the existing font atlas.

`Lite/StartupMotion.h` contains the named timing constants: a 0.90-second minimum loading presentation, 0.44-second finish and 0.18-second menu reveal. Native preparation can take longer. The finish requires real backend readiness; the menu stays disabled and game input stays blocked until its reveal completes. Android's disabled-animation setting is read once through the existing reduced-motion helper; it freezes waiting animation and skips presentation delays once the backend is ready. Clock reversal, nonfinite timestamps, reauthentication and revocation are covered by the controller tests. Preparation still means core hooks initialized and optional hook workers scheduled; unsupported optional features retain their existing unavailable states. Expiry brings login back and existing persistent runtime state is restored through its native gates.

## Lower rendering/startup cost

- No model, model simulation, anime texture asset, ESP preview render target, liquid glass, full-screen blur, authentication explosion, loading media or UI sound worker in the Lite build.
- The sidebar now uses real frosted background blur: a cropped GPU blit from the game's framebuffer followed by two separable Gaussian passes. Two RGBA8 textures are capped at 64×192 each (98,304 bytes combined payload), refreshed at 20 Hz for unchanged bounds (resize/move recaptures immediately) and allocated only after the menu becomes visible. There is no CPU pixel readback or fullscreen scene target. Capture stops while the menu is hidden or startup is active. Multisampled sources, invalid framebuffers and unavailable GL resources use the solid dark fallback. GLES state and context ownership are preserved; context-owned resources are reclaimed when their EGL context is destroyed. The rest of the menu stays matte.
- Direct GLES3 ImGui rendering by default. The existing optional Hide Record path is available in configs and allocates its overlay only when requested.
- Two Latin text font subsets and seven merged icon glyphs; one 512×256 RGBA atlas (524,288 bytes) at oversampling 1.
- Only the selected page is drawn. Home has numeric FPS/session information without the live chart. Local/Cloud music is opt-in and initializes its Android audio service only on first use; song rows use clipping. Turning it off stops playback. Once initialized, the existing music service retains its worker; a process restart returns to the default uninitialized state.
- Enemy projection/drawing is skipped when all supported visual options are off. Speed/gravity maintenance and existing license revocation restoration still run. Aim/trigger/native feature callbacks are unchanged.
- The periodic automatic screenshot/upload loop is removed from Lite's render loop. Existing explicit feedback service code is retained.

Measured ARM64 release library size with the same NDK: **3,924,168 bytes** versus main's **7,488,904 bytes**, about **47.6% smaller**. The host Home fixture makes no ImGui allocations over 600 warm frames. Host timings are not phone FPS measurements.

## Files

- `Lite/Menu.h`, `Widgets.h`, `WidgetBindings.cpp`: UI, geometry and touch-safe adapters for existing NPC controls.
- `Lite/StartupMotion.h`, `StartupView.h`: presentation clock, reduced-motion behavior, centered minimal login and vector loading/check animation.
- `Lite/FrostedSidebar.h/.cpp`: bounded cropped blur and GL state preservation.
- `Lite/NativeBackend.h`: original skin category bridge and optional Local/Cloud music UI.
- `Lite/Fonts.h`, `Assets/LiteFonts.S`, three `lite-*` font subsets: small single-atlas font setup.
- `Main.cpp`, `MainFeatureIncludes.h`, `Android.mk`: Lite startup/render path and excluded heavyweight modules/assets. Native feature modules are unchanged.
- `System/UI/Keyboard.h`: plain virtual keyboard for access keys, search, paths and configuration names. Includes letters, case, numbers, symbols, spaces and UTF-8-safe deletion. Physical keyboard input also works for native InputText fields.
- `System/UI/SkinTab.h`: Lite-specific font-scale normalization in its existing child window.
- `ImGui/ClayGlass.h`, `System/Audio/UiSounds.h`: compile-time exclusion of GLES glass and audio implementation under `ETHNIR_LITE`.
- `System/Core/SaveConfig.h`: save reports actual stream success to the new UI; existing JSON keys/load behavior are preserved.
- Android label/application ID, launcher URLs and `VerifiedLibrary.java`: separate Lite identity/download trust claim.

## Build and host setup

The baseline ARM64/API21/GLES3 device requirement is retained.

The application ID is `com.android.support.lite`; Java class package names remain `com.android.support`. Both editions can have separate app-private caches. Native licensing still uses the existing shared license panel/session protocol. No PHP or SQL migration is required for Lite.

The Lite launcher requires these files on the existing HTTPS host:

- `https://ethnir.x10.mx/native/lite/libZANEV1.so`
- `https://ethnir.x10.mx/native/lite/libZANEV1.manifest`

It accepts manifest product `ETHNIR_LITE` and rejects `SHAI_ETHNIR`, even if that other product's manifest is correctly signed. It preserves the existing public signing root and mandatory length/hash/signature verification. `tools/sign-native-release.py` retains its original default product for existing release workflows; Lite signing explicitly supplies `--product ETHNIR_LITE`.

```sh
ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=src/main/jni/Android.mk \
  NDK_APPLICATION_MK=src/main/jni/Application.mk \
  NDK_OUT=/tmp/ethnir-lite-obj NDK_LIBS_OUT=/tmp/ethnir-lite-libs -j2
python3 tools/sign-native-release.py /tmp/ethnir-lite-libs/arm64-v8a/libnets.so \
  --product ETHNIR_LITE --private-key /private/path/release-key.pem \
  --out /tmp/libZANEV1.manifest
```

Copy the built library as `libZANEV1.so` and upload both Lite files under `native/lite/`. Leave the full edition's `native/` files in place. Private keys belong outside source and release archives. Hosting has not been changed by this branch. The signed native ZIP prepared in the execution workspace is an upload package, not an APK.

For reproducible font import, install FontTools outside the Android runtime and run `python3 tools/export-lite-fonts.py /path/to/extracted/celerity`. The checked-in subsets are sufficient for ordinary Android builds.

## Validation

Executed on 2026-11-21:

| Check | Result |
| --- | --- |
| ARM64/API21 native build, NDK r22 | Pass |
| Native hardening/export inspection | Pass: RELRO, immediate binding, non-executable stack, stack protector, JNI-only exports; no OpenSLES dependency |
| Lite UI ASan/UBSan | Pass: seven pages, six viewport sizes, three scales, valid geometry/indices, balanced ImGui stacks, lazy callbacks, centered login, startup gating/animation/reduced motion at 15–120 FPS, reveal opacity/input, pending-state controls, touch taps/panning, slider endpoints, virtual keyboard, existing Config binding and idle allocation bounds |
| Surfaceless GLES3 UI rendering | Pass: page/login/preparation captures, font/sidebar textures only, real low-pass filtering and texture orientation, bounded allocation/cache/resize/release, MSAA fallback, and GL framebuffer/program/viewport/scissor/VAO/texture/sampler/unpack/color-mask/enable state restoration |
| Trigger suites | Pass: 4,401 behavior, 46 prepared input, 1,046 native profile and four permanent-rejection checks; dot appearance |
| Pickup/configuration/pinned native ELF | Pass: scoped overrides, JSON round trip/defaults/bounds and ten fingerprints |
| Hitbox/configuration | Pass: existing safety/shape handling and legacy JSON coverage |
| License/security/server/panel suites | Pass, including revocation restoration and signed session integration |
| Java sources against official Android 29 | Pass with Java 8 source/target options; existing deprecation/bootstrap warnings |
| Lite release verifier | Pass using an ephemeral test key: accepts a correctly signed Lite manifest; rejects other-product claims, tampering, framing changes and truncated/modified libraries |

Commands: `sh tests/run-lite-ui-tests.sh`; `LITE_GPU_TEST=1 sh tests/run-lite-ui-tests.sh /tmp/lite-frames`; `sh tests/run-lite-release-tests.sh`; `sh tests/run-trigger-tests.sh`; `sh tests/run-pickup-tests.sh /path/to/libunity.so`; `sh tests/run-hitbox-tests.sh`; `sh tests/run-license-security-tests.sh`; `python3 tools/verify-native-hardening.py /path/to/libnets.so`. GLES tests need EGL/GLES3 headers and a surfaceless Mesa context. Java tests need OpenSSL and a JDK; server integration needs PHP/OpenSSL.

The shared trigger/pickup/hitbox/license/Java results above belong to the initial Lite port; the sidebar/startup follow-up reruns the native build, hardening, UI sanitizer and GLES suites.

Capture the actual login/loading/check/reveal sequence using `LITE_GPU_TEST=1 LITE_CAPTURE_STARTUP=1 sh tests/run-lite-ui-tests.sh /tmp/lite-startup`. This emits 180 native frames at 30 FPS. Authentication/setup times and session data are fixtures; the presentation controller and all UI drawing are production code. The striped menu backdrop is a GPU blur test pattern; runtime blur uses the game image.

The GLES harness draws the actual Lite menu and uses fixture callbacks for the Android music service and in-game skin catalog. Screenshots were inspected for clipped headings, over-scaled text, row spacing and narrow-screen layout; padding and inherited font-scale faults were corrected. The retained skin application logic, actual audio playback, Hide Record, game integration and touch coordinate mapping still need a phone run. No installed APK, low-end device FPS, battery, thermal or total game-memory result is claimed. CJK/emoji fallback glyphs are intentionally absent from the small font atlas. The existing skin-catalog processing and native gameplay costs have not been rewritten as part of this UI port.
