# Wiz preview attribution

**Wiz – Konosuba** by **Glayer3d**

- Model: https://sketchfab.com/3d-models/wiz-konosuba-b02f0fe5868347c6a7efe21d463b3734
- Author: https://sketchfab.com/Glayer3d
- Author-provided FBX and textures: https://drive.google.com/drive/folders/1Z9vllyNq5Qg14kL8_oCfwJHarkawPMF0
- License shown on the model page: **Creative Commons Attribution-NonCommercial 4.0 International** — https://creativecommons.org/licenses/by-nc/4.0/

`wiz-model.bin` contains the artist's textured mesh and 98-bone skeleton, a new 12-second animation and the original Blink, Smile and Speak facial shapes. The animation adds planted-foot idle weight shifts, breathing, glances, blinks, an articulated wave, an anticipated hop and a cushioned landing. Hair follows the movement. The runtime uses GPU skinning and a cached offscreen texture; it does not import an FBX or decode an image each frame. The artist's painted textures are retained, reduced to 1024 pixels for body/hair and 512 pixels for the face.

`wiz-preview.png` remains an orthographic render used only as a compatibility fallback if the 3D renderer cannot initialize. Both assets derive from the same original model. The model's noncommercial license applies to these derivatives; these files do not grant commercial rights.

Reproduce with Blender 4.3.2 after downloading the four files from the author's folder:

```sh
blender -b -t 2 -P tools/render-wiz-preview.py -- /path/to/WizFiles src/main/jni/Assets/wiz-preview.png
blender -b -t 2 -P tools/export-wiz-model.py -- /path/to/WizFiles src/main/jni/Assets/wiz-model.bin
```

The exporter writes `wiz-model.json` with mesh/animation counts, dropped-weight error and measured foot-target drift. The binary starts with `WIZSKN01`, a little-endian uncompressed-size field and zlib data. The payload contains its version/counts/duration, 80-byte skinned vertices, triangle indices, material PNGs and 24-Hz bone/facial/landmark samples. `WizModel.cpp` validates lengths, indices, joints, weights and finite animation values before upload. The GPU target is 320 × 512, refreshed at most 30 times per second while the ESP panel is visible. ESP overlay anchors use the same sampled skeleton as the displayed frame.

Source SHA-256:

```text
83522a7d2bb7125857111b6313211c56213da503155abefa2cf9c249299f23d0  Wiz.fbx
3ea756effd0456f5946f13988b46c5b99a5fab94b04ae4afb9f2743137344e60  WizB_Base_color.png
9ef661a9b82246ef271e034976cf767075f6d5123a1f0d06e76205301a15c005  WizH_Base_color.png
a40218c58e2209a4aabf287edd4f1bdb382c2aa635d174d6cec98037f440cdab  WizF_Base_color.png
```
