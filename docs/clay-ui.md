# Clay UI

The runtime menu uses dark lavender clay panels, Inter Semibold title text,
rounded inset dropdowns, and checkmarks for selected dropdown options. The
sidebar's active page uses an inset clay highlight without a dot. A single-line
Inter wordmark and a compact edition capsule sit on the glass header; blueprint
tiers retain their selector without the explanatory descriptions underneath.

Toggle thumbs expand while held and during their transition, then return to a
resting pearl shape. Slider thumbs grow into pills while active. Slider positions
follow ImGui's native value directly; only the material and size animate. The
critically damped spring preserves velocity when its target changes.

The glass material samples the live game framebuffer before drawing the overlay.
One shared RGBA texture has a maximum dimension of 384 pixels (at most 576 KiB).
The expanded authenticated menu refreshes that texture each frame, before any
overlay drawing. Closing or collapsing the menu stops capture. The shell and
header use restrained dark glass with mild refraction; the clay cards remain
opaque for readability. Linear downsampling and a small magnification provide
the material without additional blur passes. Framebuffer, texture, pixel unpack
buffer, and scissor state are preserved. Multisampled sources and unavailable
contexts use an opaque fallback; control size feedback remains active. The
continuous visible-menu capture trades more GPU bandwidth for a live background.

Run the motion regression with:

```sh
c++ -std=c++17 tests/clay_motion_test.cpp -o /tmp/clay-motion-test
/tmp/clay-motion-test
```

The optional `tests/clay_capture_test.cpp` uses a surfaceless Mesa EGL/GLES3
context. It checks captured pixels, hidden-menu skipping, and preservation of
separate read/draw framebuffers, texture, unpack buffer, and scissor state.
With EGL/GLES3 headers available under `CLAY_GL_INCLUDE`, run it on Linux with:

```sh
c++ -std=c++17 -O0 -DWL_EGL_PLATFORM -I"$CLAY_GL_INCLUDE" -Isrc/main/jni \
  tests/clay_capture_test.cpp src/main/jni/ImGui/imgui.cpp \
  src/main/jni/ImGui/imgui_draw.cpp src/main/jni/ImGui/imgui_widgets.cpp \
  src/main/jni/ImGui/imgui_tables.cpp \
  -Wl,-l:libEGL.so.1,-l:libGL.so.1 -o /tmp/clay-capture-test
MESA_SHADER_CACHE_DISABLE=true /tmp/clay-capture-test
```

The ARM64 Android build, native widget preview, and Mesa capture check pass.
The supplied capture test checks real GL behavior on Mesa; in-game appearance,
touch feel, and Android GPU cost still require a device check. No FPS improvement
is claimed.
