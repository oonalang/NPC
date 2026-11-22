# Lite kill cosmetics

This is an independent implementation on `feature/ethnir-lite`; no seller header or proprietary implementation was supplied or imported. It targets the supplied ARM64 CODM 1.6.59 ELF and its existing pinned build ID. The screenshots were a UI/feature reference, not a source of function addresses or a static item catalog.

Skins has three additional categories: Kill Icon, Kill Board and Kill Broadcast. Each supports search, rarity filtering, one active selection and Default to clear it. Selections start off and use the ordinary configuration save/load buttons. The optional `KillCosmetics` JSON object stores `Kill Icon`, `Kill Board` and `Kill Broadcast` IDs; absent or malformed values reset to Default. Runtime use additionally requires an ID actually observed in that category's native config table. Display labels use the recorded rarity and item ID, not invented cosmetic names.

The catalog records the game's `SPKillIconItemConfConfig`, `SPKillBoardItemConfConfig` and `SPKillBroadcastItemConfConfig` rows after their native deserializers succeed. Item types 60/61/68 are declared by the supplied game metadata. The table field offsets, native helper signatures and consumer paths were inspected against the supplied ELF. The module checks its build identity and 19 fingerprints before installing six hooks. Unsupported code, partial installation, absent catalog entries, disabled selections and missing feature authorization retain original behavior.

The icon adapter passes the selected background/foreground/effect into the existing normal medal presentation, preserving the original medal icon, text, sound and difficulty. Positive level-up/weapon-unlock presentations retain their original appearance. The mapping follows the native icon-preview path that reads `BoardUIName`, `TieUIName` and `EffectAssetID`. Managed strings use normal GC handles, with temporary roots across a presentation so a concurrent config refresh cannot release a string in use. The verified static GC handle wrappers take a leading unused argument; they cannot be called using a conventional no-instance IL2CPP signature.

The board adapter overrides the native player-specific board lookup only for the local player's ID. Existing native calling-card/event timing remains in control. The broadcast adapter wraps the native kill-feed bind only for a local kill whose weapon matches the local current inventory. It temporarily supplies that inventory's broadcast ID for the existing native asset lookup, then restores it. A temporary normal GC handle retains the inventory until restoration. Nested use of the same inventory does not double-apply the override; a subsequent native field change is retained. Other players, weapon mismatches and missing local context use original handling. This is a client UI integration, not an outgoing equipment request or a promise that other players see the selection.

All rendering and effect assets are loaded by the game's existing UI/resource code. This module adds no images, preview renderer, audio worker or rendering dependency. Each catalog is capped at 512 rows. Icon string roots are bounded by that catalog (two per icon, with two temporary roots per active medal presentation and one per local broadcast bind). Native collectors run when config rows deserialize; no render-loop scan is added. UI copies/filter lists refresh only after a catalog revision or filter change and use clipped rows. Persistent catalog roots last for the hooked module's process lifetime; unloading a live hooked module is not supported.

Limitations: downloaded resources are still required. Loading the module after these config tables have already deserialized can leave the lists empty; load it earlier on the next game launch. The initial native consumers are the standard medal, calling-card and MP kill-feed paths; dedicated BR variants, resource availability, installed-device appearance, input, stability and FPS are not verified. A late kill after switching weapons uses the stock broadcast when its weapon no longer matches current inventory. The catalog contains observed native rows only. Trigger behavior is outside this change.

Validation commands:

```sh
sh tests/run-kill-cosmetics-tests.sh
python3 tests/verify-kill-cosmetics-profile.py /path/to/libunity.so
sh tests/run-lite-ui-tests.sh
LITE_GPU_TEST=1 sh tests/run-lite-ui-tests.sh /tmp/lite-cosmetics-frames
```

The native-adapter tests use host fixtures for game functions and GC handles. The UI captures use fixture catalog entries; neither constitutes gameplay verification. Logs for this change use `lite-cosmetics-*` outside source. The existing native build and hardening commands apply.
