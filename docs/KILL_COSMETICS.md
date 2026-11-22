# Automatic mythic kill cosmetics

This independent implementation is on `feature/ethnir-lite`, targeting the supplied ARM64 CODM 1.6.59 ELF and its pinned build ID. No seller header was supplied or imported. It supersedes the initial manual cosmetic selection design in `25c04b2`.

Automatic cosmetics defaults on. The three Skins categories show status and a shared opt-out checkbox; they no longer offer arbitrary cosmetic IDs or cosmetic rarity filters. Equipping a confirmed mythic blueprint selects compatible kill icon, board and broadcast data from the game's own mappings. Other skins retain original game behavior. The weapon's native `WeaponConfConfig.ColorID` determines mythic eligibility, not the cosmetic row's ColorID, a displayed label, an item-ID suffix or the blueprint-unlock filter.

Four collectors observe successful deserialization of weapon blueprints and the three `SPKill*ItemConfConfig` tables. Seven hooks require the pinned module identity and 22 fingerprints, including blueprint identity/tier and presentation field anchors. The cosmetic tables provide `WeaponCanEquip` associations. Matching prefers a compatible native-equipped cosmetic, then unique exact-blueprint, native-skin or base-weapon associations in that order. Missing data or ambiguous variants preserve original handling; the code never chooses an arbitrary highest/first cosmetic ID. Confirmed mythic blueprint IDs come from weapon config records. Applying an existing NPC visual skin records that selected blueprint for its weapon aliases, so an unmodified base inventory ID does not falsely classify an applied mythic as common. An explicit native blueprint takes priority over a different base weapon's saved visual override; only an override recorded for that exact blueprint may replace it. This prevents a stale base override from decorating a subsequently equipped common/legendary blueprint. Selecting a non-mythic visual skin stops the automatic override on subsequent UI events.

Only `KillCosmetics.Automatic` is saved. Missing or malformed values default to true; false is retained as an explicit opt-out. Legacy manual Icon/Board/Broadcast IDs are ignored and no longer saved. Other configuration values are unchanged.

The icon adapter passes matching background/foreground/effect data into normal medal presentation while preserving the original medal icon, text, sound and difficulty. Positive level-up/unlock presentations remain original. The board adapter only substitutes a matching local player's board lookup. The broadcast adapter additionally requires a local kill and matching current weapon, temporarily supplies the compatible broadcast ID during the native bind and restores it afterward. Nested calls do not double-apply; a subsequent native field change is retained. Normal GC handles protect icon strings and the temporary inventory across native calls. Static GC handle helper signatures include their verified leading unused argument.

Catalogs are bounded to 512 rows per category, 64 weapon associations per row and 512 confirmed mythic records; visual aliases are bounded to 1,024. Associations are indexed on metadata updates, not scanned every frame. UI draws status rather than a full item browser. Persistent icon roots are bounded by the icon catalog; temporary roots are released after presentation. No new image, rendering dependency, audio worker or thread is added. Hooked module unloading is not supported; persistent roots last for that module's process lifetime.

Downloaded game resources are required. Loading after config deserialization can leave the mappings unavailable; load earlier on the next launch. Standard medal/calling-card/MP kill-feed paths are the initial integration. Dedicated BR paths, live-game appearance, resource availability, stability/FPS and other-player visibility remain unverified. Late kills after switching weapons use stock broadcast handling when their weapon no longer matches current inventory. This does not force a highest upgrade level or guess among multiple compatible variants.

Validation commands:

```sh
sh tests/run-kill-cosmetics-tests.sh
python3 tests/verify-kill-cosmetics-profile.py /path/to/libunity.so
sh tests/run-lite-ui-tests.sh
LITE_GPU_TEST=1 sh tests/run-lite-ui-tests.sh /tmp/lite-cosmetics-auto-frames
```

Native-adapter fixtures cover actual tier gating, automatic switching, visual overrides, ambiguous/native-preferred mappings, stock handling, root lifetimes, locality/license/disable gates and scoped restoration. Real configuration file tests cover opt-out persistence and migration. UI tests cover six viewport sizes, actual touch opt-out/re-enable and warm allocation bounds. Captures use fixture catalog data; these checks do not constitute gameplay verification. Logs use `lite-cosmetics-auto-*` outside source.
