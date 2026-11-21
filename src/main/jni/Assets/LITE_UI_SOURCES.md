# Supplied Celerity UI sources

The user supplied `celerity _1_.rar` and requested an Android port of its actual UI code for ETHNIR LITE. Archive SHA-256: `15051b388b78a24996f9855c375140eb284979029a81ccf67db6d555d8e93c93`.

`Lite/Widgets.h` adapts the drawing geometry from `ui/menu/elements/checkbox.h`, `slider.h` and `button.h`, plus `DrawBox` in `ui/menu/menu.cpp`. `Lite/WidgetBindings.cpp` adapts the compact field drawing in `dropdown.h`, using NPC's ImGui popup implementation for Android input and clipping. `Lite/Colors.h` derives from the supplied `colors.h`. `Lite/Menu.h` adapts the supplied menu frame, sidebar groups, selection rows and dimensions, with ETHNIR LITE branding and NPC's existing controls.

Desktop Win32/D3D11 input, renderer, blur, keybind registry and CS feature implementations are excluded. Touch taps commit on release; vertical drags scroll instead of changing controls. The sidebar is opaque to avoid a blur pass. Checkbox checkmarks use ImGui vector geometry; the seven sidebar glyphs come from the supplied icon font. Repetitive desktop widget animation maps are omitted.

Fonts are imported from the supplied byte arrays by `tools/export-lite-fonts.py`, retaining original glyph outlines and name/copyright/license records. The text subsets retain U+0020–U+00FF; the icon subset retains seven glyphs. FontTools is an offline import dependency, not an Android build/runtime dependency.

| Supplied file | Source SHA-256 | Imported asset |
| --- | --- | --- |
| `SFProText-Semibold.hpp` | `daddc8180b5e7c4aad7f8f71c1ffebca8b05962e439594ef2bc631666cd27f63` | `lite-body.ttf`, 36,440 bytes |
| `sfpro-bold.hpp` | `4c9ccdc49f3b10f3ce2c80a8e2dc06be614f5d803a3317d695eb0c0a7cf43b46` | `lite-title.otf`, 30,636 bytes |
| `fontawesome_binary.hpp` | `be994d16330f6e25b8f29f833425c45fdf83e885c3a6139cfe51b36489c12289` | `lite-icons.ttf`, 3,152 bytes |

The text fonts identify as SF Pro Text and SF Pro Display, © 2015–2017 Apple Inc. Their retained license record refers to the applicable iOS Software License Agreement. The icon font identifies as Font Awesome 5 Pro Regular, copyright Font Awesome. The supplied archive did not include a project-wide license or a separate font redistribution license. This port does not grant additional rights to those source materials. Preserve these records and obtain any required distribution rights before distributing the supplied fonts.

The original Wiz assets and `WIZ_LICENSE.md` remain in source history; they are excluded from the Lite native library.
