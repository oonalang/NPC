# Liquid startup UI

The login and loading views share a live glass material, a Waguri-inspired anime
portrait inside a glass rim, a simple Inter SHAI ETHNIR title, a small EDITION
badge, flowing edge ribbons, and pill controls. Authentication
still submits a captured key through the existing worker, consumes its published
result, saves the verified key, and then starts the loading animation. Paste and
cached-key actions use their existing handlers. The primary action is disabled
while a request is pending. The key field moves above the on-screen keyboard and
the card returns to its prior position after the keyboard closes.

The loading view follows the supplied window layout: a softly frosted charcoal
shell with a silver rim, muted corner marks, a centered header and divider,
wide illustrated banner, compact square stages, a lavender progress capsule and
an information pill. Its clean rim replaces the login view's flowing ribbons.
The banner has gentle camera drift and six floating
petals. It uses an original generated illustration and animated texture coordinates.
Three requested tips replace the system labels, switching every 2.5 seconds with
a short fade. Tip timing resets with each loading sequence. The four compact
loading stages, percentage and progress fill share a single progress value.
Completion uses monotonic elapsed time rather than a
frame counter, so the configured seven-second sequence does not stretch when
frames are dropped. The standalone timeline keeps the original minimum duration.

The portrait and banner share one embedded 768 by 256 JPEG (44,691 bytes),
decoded and uploaded once per current EGL context. Its RGBA texture uses 0.75 MiB.
There is no video decoder, network fetch, per-frame decoding or texture upload.
Image upload preserves the caller's texture, unpack-buffer and pixel-store state.
`StartupArtData.h` contains the optimized artwork. To replace it from an original
3:1 illustration, install Pillow and run:

```sh
python3 tools/embed-startup-art.py /path/to/illustration.png
```

## Rendering and drag cost

The renderer copies the game scene before overlay drawing into a shared RGBA
texture with a maximum dimension of 384 pixels. One five-tap blur pass produces a
second texture. Together they allocate at most 1.125 MiB. Resources and shader
uniform locations are cached per current EGL context and resized only when
needed. There is no CPU pixel readback or full-resolution blur buffer.

Rounded material shaders sample the blurred live scene with edge refraction,
slight color separation and directional highlights. They run through bounded
ImGui draw callbacks and restore the normal renderer afterwards. Sampler state is
restored locally; capture also preserves framebuffers, program, VAO, viewport,
active texture, texture bindings, unpack buffer, color mask and raster states.
Unsupported or multisampled capture sources use the opaque material fallback.

Capture is capped at 30 Hz while stationary and 20 Hz while a window is moving.
The lens and UI continue to render at the overlay's frame rate, so dragging uses
the current pointer position and current screen-space texture coordinates. A
simulated 60 FPS drag verifies 20 copies in one second instead of one per frame.
Nested menu panels tint the parent material instead of resampling the scene.
Closing or collapsing the menu stops capture; login and loading can use glass
before authentication. Delta time follows the actual frame interval.

## Validation

Run the native input/timing checks with `sh tests/run-startup-tests.sh` and the
existing dropdown checks with `sh tests/run-dropdown-tests.sh`.
Startup checks also cover rotating-tip timing, opacity and banner crop bounds.

The optional Mesa GLES test in `tests/clay_capture_test.cpp` verifies copied and
blurred pixels, state restoration, capture cadence, shader-rendered output,
rounded clipping and sampler restoration. It also verifies the artwork's decoded
pixels, a single upload across repeated calls, and unpack-state preservation.
With EGL/GLES3 headers in
`CLAY_GL_INCLUDE`, build it on Linux with:

```sh
c++ -std=c++17 -O0 -DWL_EGL_PLATFORM -DIMGUI_IMPL_OPENGL_ES3 \
  -I"$CLAY_GL_INCLUDE" -Isrc/main/jni tests/clay_capture_test.cpp \
  src/main/jni/ImGui/imgui.cpp src/main/jni/ImGui/imgui_draw.cpp \
  src/main/jni/ImGui/imgui_widgets.cpp src/main/jni/ImGui/imgui_tables.cpp \
  src/main/jni/ImGui/imgui_impl_opengl3.cpp src/main/jni/ImGui/stb_image.cpp \
  -Wl,-l:libEGL.so.1,-l:libGL.so.1 -o /tmp/clay-capture-test
MESA_SHADER_CACHE_DISABLE=true /tmp/clay-capture-test
```

Previews use the actual ImGui GLES renderer over a synthetic background. They do
not establish device FPS or game compatibility. Android GPU timing, touch feel,
authentication against the hosted service and appearance over the actual game
still require device validation.
