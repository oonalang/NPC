# Native ESP character upgrade

## Baseline and ownership

The starting main commit is `51e03ed`. The attached prompt and the original reference GIF were read before editing. The GIF is 320 × 512, 240 frames at 50 ms each, lasting 12 seconds. It demonstrates the existing purple/gold Wiz design, a wave around 3–6 seconds and the hop near 8 seconds. Its green background is reference-only.

- `Main.cpp`: existing ESP page layout and config pointers; calls the preview panel only on the ESP page.
- `runtime_preview_menu.h`: clay/glass preview panel and existing option-controlled lines, boxes, health, labels and skeleton.
- `AnimePreview.h`: ImGui image presentation and conversion of projected native anchors to overlay coordinates; PNG compatibility fallback.
- `WizModel.cpp/.h`: asynchronous asset/PNG decode, animation sampling, GPU skinning, 320 × 512 transparent target, 30-Hz render cache, projection and isolated EGL/GL resources.
- `tools/export-wiz-model.py`: imports the artist FBX and painted textures and generates the embedded compressed asset.
- `Assets/WizModel.S` and `Android.mk`: embed the model and rebuild its assembly object when the binary changes.

`WIZSKN01` stores 98 finished skinning matrices per sample, not parent-local poses. Its matrices alone cannot safely support joint-local dynamics. The source FBX is available in this workspace. A fresh Blender audit confirmed existing side-lock (`HairF.*`), back-hair (`Hairb.*`, `HairM*`) and top-strand (`HairA*`) chains, actual Blink/Smile/Speak shapes, and no dedicated skirt/sleeve bones. Hair skinning must use these actual names and hierarchy. Garment geometry is identifiable as two connected coat components spanning the upper torso and lower hem; hands, boots, face and hair belong to other components.

## Implementation and validation

Implementation details, exact asset layout, executed validation and remaining limits are recorded here as the upgrade is completed. Original Glayer3d attribution and CC BY-NC 4.0 remain in `Assets/WIZ_LICENSE.md`. This feature branch does not change gameplay hooks, authentication, licensing, ESP configuration or main.

### Rig and dynamics

The mesh remains 17,530 exported vertices / 27,024 triangles. Fourteen export-time bones extend the original 98-bone rig: two front bangs, two sleeve chains (two bones each) and four skirt-panel chains (two bones each). Topology checks identify the two original coat components before reweighting 939 garment vertices; 282 bang vertices are identified separately. Hands, boots, face, UVs, topology and all three painted PNGs remain unchanged. Four normalized GPU influences are retained per vertex; the largest discarded influence sum is 0.08394 in the extended garment weights. The metadata reports this rather than implying lossless weight storage.

There are 32 weighted spring joints in six independent parameter groups:

| Group | Joints | Acceleration stiffness | Damping / s | Gravity scale | Tip radius / m | Cone / radians |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Bangs | 2 | 220 | 18 | .03 | .007 | .08 |
| Side locks | 6 | 110 | 10 | .09 | .009 | .18 |
| Back hair | 9 | 75 | 8 | .14 | .012 | .20 |
| Top strand | 3 | 145 | 6 | .04 | .006 | .32 |
| Skirt | 8 | 180 | 13 | .09 | .012 | .14 |
| Sleeves | 4 | 160 | 12 | .06 | .009 | .10 |

The audited model uses meters and Z up. Damped semi-implicit integration tracks world-space tips under gravity and moving animated parents. Length and angular-cone constraints, plus body-capsule separation, are solved before the resulting swing is converted back through the actual parent rotation to a local quaternion. Children inherit the solved parent transform. Only then are global poses and inverse binds multiplied into GPU skinning matrices. Ten capsules follow the head, neck, shoulders, torso, hips, thighs and forearms. Finite collision iterations and a bounded cone search handle difficult contacts without an unbounded solver.

`Simulation::stepSeconds`, `maxCatchUp` and `warmSteps` define the scheduling: 1/60 second, at most six catch-up steps, or an eleven-step bounded warm start after a seek, long stall, tab reopen or resource recreation. Repeated timestamps never integrate twice. A renderer cache hit still allows the fixed-step simulation to advance; it does not rebuild the displayed frame. Render requests, including changing camera angles/aspect ratios, coalesce into 1/30-second buckets. Repeated explicit resets are outside that normal cadence. The existing offscreen size and GPU material cache remain unchanged. `SetDynamics(Full/Hair/Off)` offers lower-cost dynamics modes without adding saved game settings.

### Pose, face and preview controls

All new-format rotations interpolate with normalized shortest-path quaternion slerp. A two-bone leg solve removes foot drift after interpolation while retaining the foot orientation and authored hop lift. The major wave, anticipation, hop and landing keep the reference's 12-second sequence. Separate 4.17-second breathing, smoothly seeded head/gaze/finger variation, delayed eye motion, irregular blink intervals with occasional doubles, and expression blending do not restart with that clip. Existing Blink/Smile/Speak geometry is retained; no new facial shape is fabricated.

The initial front camera retains the previous position, target and field of view. **Orbit** in the preview heading enables horizontal dragging, limited to ±90 degrees. **Front** disables orbit and resets the view. Vertical gestures cancel orbit intent and remain available to content scrolling; no existing slider, toggle or config is reassigned. The renderer also bounds its programmatic pitch input to ±.18 radians. Camera movement does not apply fake forces to the character.

The GPU palette, 15 landmark bindings and exact current skinned-vertex bounds come from the same final pose. Existing config-controlled overlay drawing stays in `runtime_preview_menu.h`. A loading state does not draw a temporary 2D character. Native failure explicitly selects/lists the old PNG fallback; `Bounds::native3d` remains false for it. GPU resources are rebuilt after an EGL context change or loss of owned GL objects. Partial initialization frees its allocated GPU objects before falling back.

### WIZSKN02 layout and migration

All integers/floats are little endian. The outer envelope is eight bytes `WIZSKN02`, a `uint32` uncompressed size and a zlib payload. The payload is:

1. Header: seven `uint32` values (version=2, vertices, indices, bones, frames, sample FPS, materials), then float duration.
2. Existing 80-byte vertices: position/normal/UV, four byte joint IDs, four normalized `uint16` weights, and Blink/Smile/Speak position deltas. Then `uint32` triangle indices.
3. Each material: first index, index count, PNG byte length (three `uint32`), then the unchanged PNG.
4. Rig counts: bones, spring joints, colliders, anchors (four `uint32`).
5. Each bone: signed parent index; name byte count; local rest position (3 floats), quaternion XYZW (4), scale (3); column-major inverse bind (16); local tail (3); conservative local low/high bounds (6); bounded flag (`uint32`); printable name bytes. Parents precede children. The fixed portion is 152 bytes.
6. Each spring: bone/group/collision-mask (`3 × uint32`), stiffness/damping/gravity/radius/cone (`5 × float`). Length derives from the real local tail. Each collider: bone/mask (`2 × uint32`), local capsule A/B/radius (`7 × float`).
7. Fifteen anchors: two bone IDs, two local points and blend weight (`2 × uint32 + 7 × float`). Eight semantic bone IDs identify the head, face, eyes, chest/spine and fingers.
8. 289 samples at 24 Hz: 10 floats per bone (local position, XYZW quaternion, scale), followed by Blink/Smile/Speak, hop lift and excited weight. The stride is 1,125 floats.

Validation bounds counts, raw/compressed lengths, indices, weights, finite values, unit quaternions/scales, names, hierarchy order, spring/collider parameters and anchors. Reconstructed rest transforms must agree with inverse binds. PNG CRCs and decoding remain checked. `WIZSKN01` is still accepted and follows its original matrix-sampling path, with no invented hierarchy or spring physics. A true physics migration requires the original FBX/textures; re-export with the command in `WIZ_LICENSE.md`. A clean Blender 4.3.2 re-export reproduced the exact new binary SHA-256:

```text
64b253a83c8ead867092396a184b619076ca657bfa54512360536ed2cb32f62f
```

### Validation and resource scope

Executed validation commands and results are recorded below. GPU captures use a surfaceless Mesa GLES3 context, not an Android phone. ASan/UBSan scripts disable leak detection for the execution environment, as the existing scripts already did.

| Check | Result |
| --- | --- |
| `sh tests/run-anime-preview-tests.sh` | Fallback geometry/PNG checks; actual ImGui touch Orbit/Front taps, horizontal drag and vertical scroll coexistence. |
| `sh tests/run-wiz-model-tests.sh /workspace/wiz-upgrade-reference/wiz-model-v1.bin` | Codec/parser rejection, v1 fixture and original shipped v1 asset, slerp, all six real weighted groups, stability, joint lengths/cones, capsule separation, deterministic 30/60/120-Hz inputs, settling, quality modes, seeds and bounded pause/seek behavior. The optional argument adds the original asset check; it does not replace the base suite. |
| `MESA_SHADER_CACHE_DIR=/tmp/npc-mesa-cache sh tests/run-wiz-model-gpu-tests.sh /workspace/wiz-upgrade-frames` | Full real GLES loop, representative motion captures, front/quarter/side views, landmark bounds, camera cache budget, stable storage/upload counts, separate read/draw FBO and shader/VAO/UBO/texture/sampler/pixel-store/state restoration, owned-resource loss, EGL recreation and native-failure PNG fallback. |
| NDK r22 `ndk-build`, existing Android.mk/Application.mk, ARM64, API 21 | Native `libnets.so` build; an APK build is not claimed. |
| Artist material comparison | All three embedded PNGs match the previous asset byte for byte. |

The original v1 check file was extracted with `git show 51e03ed:src/main/jni/Assets/wiz-model.bin`. The native build command was:

```sh
/workspace/toolchains/android-ndk-r22/ndk-build NDK_PROJECT_PATH=. \
  APP_BUILD_SCRIPT=src/main/jni/Android.mk \
  NDK_APPLICATION_MK=src/main/jni/Application.mk \
  NDK_OUT=/workspace/liquid-build/obj NDK_LIBS_OUT=/workspace/liquid-build/libs -j2
```

Review media was generated outside the repository: `/workspace/wiz-upgrade-preview.mp4`, `/workspace/wiz-upgrade-comparison.mp4` and `/workspace/wiz-upgrade-reference/final-contact.png`. The comparison places the unchanged original GIF on the left and captured native output on the right; both run for 12 seconds. It is a software GLES validation artifact, not an Android screen recording.

Across the real-rig host test, maximum planted-foot position error was approximately 0.0000006 m, and the accumulated spring-tip capsule penetration metric stayed below 0.0001 m. Fixed-step outputs matched for 30/60/120-Hz input schedules. Settled aggregate tip speed after the hop was .724 versus a 27.584 peak during the landing window; this is a comparison of the same host simulation, not a device performance measurement.

The model's packed bytes fall from 2,541,277 to 2,165,320. Reported host storage estimates after upload are 3,958,970 CPU bytes plus the 2,165,320-byte embedded asset and 12,155,104 bytes of requested GLES texture/buffer storage. Simulation scratch capacity is 19,904 bytes. These stayed constant through the captured loop and camera stress test. They exclude allocator/driver overhead, other menu/game resources and Android process memory. Native CPU/GPU time, thermal behavior and phone FPS have not been measured.

Representative original/new frames were inspected for clipping, eye attachment, foot placement, costume fidelity, wave/hop framing and the new side views. Original GIF timing is preserved for the main gestures; new secondary motion and facial timing deliberately differ. The coat now follows its garment rig rather than the old leg twist, so its front panel/hem is straighter. Source face, proportions, topology, purple/gold materials and painted details remain intact. The GIF provides no authoritative side/back animation; side-view validation uses the genuine source mesh.

### Exact changed files

- `src/main/jni/ImGui/WizModel.cpp`, `WizModel.h`: v1/v2 decoder, rigged rendering, physics/cache/camera integration, diagnostics and resource lifecycle.
- `src/main/jni/ImGui/WizMotion.cpp`, `WizMotion.h`: quaternion/local-pose math, foot stabilization, spring solver, collision constraints and deterministic scheduling.
- `src/main/jni/ImGui/PreviewOrbit.h`, `PreviewOrbitUi.h`: isolated transient camera input/control and shared ImGui gesture handling.
- `src/main/jni/ImGui/AnimePreview.h`, `runtime_preview_menu.h`: correct 3D/pending/failure presentation, shared orbit control and current-pose overlays.
- `tools/export-wiz-model.py`: audited source rig/weight extension and explicit WIZSKN02 export.
- `src/main/jni/Assets/wiz-model.bin`, `wiz-model.json`, `WIZ_LICENSE.md`: new embedded rig, metadata and preserved/updated attribution.
- `tests/anime_preview_test.cpp`, `wiz_model_test.cpp`, `wiz_model_gpu_test.cpp`: touch, parser, physics, legacy/fallback, state/resource and rendered-frame validation.
- `tests/run-wiz-model-tests.sh`, `run-wiz-model-gpu-tests.sh`: compile the real runtime modules and exercise good/failing native assets.
- `docs/ESP_3D_UPGRADE.md`, `NEXT_STEPS.md`: review notes and remaining validation.

`Main.cpp`, gameplay hooks, auth/licensing, music, ESP option/config fields and unrelated UI styling are unchanged. Android.mk's existing source wildcard already includes the new motion module; its existing embedded-asset dependency rebuilds the model assembly correctly.

### Remaining limits and phone review

- Cloth is weighted spring-bone cloth, not a triangle cloth/self-collision solver. Capsule constraints protect tips; they do not prove every hair card or garment triangle is penetration-free at every possible pose. Inspected frames did not show severe body clipping.
- The authored wave/hop clip still repeats every 12 seconds. Independent idle, face and physics state prevent a perfectly repeated whole-character loop; new gestures were not invented beyond the reference sequence.
- The exporter preserves four GPU influences, including the reported maximum weight truncation. It does not change mesh topology or add facial morph geometry.
- Normal render cadence is 30 Hz, physics is 60 Hz. This is not a claim of 60 rendered preview frames per second.
- On a real Android phone, check Orbit/Front taps and vertical scrolling beside ESP controls, all overlay combinations, face/cloth motion at the actual panel size, app background/resume, tab reopen, shader/texture allocation failure, device frame time, thermal load and memory. No phone, Android touch backend, device GPU profiler or APK build was available for this pass.
- Commit/publish only the feature branch for review; main is unchanged by this task.
