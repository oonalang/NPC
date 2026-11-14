# Wiz preview attribution

**Wiz – Konosuba** by **Glayer3d**

- Model: https://sketchfab.com/3d-models/wiz-konosuba-b02f0fe5868347c6a7efe21d463b3734
- Author: https://sketchfab.com/Glayer3d
- Author-provided FBX and textures: https://drive.google.com/drive/folders/1Z9vllyNq5Qg14kL8_oCfwJHarkawPMF0
- License shown on the model page: **Creative Commons Attribution-NonCommercial 4.0 International** — https://creativecommons.org/licenses/by-nc/4.0/

`wiz-preview.png` is an orthographic transparent render of that model with the author's original painted textures. It replaces the generated original mascot. The preview adds mesh deformation for breathing, hair, a small cheer/jump and secondary motion. The game loads the rendered PNG, not the FBX. The model's noncommercial license applies to this derivative; these files do not grant commercial rights.

Reproduce with Blender 4.3.2 after downloading the four files from the author's folder:

```sh
blender -b -t 2 -P tools/render-wiz-preview.py -- /path/to/WizFiles src/main/jni/Assets/wiz-preview.png
```

Source SHA-256:

```text
83522a7d2bb7125857111b6313211c56213da503155abefa2cf9c249299f23d0  Wiz.fbx
3ea756effd0456f5946f13988b46c5b99a5fab94b04ae4afb9f2743137344e60  WizB_Base_color.png
9ef661a9b82246ef271e034976cf767075f6d5123a1f0d06e76205301a15c005  WizH_Base_color.png
a40218c58e2209a4aabf287edd4f1bdb382c2aa635d174d6cec98037f440cdab  WizF_Base_color.png
```
