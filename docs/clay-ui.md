# Clay UI

The runtime menu uses dark lavender clay panels, Inter Semibold title text,
rounded inset dropdowns, and checkmarks for selected dropdown options. The
sidebar's active page uses an inset clay highlight without a dot. A single-line
Inter wordmark and a compact edition capsule sit on the glass header; blueprint
tiers retain their selector without the explanatory descriptions underneath.

Closed dropdowns use taller fields with the same dark fill and subtle border as
their popovers, with vertically centered labels and chevrons.

Open dropdowns use a dark rounded popover, a short muted heading, roomy rows,
and right-aligned white checkmarks. Only hovered rows receive a subtle fill.
Blueprint rows use red Mythic, gold Legendary, purple Epic, and green Common
circles; All blueprints uses an outlined neutral circle. These are explicit row
decorations and do not change tier matching or multi-selection. Popovers keep
their touch targets stationary, flip above the field when needed, and scroll
when the list cannot fit within the viewport.

Run `sh tests/run-dropdown-tests.sh` for native input checks covering tier
multi-selection, clearing, single selection, disabled rows, long labels,
outside-click dismissal, upward placement, and scrolling to the last option.

Toggle thumbs expand while held and during their transition, then return to a
resting pearl shape. Slider thumbs grow into pills while active. Slider positions
follow ImGui's native value directly; only the material and size animate. The
critically damped spring preserves velocity when its target changes.

Slider fill reaches both track endpoints, and the thumb stays inside the track
in both its resting and expanded shapes. Content panes and dropdown lists scroll
by touch without visible scrollbars. A vertical gesture cancels control presses;
a horizontal slider gesture adjusts its value, and a stationary tap commits on
release. Content gestures keep the menu in place while its header remains draggable.
Login starts centered when it appears, including after being reopened.

Run `sh tests/run-touch-slider-tests.sh` for queued touch input checks covering
scrolling over controls, cancellation, final release samples, slider endpoints,
dropdowns and child panel geometry. Run `sh tests/run-startup-tests.sh` for login
centering and existing input/timing checks.

The glass material combines sharp scene transmission with a five-tap blur,
rounded lens refraction and pointer-reactive directional highlights. Captures are capped at 30 Hz while
stationary and 20 Hz while dragging; the UI and lens positions track every frame.
Login and loading use the same material. The two bounded RGBA textures together
use at most 1.125 MiB. A separate official Waguri portrait and GIF frame atlas
use about 3.18 MiB across two textures, uploaded once per EGL context. Nested menu panels
tint the parent glass, and opaque clay
cards keep text readable. See [Liquid startup UI](liquid-startup.md) for pipeline,
state preservation, fallback and validation details.

Run the motion regression with:

```sh
c++ -std=c++17 tests/clay_motion_test.cpp -o /tmp/clay-motion-test
/tmp/clay-motion-test
```

The GLES capture/material integration check and its build command are documented
in [Liquid startup UI](liquid-startup.md). Device FPS and Android GPU cost still
require an in-game check; the bounded capture budget is verified separately.
