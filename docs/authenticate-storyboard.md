# SHAI ETHNIR authenticate storyboard

The updated storyboard supplies the stages and visual requirements. The reference
GIF supplies the motion sequence and appearance. Its original file is 283 × 207
pixels with 25 frames, each held for 280 ms: 7,000 ms in total. It therefore cannot
establish motion timing to a 60 Hz frame. Native captures are aligned to the same
visual stages; the confirmed individual durations determine the native timeline.

The written B4 description takes priority over the heavier storyboard thumbnail:
an irregular liquid-glass front, visibly refracted checkerboard, transparent
interior, RGB fringe, blue-white leading edge and warm orange trailing edge.
There is no galaxy or star background.

## Timing contract

All durations and derived boundaries are in `startup_ui::AuthTiming` in
`src/main/jni/System/UI/StartupState.h`. Times below are milliseconds after a
successful request is pressed, assuming verification returns before the spin
finishes. The sequence is sampled from monotonic time rather than frame counts.

| Step | Start | End | Duration | Required behavior |
| --- | ---: | ---: | ---: | --- |
| A1 | 0 | 0 | — | Existing idle authenticate button |
| A2 | 0 | 150 | 150 | Authenticate label fades out |
| A3 | 150 | 450 | 300 | Sign-in icon moves to center and grows to 1.6× |
| A4 | 450 | 2450 | 2000 | Eased spin, ending upright |
| A5 morph | 2450 | 2700 | 250 | Icon morphs into a droplet |
| A5 hover | 2700 | 2800 | 100 | Droplet hovers |
| A6 | 2800 | 3150 | 350 | Droplet accelerates downward |
| A7 | 3150 | 3600 | 450 | Splash, droplets and ripple inside the button |
| A8 | 3600 | 4400 | 800 | Liquid rises from bottom, undulates and ends flat |
| A9 draw | 4400 | 4750 | 350 | Black check draws on; Access granted status |
| A9 hold | 4750 | 5250 | 500 | Completed check holds |
| B1 | 5250 | 5950 | 700 | Every card element and the glass card spiral inward |
| B2 | 5950 | 6100 | 150 | Tiny singularity and glowing white/purple rim |
| B3 | 6100 | 6250 | 150 | White-hot center flash |
| B4 | 6250 | 8050 | 1800 | Irregular refractive explosion propagates and decays |
| B5 | 8050 | 8400 | 350 | Empty, calm, undistorted checkerboard only |
| B6 | 8400 | 8850 | 450 | Loading fades and scales 0.9× to 1× with grouped stagger |
| B7 | 8850 | onward | — | Settled loading view |

The supplied absolute timestamps allocated only 550 ms to A9 despite specifying
a 350 ms draw and 500 ms hold. Using both individual durations shifts Part B
forward by 300 ms and reaches the settled loading state at 8,850 ms. The original
approximately 9.3-second caption and the 7-second GIF are not additional timing
constraints. `settledReviewMs` is only a capture-tail allowance after B7; it does
not delay authentication or loading.

Loading groups are header, image, tasks, progress bar and footer text, in that
order. Their stagger and local reveal duration are named constants in the same
table. The window layout, fonts, theme colors and final geometry stay unchanged;
the reveal uses a parent transform and opacity rather than relaying out widgets.

## Authentication and input

The animation starts on a valid nonempty submission. The existing authentication
worker still determines whether access is granted. A fast response cannot shorten
the 2-second spin. If verification has not returned when the spin completes, the
upright icon waits there; no droplet, check, transition or loading is shown. An
accepted late response resumes the remaining sequence without skipping its
durations. A rejection resets to the editable login form for retry.

Input remains blocked until B6 completes. The key field, paste, cached-key action,
submit and card dragging cannot change the request during this interval. The
loading timeline and animated banner must not start before B6. The calm beat must
not contain a fading login card, floating information, target dots or an early
loading view.

The Unity touch-count hook uses the same atomic modal gate. While blocked, it
returns zero without calling the original touch-count function; when the reveal
finishes, polling resumes normally. This prevents touches during the transition
from reaching the underlying Unity input route as well as the disabled ImGui
controls. The standalone regression verifies suppression, no original-function
polling while blocked, normal resumption and a safe null original pointer.

Reduced motion waits for genuine verification, shows a fully drawn A9 check for
one rendered frame, then switches straight to the settled loading screen. The
`Presented` latch records that the check actually reached a frame; elapsed time
alone cannot skip it when rendering is delayed. Spin, droplet, splash, wave,
black-hole transition and loading-scale animation are bypassed.

## Native capture and checks

Run the deterministic timing, interaction and layout regressions with:

```sh
sh tests/run-startup-tests.sh
```

These checks cover every phase boundary, the 2-second eased spin ending upright,
check/status ordering, failure and retry, late verification, duplicate results,
input blocking, a complete 350 ms calm beat, no loading before B6, reveal group
ordering and the reduced-motion presentation latch. Sampling at 30, 60 and
120 FPS must detect every transition within one frame of its named boundary.
Existing first-frame/resized centering, keyboard restoration, utility actions,
manual dragging, tip playback, media crop and capture-budget tests remain active.

The offscreen GLES validation can be run on a host with EGL/GLES development
headers and a surfaceless Mesa driver:

```sh
EGL_INCLUDE_DIR=/path/to/egl-and-gles-headers \
  sh tests/run-auth-storyboard-gpu-tests.sh /tmp/auth-storyboard-captures
```

For each native pass, capture both sides of every boundary and the representative
storyboard stages. Compare the reference GIF by stage rather than by its original
wall clock. For the E1–E5 crops, examine the same explosion frame at full size:

| Crop | Evidence required |
| --- | --- |
| E1 | Front radius varies with angle; no frame presents a clean circular rim |
| E2 | Checkerboard edges bend ahead of and behind the front |
| E3 | Interior checkerboard remains recognizable through swirling refraction and a faint warm tint |
| E4 | RGB separation is visible near the front, with cool leading and warm trailing edges |
| E5 | Distortion and energy disappear completely; B5 pixels match the undistorted baseline |

Readback and frame export belong to the test harness, not the Android frame loop.
Rendered capture comparisons must include the glass material and artwork as well
as ordinary ImGui vertices. Testing a few drawn rings or timing values alone does
not verify the requested explosion.

## Verification limits

A host GLES run establishes native shader compilation, graphics-state handling,
deterministic timing and the captured visual result. It does not establish 60 FPS
on an Android device, driver-specific appearance, power use, thermal behavior or
live network authentication. The touch-count hook's actual in-game behavior also
requires device validation. Those checks require the rebuilt library on a device.
The reference GIF's 280 ms frame holds also prevent a frame-exact claim about the
source's continuous easing between frames. Record the native capture rate and
stage alignment whenever reporting a comparison.

## Recorded verification

The delivery run used native Mesa GLES 3.2 at 960×540, sampling the corrected
timeline at 60 Hz and exporting the complete 9600 ms review interval. Phase
boundaries also pass pure timeline checks at 30, 60 and 120 Hz within one frame.
The Android ARM64 API 21 build passes with NDK r22.

The GLES tests verify irregular front radii at several propagation stages,
visible checker-edge displacement, transmitted interior contrast and independent
RGB scene sample positions. All 21 consecutive B5 frames match the undistorted
checkerboard exactly. The loading timeline and banner texture remain inactive
until 8400 ms; the zero-opacity first reveal frame also matches the calm baseline.
The reveal settles at 8850 ms, with separate header, image, tasks, bar and footer
opacity ramps and a shared transform. Native alpha bounds confirm the scale.

The native reduced-motion run presents a completed check before settled loading
on the next frame. Fluid exterior-pixel checks cover the morph, fall, splash and
wave, while the rotating glyph remains independent of the fluid clip. Graphics
state restoration, exact zero-opacity tip boundaries and source checksums pass.
The pure touch-gate test verifies that blocked input never polls the original
Unity getter and forwarding resumes afterward. Physical device FPS, live
authentication and actual Unity hook behavior remain unverified.
