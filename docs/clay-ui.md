# Clay UI

The runtime menu uses dark lavender clay panels, Inter Semibold title text,
rounded inset dropdowns, and checkmarks for selected dropdown options.

Toggle thumbs expand while held and during their transition, then return to a
resting pearl shape. Slider thumbs grow into pills while active. Slider positions
follow ImGui's native value directly; only the material and size animate. The
critically damped spring preserves velocity when its target changes.

The glass material samples the live game framebuffer before drawing the overlay.
One shared RGBA texture has a maximum dimension of 384 pixels (at most 576 KiB).
Capture runs while the visible menu receives a press or a glass transition is
still settling. It uses linear downsampling and a small magnification for
refraction, without additional blur passes. Framebuffer, texture, pixel unpack
buffer, and scissor state are preserved. Multisampled sources and unavailable
contexts use the opaque pearl fallback; control size feedback remains active.

Run the motion regression with:

```sh
c++ -std=c++17 tests/clay_motion_test.cpp -o /tmp/clay-motion-test
/tmp/clay-motion-test
```

The ARM64 Android build and CPU-rendered native widget preview validate compile
compatibility and control geometry. Live framebuffer sampling, touch feel, and
GPU cost still require an in-game device check. No FPS improvement is claimed.
