# Liquid startup UI

The login and loading views share a live glass material, an official Waguri
portrait inside a glass rim, a simple Inter SHAI ETHNIR title, a small EDITION
badge, a clean glass rim with muted corner marks, and pill controls. Login adds
an Access key label, sentence-case actions and an inset status footer. The
primary action remains visually distinct, with its purple rim. Authentication
still submits a captured key through the existing worker, consumes its published
result and saves the verified key. The authenticate action follows the corrected
[storyboard timing and verification contract](authenticate-storyboard.md): label
fade, centered icon growth, a two-second eased turn ending upright, droplet morph
and hover, accelerating fall, clipped splash, bottom-up liquid wave and a drawn
black check. The complete card then spirals inward before a singularity, flash
and screen-space refractive shockwave. A real 350 ms checkerboard-only beat
precedes the staggered loading reveal. The loading timeline and animated banner
start at B6, never earlier. Failure restores the editable form; success artwork
requires genuine verification. A late result holds the upright icon at the end
of the spin. Controls and Unity touch-count input remain blocked through the
completed reveal. Android's disabled-animator preference selects the reduced
path: one presented success-check frame followed by settled loading. Timings use
monotonic elapsed time without blocking the render thread.

The login card centers on its first frame, when reopened, and whenever the
viewport changes size or position. This handles the game's initial surface
resolution changes. Manual dragging is retained at a stable viewport size.
The key field moves above the on-screen keyboard and the card returns to its
resting position after the keyboard closes; a resize refreshes that resting
position to the new viewport center.

The loading view follows the supplied window layout: a clear lavender glass
shell with a silver rim, muted corner marks, a centered header,
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

The custom GLES material adapts the displacement model from
[deepika-builds/liquid-glass](https://github.com/deepika-builds/liquid-glass/blob/98ed97bd99def529493fd37177228810f6422f6d/liquid-glass.js).
Local X/Y gradients encode displacement. A softened inset rounded-rectangle mask
neutralizes 93% of the interior ramp. Negative per-channel scales (-112, -106,
-100) magnify the scene inward at the rim and add a prism fringe, with screen Y
converted to GLES texture Y. Background saturation is 1.5, and thin surfaces use
an approximately 18% to 32% dark tint gradient.

The mask uses a smooth SDF transition as an analytic approximation of the
reference's Canvas blur. Small controls attenuate displacement and curvature to
fit their size. Existing sharp and blurred scene textures provide sampling;
there is no DOM, SVG filter, CPU displacement map or additional render target.
Pointer-reactive highlights and the light veil remain native renderer features.
This is a GLES adaptation of the web reference, not a pixel-identical Canvas
filter or Apple's platform material. The source's MIT notice is retained in
[third_party/liquid-glass/LICENSE](../third_party/liquid-glass/LICENSE).
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
The menu shell, header, sidebar and content host use the same refractive material,
with denser tints on structural regions. Login and loading footers share it too.
All glass surfaces reuse the same captured scene textures; clay cards and
dropdowns retain their raised material.
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
Four rim probes check inward X/Y displacement, channel deltas check the prism
fringe, and an off-center interior probe checks neutralization away from the
trivial zero-gradient midpoint.
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
