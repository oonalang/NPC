# Liquid startup UI

The login and loading views share a live glass material, an official Waguri
portrait inside a glass rim, a simple Inter SHAI ETHNIR title, a small EDITION
badge, a clean glass rim with muted corner marks, and pill controls. Login adds
an Access key label, sentence-case actions and an inset status footer. The
primary action remains visually distinct, with its purple rim. Authentication
still submits a captured key through the existing worker, consumes its published
result, saves the verified key, and then starts the loading animation. Paste and
cached-key actions use their existing handlers. The primary action is disabled
while a request is pending. The key field moves above the on-screen keyboard and
the card returns to its prior position after the keyboard closes.

The loading view follows the supplied window layout: a clear lavender glass
shell with a silver rim, muted corner marks, a centered header and divider,
wide animated banner, compact square stages, a lavender progress capsule and
an information pill. Both startup views use the same restrained window frame.
The banner uses a separate official nighttime Waguri GIF, with actual blinking,
hair and expression changes. The static brand portrait is a distinct character icon.
Three requested tips replace the system labels, switching every 2.5 seconds with
a short fade. Tip timing resets with each loading sequence. The four compact
loading stages, percentage and progress fill share a single progress value.
Completion uses monotonic elapsed time rather than a
frame counter, so the configured seven-second sequence does not stretch when
frames are dropped. The standalone timeline keeps the original minimum duration.

The portrait is a 160 by 160 JPEG (11,279 bytes). The 2.52-second GIF is resampled
offline into 24 frames in a 1296 by 624 JPEG atlas (157,953 bytes). Two-pixel
gutters prevent filtering from bleeding into neighboring frames. Their RGBA
textures use about 3.18 MiB together. Each texture is decoded and uploaded once
per current EGL context; playback only selects a frame's texture coordinates.
There is no runtime GIF/video decoder, network fetch or per-frame texture upload.
Image upload preserves the caller's texture, unpack-buffer and pixel-store state.
`StartupArtData.h` declares the optimized assets; generated hex parts in
`StartupMediaData` keep individual source files small without adding runtime
decoding or allocations. Sources and attribution are in
[Startup media](startup-media.md). To replace them, install Pillow and run:

```sh
python3 tools/embed-startup-art.py /path/to/portrait.png /path/to/animation.gif
```

## Rendering and drag cost

The renderer copies the game scene before overlay drawing into a shared RGBA
texture with a maximum dimension of 384 pixels. One five-tap blur pass produces a
second texture. Together they allocate at most 1.125 MiB. Resources and shader
uniform locations are cached per current EGL context and resized only when
needed. There is no CPU pixel readback or full-resolution blur buffer.

The custom GLES material follows Apple's documented clear-glass properties:
[lensing and reactive highlights](https://developer.apple.com/videos/play/wwdc2025/219/),
with [transmission over rich backgrounds](https://developer.apple.com/design/human-interface-guidelines/materials).
It samples both the sharp scene and its blur, preserving center detail while
softening the curved edges. SDF lens normals displace edge samples, concentrate
light into a narrow highlight and caustic, and apply restrained color separation.
The highlight direction follows pointer position. The loading panel uses a 27%
dark tint and a light veil, replacing the prior 74% charcoal tint. This is a
custom GLES approximation, not Apple's platform material.
Startup text uses a fine dark contour shadow for bright-scene contrast; the
input, action buttons and footer use denser local glass tints. The outer shell
retains clear scene transmission. White-background previews cover this case.
Shaders run through bounded
ImGui draw callbacks and restore the normal renderer afterwards. Sampler state is
restored locally on both texture units; capture also preserves framebuffers, program, VAO, viewport,
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
Startup checks also cover rotating-tip timing, opacity, GIF loop timing and atlas crop bounds.

The optional Mesa GLES test in `tests/clay_capture_test.cpp` verifies copied and
blurred pixels, state restoration, capture cadence, shader-rendered output,
rounded clipping and sampler restoration. It also verifies the artwork's decoded
pixels, separate cached portrait/atlas uploads, differing animation frames, and
unpack-state preservation. A patterned background verifies edge displacement and
center transmission; both texture units' sampler/texture bindings are checked.
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
