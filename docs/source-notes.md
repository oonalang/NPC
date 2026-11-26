# Notes moved out of the source

Comments that used to sit inside the native sources are collected here instead, so the code stays free of them.

## WeaponProjectile layout (`System/Hooks/ThrowableAlert.h`)

`ThrowAlertCfg` reads a live projectile through these offsets (game build 1.6.59):

| Constant | Offset | Field |
| --- | --- | --- |
| `PosRva` | `0xB4724B4` | `WeaponProjectile.Position` |
| `TickRva` | `0xB48214C` | `WeaponProjectile.Tick(float)` |
| `Exploded` | `0xA1` | `WeaponProjectile.bHasExplode` |
| `DamageRadius` | `0xA8` | `WeaponProjectile.m_DamageRadius` |
| `ItemId` | `0xE4` | `WeaponProjectile.m_WeaponItemID` |
| `LethalRadius` | `0x200` | `WeaponProjectile.m_LethalRadius` |

`MaxDist` (70 m) is how far a thrown projectile is tracked at all; `WarnDist` (35 m) is where the on-screen warning starts. Both are in metres of world distance, not screen pixels.

## Unlimited Ammo hooks (`System/Hooks/Feats.h`)

Two hooks make the feature complete, because the game tracks reserve ammunition and the shot cost separately:

| RVA | Hook | Effect |
| --- | --- | --- |
| `0xC2BAB48` | `get_InfiniteCarriedAmmo` | Reports unlimited reserve ammunition, so magazines can be refilled |
| `0x6AE86E0` | `GameInfo::IsNoCostAmmo` | Removes the cost of the shot itself, so firing does not consume the magazine |

Hooking only the first leaves the magazine draining; hooking only the second leaves a magazine that cannot be refilled. `UnlimitedAmmoCfg` keeps the two addresses together for that reason.

## Red Wallhack

The red outline is the game's EM3 eye effect, so the mod does not paint it: it makes the game believe the effect is active for every pawn and widens the distance at which that is accepted.

| RVA | Hook | Effect |
| --- | --- | --- |
| `0xA0B6018` | `IsInEM3Eye` | Forces the eye state on while the toggle is set |
| `0xB1A2814` | `GetAccDistance` | Returns the configured range instead of the stock value |

The range used to be a fixed `50.0f`. It is now `Config.ExtraMenu.RedWallhackRange`, clamped to 5–200 m and falling back to 50 for a non-finite value. A shorter range is less obvious because pawns only light up when they are genuinely close; longer ranges reach further at the cost of being easier to notice. Both the Lite panel and the runtime preview menu show the slider only while Red Wallhack is enabled, and the value is saved as `ExtraMenu.RedWallhackRange` next to the toggle.

## Game launcher activity (`MainActivity.java`)

The launcher used to hold one hardcoded class, `com.tencent.tmgp.cod.CODMainActivity`. That class exists only in the Tencent build. A Garena install (`com.garena.game.codm`) does not contain it, so `Class.forName` failed, the launcher reported "Official build verification failed" and the game never started. Every other target in the project was already Garena: `SaveConfig.h`, `MusicPlayer.h`, `FeedbackService.cpp` and `GuestAccount.h` all use `com.garena.game.codm`.

The class is now resolved at runtime instead of trusted from one string. In order:

| Step | Class |
| --- | --- |
| 1 | `com.garena.game.codm.MainActivity` |
| 2 | `com.garena.game.codm.CODMainActivity` |
| 3 | `com.tencent.tmgp.cod.CODMainActivity` |
| 4 | `com.activision.callofduty.shooter.MainActivity` |
| 5 | `com.activision.callofduty.shooter.CODMainActivity` |
| 6 | any activity declared in the merged manifest whose class name ends in `MainActivity` |

Steps 1, 2, 4 and 5 are attempts, not facts: a name is used only when `Class.forName` can load it. Step 6 covers the case where Garena renames the activity, because the merged APK keeps the game's own activity declarations, so the real class is discovered from the manifest rather than guessed. When nothing resolves, the launcher says the build does not match the installed game instead of blaming release verification.

Because `startActivity` needs the class inside the same APK, this resolution only works when the mod is merged into the game APK. Do not copy an activity name from another project: read it from the Garena APK's `AndroidManifest.xml` if a build ever needs the string written down.

## ESP and pickup cost notes

The BR performance work is easier to review with the intent written down:

- The player list and the local pawn are validated once per frame; per-enemy validation is gone.
- Health is read only when the health element is actually drawn or needed for a filter.
- Non-finite world positions are dropped before projection, so a torn read cannot poison the draw list.
- Camera world-to-screen matrices are cached per frame and reused for every enemy, skeleton point and tracer.
- `Tools::IsPtrValid` caches which mappings are readable instead of probing with a per-call write.
- Pickup runs its fast path before the general interaction logic so a full lobby of loot does not stall the render loop.

- Pickup runs its fast path before the general interaction logic so a full lobby of loot does not stall the render loop.

## Panel address in the launcher (`MainActivity.java`)

The launcher used to carry the panel address as plain literals: `BASE`, `HOST`, `TICKET_PATH` and the `/lib/protected/` prefix a ticket is checked against. Decompiling the DEX, or running `strings` over it, printed `https://ethnir.x10.mx` and `/lib/token` straight away, which is how the panel is found before anyone needs to break the key.

Those strings are no longer stored as text. Each one is a list of scrambled bytes (`LINK`, `TICKET`, `PREFIX`); `unfold(list, shift)` XORs them back through `MIX` and the result is assigned to the same constants the rest of the class already used, so nothing else had to change. `HOST` is cut out of `BASE` instead of being written down twice. The library name is gone from the source as well: the cache file is named after the ticket's `file` field, so a `libnets.so` upload is saved and loaded as `libnets.so` with no local name to keep in sync.

| List | Shift into `MIX` | Becomes |
| --- | --- | --- |
| `LINK` | 0 | `BASE` |
| `TICKET` | 7 | `TICKET_PATH` |
| `PREFIX` | 13 | `PROTECTED` |

What this buys and what it does not:

- A DEX text search, `strings` or a decompiler's find-text now shows only numbers for the host, the ticket path and the protected prefix.
- It is not cryptography. Anyone who reads `unfold` can copy it and recover the strings; an address the app has to call cannot be hidden more strongly than that.
- The real gate stays the HMAC key in `SECRET` and the RSA-signed manifest. A recovered address still gets a 403 without a valid ticket, and a ticket lives five minutes.
- `SECRET` is a 64-hex literal in the DEX and can be extracted the same way. Treat it as rotatable: rotate it on the panel and rebuild the launcher if it leaks.
- The domain also appears inside the native library (`AntiChangePanel.h` and `Jni_Stuff.h` both build `connect.php` from it), so `strings libnets.so` still shows it. Hiding only the Java side does not hide the domain from someone who inspects the library.

Changing the panel address later means new `LINK`/`TICKET`/`PREFIX` lists, not a string edit.

These change cost, not behaviour: the same information is drawn with fewer reads and fewer failed probes.
