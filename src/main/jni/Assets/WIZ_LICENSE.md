# Wiz preview attribution

**Wiz – Konosuba** by **Glayer3d**

- Model: https://sketchfab.com/3d-models/wiz-konosuba-b02f0fe5868347c6a7efe21d463b3734
- Author: https://sketchfab.com/Glayer3d
- Author-provided FBX and textures: https://drive.google.com/drive/folders/1Z9vllyNq5Qg14kL8_oCfwJHarkawPMF0
- License shown on the model page: **Creative Commons Attribution-NonCommercial 4.0 International** — https://creativecommons.org/licenses/by-nc/4.0/

`wiz-model.bin` contains the artist's textured mesh, its original 98 bones plus 22 added bang/garment bones, a 12-second gesture source sequence and the original Blink, Smile and Speak facial shapes. The exporter identifies the original coat and bang components, adds and weights the new bones, and writes a real parent-local hierarchy. The runtime adds deterministic facial/idle variation, planted-foot stabilization and 60-Hz spring-bone dynamics for bangs, side locks, back hair, the top strand, skirt panels, sleeve hems and the original chest bones. Eight skirt chains are coupled by 48 distance, shear and bending links. The chest movement comes from acceleration-driven dynamics; its old baked oscillation is removed. Gestures are scheduled independently after the initial demonstration sequence. GPU skinning renders into a cached transparent target. Android does not import an FBX or decode images each frame.

All three embedded painted PNGs are byte-for-byte identical to the previous asset: body/hair remain 1024 pixels and the face 512 pixels. Mesh topology, UVs, facial morph deltas and the artist's original colors remain intact. Only the audited bang and garment vertex weights are extended. Folded interior garment weights follow mesh connectivity, so the hem does not jump between opposite panels. The new garment influences fit the existing four-weight limit without truncation; unchanged source influences elsewhere retain their previous maximum discarded weight of 0.02883. Garment hems follow hips and garment bones instead of inheriting the old leg twist. This is a derivative of Glayer3d's model, not a newly authored character mesh.

`wiz-preview.png` remains an orthographic render used only as a compatibility fallback if the 3D renderer cannot initialize. Both assets derive from the same original model. The model's noncommercial license applies to these derivatives; these files do not grant commercial rights.

Reproduce with Blender 4.3.2 after downloading the four files from the author's folder:

```sh
blender -b -t 2 -P tools/render-wiz-preview.py -- /path/to/WizFiles src/main/jni/Assets/wiz-preview.png
blender -b -t 2 -P tools/export-wiz-model.py -- /path/to/WizFiles src/main/jni/Assets/wiz-model.bin
```

The exporter writes `wiz-model.json` with mesh/animation counts, physical groups, reweighted-vertex counts, dropped-weight error and measured source foot-target drift. The binary starts with `WIZSKN03`, a little-endian uncompressed-size field and zlib data. Its hierarchy, inverse binds, local TRS samples, spring/collider descriptors, cloth links and landmark bindings are documented in [ESP_3D_UPGRADE.md](../../../../docs/ESP_3D_UPGRADE.md). The updated decoder also accepts `WIZSKN01` and `WIZSKN02`; old matrix-only assets render without claiming runtime rig physics. Re-exporting version 1 physics data requires the original FBX and textures, since skinning matrices do not encode the source rig.

The GPU target remains 320 × 512. Normal rendering, including camera dragging, uses one refresh per 1/30-second bucket while visible; physics runs independently at 1/60 second. Explicit resets and resource recreation can render a fresh target immediately. Overlay anchors and bounds projected from the current skinned vertices follow the same pose and camera as the displayed frame. PNG fallback is selected only after native decode/GPU failure and is labeled as a 2D compatibility fallback.

Source SHA-256:

```text
83522a7d2bb7125857111b6313211c56213da503155abefa2cf9c249299f23d0  Wiz.fbx
3ea756effd0456f5946f13988b46c5b99a5fab94b04ae4afb9f2743137344e60  WizB_Base_color.png
9ef661a9b82246ef271e034976cf767075f6d5123a1f0d06e76205301a15c005  WizH_Base_color.png
a40218c58e2209a4aabf287edd4f1bdb382c2aa635d174d6cec98037f440cdab  WizF_Base_color.png
```
