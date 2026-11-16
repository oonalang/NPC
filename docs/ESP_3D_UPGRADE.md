# Native ESP character physics revision

This revision integrates the existing `a01d108` runtime-physics work into the current client on `main`. Main was still shipping the old WIZSKN01 matrix-only asset. The user reports broken clothing and limbs, unrealistic physics and only a standing pose. The hierarchy-aware rig, garment weights and runtime motion are now wired into the production preview. The attached prompt and 320 × 512, 240-frame, 12-second reference GIF were inspected. The original Glayer3d mesh, face, proportions, UVs, purple/gold materials and painted textures remain the source asset. Attribution and CC BY-NC 4.0 are preserved in [WIZ_LICENSE.md](../src/main/jni/Assets/WIZ_LICENSE.md).

## What changed

The old garment assignment used abrupt height/position rules and independent skirt panels. In particular, the folded inner hem crosses the center line: assigning a panel by `atan2` alone sent neighboring vertices to opposite sides of the skirt. The exporter now solves a harmonic direction field through the actual garment edge graph for those interior folds. Arm-weight checks keep sleeve geometry out of the skirt assignment. The waist and sector weights blend continuously. All new garment influences fit four GPU weights without truncation; the previous maximum discarded source influence elsewhere remains 0.02883.

Eight skirt chains replace four. Their 16 moving joints have 48 explicit distance, shear and bending constraints, solved with XPBD compliance and a bounded ten-pass solve. Both original garment layers use the same continuous attachment field. The simulated control net maintains the dress shape during acceleration instead of allowing neighboring panels to drift independently.

Hair, sleeves and the original `breast.L` / `breast.R` bones use damped 3D springs. The integrator solves the spring response analytically in the moving target frame, including gravity, inertia and relative damping. A child hair joint targets its already simulated parent; it does not fight an unrelated baked world transform. Constraint corrections feed back into velocity. Fixed bone lengths, cone limits, speed limits and 16 animated body capsules bound movement. The chest's previous baked sine motion is removed and its source local transforms stay constant; acceleration now produces the secondary response.

The body has an asymmetric resting pose, weight transfer, breathing, coordinated head/gaze movement, irregular blinks, finger variation and occasional arm/posture adjustments. The wave folds the elbow before raising the arm, includes body counterbalance and a slower varied wrist rhythm, and stays inside the preview. The hop has anticipation, ballistic flight under 9.81 m/s², and a cushioned landing. The original gesture clip remains 12 seconds of source poses; after its initial demonstration, wave and hop events are scheduled at seeded, varied intervals independently of continuous idle and physics. This deliberately changes the original GIF's slow float and exact repeated timing.

The final palette still uses GPU skinning. Skeleton landmarks follow the final rig. ESP boxes now use bounds projected from the skinned vertices, rather than projecting a loose world-space bounding box. Orbit/Front controls, scroll behavior, overlay option wiring, UI styling, saved configuration and the failure-only PNG path retain their existing integration.

## Ownership and runtime budget

- `Main.cpp`: existing ESP page and config pointers; the unrelated retired login-policy feature is removed in this delivery.
- `runtime_preview_menu.h`: existing option-controlled overlays and panel, integrated Orbit/Front controls, loading-state handling and an explicit failure-only fallback label.
- `AnimePreview.h` and `PreviewClock.h`: native presentation, model-local playback, tab-reopen reset, landmarks and failure-only fallback. Playback starts at zero when the actual 3D model is ready, so time spent on login or other tabs cannot skip the introductory wave and hop. Duplicate draws do not advance playback; a hidden tab or long suspension restarts it safely.
- `WizModel.cpp/.h`: bounded asset decoder, asynchronous textures, GPU skinning, projected bounds, render cache and EGL/GL lifecycle.
- `WizMotion.cpp/.h`: local-pose interpolation, idle/gesture timing, feet, spring dynamics and cloth constraints.
- `tools/export-wiz-model.py`: source FBX audit, garment weights/rig, source gestures and packed asset.

The character has 17,530 exported vertices, 27,024 triangles and 120 bones: the original 98 plus 22 bang/garment bones. There are 42 spring joints and 16 body capsules. All three embedded painted PNGs are byte-identical to the previous asset.

| Group | Joints | Stiffness / mass | Damping / mass | Gravity scale | Tip radius / m | Cone / radians |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Bangs | 2 | 150 | 12 | .10 | .007 | .12 |
| Side locks | 6 | 85 | 7 | .18 | .009 | .28 |
| Back hair | 9 | 52 | 5 | .28 | .012 | .34 |
| Top strand | 3 | 110 | 4.5 | .08 | .006 | .46 |
| Skirt | 16 | 95 | 8 | .24 | .012 | .28 |
| Sleeves | 4 | 105 | 8 | .18 | .009 | .24 |
| Chest | 2 | 170 | 9 | .12 | .025 | .18 |

The model uses meters and Z up. Physics advances at 60 Hz, independently of the 30-Hz cached 320 × 512 GLES target. Repeated timestamps do not advance it twice. `Simulation::stepSeconds`, `maxCatchUp`, `warmSteps` and `clothIterations` name the scheduling/solver limits. Catch-up is bounded at six ticks; seek, long stall, tab reopen and resource recreation use an eleven-step warm start initialized near gravity equilibrium. `Full`, `Hair` and `Off` dynamics modes remain available through the existing runtime API. No work is submitted by a hidden ESP panel.

Measured host storage estimates after upload are 4,058,578 CPU bytes, plus the 1,717,345-byte embedded asset, and 12,155,104 bytes of requested GLES buffer/texture storage. Simulation scratch is 22,880 bytes. Storage/upload counts remained stable during the 48-second test and orbit stress checks. These counts exclude allocator/driver overhead, other menu/game resources and Android process memory. They are not device frame-time or thermal measurements.

## Asset format and reproduction

`WIZSKN03` extends the explicit local hierarchy introduced by `WIZSKN02`. All fields are little endian. The envelope is eight magic bytes, a uint32 raw length and zlib data. The payload is:

1. Seven uint32 header values: version=3, vertices, indices, bones, samples, sample FPS, materials; followed by float duration.
2. Existing 80-byte vertex layout: position/normal/UV, four byte joints, four normalized uint16 weights, Blink/Smile/Speak position deltas. Then uint32 triangle indices.
3. Material records: first index, count, PNG length (three uint32), followed by the unchanged PNG bytes.
4. Five uint32 rig counts: bones, springs, colliders, anchors and cloth links. Version 2 has only the first four.
5. Bone records: signed parent index, name length, rest local TRS (position3, XYZW quaternion4, scale3), inverse-bind matrix16, local tail3, low/high bounds6, bounded flag and printable name bytes. The fixed part is 152 bytes; parents precede children.
6. Springs: bone/group/mask (three uint32), stiffness/damping/gravity/radius/cone (five floats). Groups 0–5 retain their old meanings; group 6 is chest in version 3. Bone length comes from the actual local tail.
7. Colliders: bone/mask (two uint32), local capsule A/B/radius (seven floats). Fifteen anchors: bone A/B, local A/B and mix (two uint32 + seven floats). Eight existing semantic bone IDs follow.
8. Cloth links: spring-array index A/B (two uint32), rest distance and compliance (two floats). These refer to spring entries, not arbitrary bone indices. Rest lengths are validated against the decoded bind hierarchy.
9. 289 samples at 24 Hz: local position3/quaternion4/scale3 for every bone, followed by Blink/Smile/Speak, hop lift and excitement. The current stride is 1,205 floats.

The decoder validates limits, counts, topology indices, normalized weights, finite/unit transforms, hierarchy, inverse binds, bounded physical parameters, PNGs and constraint endpoints/rest distances. Duplicate or malformed cloth links are rejected. `WIZSKN01` keeps its original sampled-matrix path; `WIZSKN02` still decodes and simulates its explicit spring hierarchy without cloth links. The renderer does not import FBX or fetch assets at runtime.

Re-export using the artist files described in the attribution file:

```sh
blender -b -t 2 -P tools/export-wiz-model.py -- /path/to/WizFiles src/main/jni/Assets/wiz-model.bin
```

Blender 4.3.2 was used. A clean second export reproduced the exact binary:

```text
932117890fe0b5da96d0c693f15f0cf45f13ca736b4ca98e4600cc8e7e52a2c0
```

The new raw payload is 4,022,985 bytes; the packed asset is 1,717,345 bytes, down from 2,165,320 in the preceding feature commit. `wiz-model.json` records geometry, groups, 941 reweighted garment vertices, 282 bang vertices and both source/new-garment influence losses.

## Executed validation

| Check | Result |
| --- | --- |
| `sh tests/run-wiz-model-tests.sh /workspace/wiz-upgrade-reference/wiz-model-v1.bin /workspace/wiz-motion-rework/wiz-v2.bin` | ASan/UBSan: malformed v3 data, v1/v2 compatibility, quaternion interpolation, passive under/critical/overdamped spring response, gravity equilibrium, moving-frame invariance, acceleration lag, gesture scheduling, 48-second deterministic 30/60/120-Hz inputs, finite matrices, cones/lengths/capsules, feet, stalls and dynamics modes. Also checks actual skinned garment edges/seams and confirms chest source transforms contain no baked wobble. |
| `MESA_SHADER_CACHE_DIR=/tmp/npc-mesa-cache sh tests/run-wiz-model-gpu-tests.sh /workspace/wiz-motion-rework/frames` | Real surfaceless Mesa GLES3: 48 seconds / 1,440 cached renders, no clipped pixels in 960 saved motion frames, current-pose landmarks, front/quarter/both side views, camera cache limits, fixed storage/upload counts, hostile shared-GL-state restoration, owned-resource loss, EGL recreation and failure-only real PNG fallback. |
| `sh tests/run-anime-preview-tests.sh` | Existing preview/fallback geometry and actual ImGui Orbit/Front touch/scroll coexistence suite. |
| NDK r22 ARM64 / API 21 | Built `libnets.so` using the existing Android.mk/Application.mk. The final asset was verified inside the built library. An APK build is not claimed. |
| Asset reproduction and materials | A second export is byte-identical; all three embedded PNG hashes match the preceding feature asset. |

The optional CPU arguments add original asset compatibility checks. Current CPU tests also check that upper-arm and forearm segment lengths remain constant throughout all poses at 30/60/120-Hz input rates. The production `AnimePreview::Draw` GPU test opens the panel after 1,000 seconds of host uptime and verifies more than 60 pixels of articulated hand travel, a visible hop and safe replay after hiding the tab. The clock unit test covers loading, duplicate draws, hidden frames, time reversal and long suspension. ASan/UBSan scripts retain the existing environment-specific leak-detection setting. GPU captures are software GLES output, not an Android recording. The test still reads/checks all 960 sampled frames but saves a bounded set by default. Set `WIZ_CAPTURE_SECONDS=12` to capture every sampled frame of the first demonstration, or `48` for the full run. The native build command is:

```sh
/workspace/toolchains/android-ndk-r22/ndk-build NDK_PROJECT_PATH=. \
  APP_BUILD_SCRIPT=src/main/jni/Android.mk \
  NDK_APPLICATION_MK=src/main/jni/Application.mk \
  NDK_OUT=/workspace/liquid-build/obj NDK_LIBS_OUT=/workspace/liquid-build/libs -j2
```

Measured over 48 seconds, planted-foot error stayed below 0.0000006 m, and the spring-tip capsule penetration metric stayed below 0.0001 m. Maximum cloth-link strain was about 0.0056. Skinned garment edges of at least 4 mm ranged from 0.8487 to 1.1997 times the same poses with physics disabled; 103 duplicated garment seams showed no measured separation. Chest-tip acceleration lag reached 0.0363 m during the hop/landing and averaged 0.0066 m in the later settling window. These are deterministic host-rig measurements, not guarantees about every triangle or device.

Representative idle, blink, wave, anticipation, jump, landing, settling, later gestures and orbit frames were inspected against the reference and preceding output. The folded-hem spikes seen during development were corrected before the final captures. Review artifacts are outside the repository: `/workspace/wiz-motion-rework/comparison.mp4` (preceding feature vs revised first 12 seconds), `/workspace/wiz-motion-rework/preview.mp4` (48 seconds), `final-contact.png` and `orbit-contact.png`. Frame captures are sampled at 20 Hz; the renderer itself remains cached at 30 Hz.

## Exact files in this revision

- `src/main/jni/ImGui/WizMotion.cpp`, `WizMotion.h`: moving-frame spring integration, connected cloth solve, chest physics, expressive idle, independent gesture schedule and projected skinned bounds.
- `src/main/jni/ImGui/WizModel.cpp`, `WizModel.h`: v1/v2/v3 decode, cloth-link validation, current-pose bounds and resource accounting.
- `tools/export-wiz-model.py`: garment topology/weights, extended skirt rig, physical descriptors, source motion and v3 output.
- `src/main/jni/Assets/wiz-model.bin`, `wiz-model.json`, `WIZ_LICENSE.md`: exported rig and preserved attribution.
- `tests/wiz_model_test.cpp`, `wiz_model_gpu_test.cpp`: physical-response, garment, extended-motion and real rendered-frame checks.
- `docs/ESP_3D_UPGRADE.md`, `NEXT_STEPS.md`: review notes and remaining device validation.

The preview integration preserves gameplay hooks, auth/licensing flow, music, saved ESP fields and menu styling. This delivery separately removes the retired login-policy toggle, its config fields, runtime/profile, installer calls and dedicated tests at the user's request. Agent-added source comments were removed; original comments and artist attribution are retained.

## Limits and device review

This is a constrained physical rig with a coarse coupled cloth control net. It is not triangle-level cloth/self-collision or volumetric soft-body simulation. Body capsules constrain spring tips; they do not prove all intervening hair cards and garment triangles are collision-free for every possible pose. The same original gesture assets are reused at varied intervals, with new procedural idle layered on them; a motion-capture library was not added.

Android touch backend behavior, real device CPU/GPU time, FPS, thermal load, process memory and background/resume still require phone testing. Check cloth/hair/chest motion at the actual ESP panel size, all overlay combinations, Orbit/Front and adjacent scrolling, reopening the tab, EGL recreation and asset/GPU allocation failure. Host tests and an ARM64 library build do not establish those device results.

## Current integration captures

Current test logs are under `/workspace/license-hardening/logs/esp-fixed-*.log`. Captured native GLES frames and the review animation are under `/workspace/license-hardening/esp-fix/`. These are current host-rendered results; older `/workspace/wiz-motion-rework/` artifacts above are historical reference material. The automatic sequence includes idle, an articulated wave, jump anticipation, a ballistic hop, cushioned landing and settling; it is no longer driven by the global application uptime.

Current integration checks passed: ASan/UBSan Wiz CPU/compatibility/limb tests; real surfaceless GLES3 motion, production playback and fallback tests; preview clock/Orbit touch tests; Settings controls; hitbox/config and trigger regressions; signed-session and full-panel HTTP security tests; ARM64 NDK build and ELF hardening. The model exporter was rerun and matched the exact asset hash above. Current review files are `wiz-preview-fixed.mp4` and `verified-poses.png` in the integration-capture directory.
