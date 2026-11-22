# Automatic mythic kill cosmetics

This independent implementation is on `feature/ethnir-lite`, targeting the supplied ARM64 CODM 1.6.59 ELF and its pinned build ID. No seller header was supplied or imported. It supersedes the initial manual cosmetic selection design in `25c04b2`.

Automatic cosmetics defaults on. The three Skins categories show status and a shared opt-out checkbox; they no longer offer arbitrary cosmetic IDs or cosmetic rarity filters. Equipping a confirmed mythic blueprint selects compatible kill icon, board and broadcast data from the game's own mappings. Other skins retain original game behavior. The weapon's native `WeaponConfConfig.ColorID` determines mythic eligibility, not the cosmetic row's ColorID, a displayed label, an item-ID suffix or the blueprint-unlock filter.

Four collectors observe successful deserialization of weapon blueprints and the three `SPKill*ItemConfConfig` tables. Seven hooks require the pinned module identity and 22 fingerprints, including blueprint identity/tier and presentation field anchors. The cosmetic tables provide `WeaponCanEquip` associations. Matching prefers a compatible native-equipped cosmetic, then unique exact-blueprint, native-skin or base-weapon associations in that order. Missing data or ambiguous variants preserve original handling; the code never chooses an arbitrary highest/first cosmetic ID. Confirmed mythic blueprint IDs come from weapon config records. Applying an existing NPC visual skin records that selected blueprint for its weapon aliases, so an unmodified base inventory ID does not falsely classify an applied mythic as common. An explicit native blueprint takes priority over a different base weapon's saved visual override; only an override recorded for that exact blueprint may replace it. This prevents a stale base override from decorating a subsequently equipped common/legendary blueprint. Selecting a non-mythic visual skin stops the automatic override on subsequent UI events.

Only `KillCosmetics.Automatic` is saved. Missing or malformed values default to true; false is retained as an explicit opt-out. Legacy manual Icon/Board/Broadcast IDs are ignored and no longer saved. Other configuration values are unchanged.

Startup installs only the four metadata collectors. The three board/medal/feed presentation hooks are installed during authorized feature setup after login. The shared installation lock preserves partial-install progress and checks remaining fingerprints on retry. Presentation availability is published only after all seven hooks complete; metadata collection does not require a logged-in session and automatic matching still defaults on.

The initial implementation used `Tools::ReadAddr` for managed fields. That helper changes surrounding pages to read/write/execute, which is inappropriate for game heap data. Cosmetic reads now use a private nonblocking pipe to copy at most 256 bytes through the kernel, without changing page permissions or depending on `process_vm_readv` or `/proc/self/mem`. Invalid, protected, partially readable and stale source ranges fail without publishing partial field values. Reads use a mutex, two close-on-exec descriptors and bounded interruption retries. Config fields and weapon arrays are copied in bounded snapshots rather than one syscall pair per array element. Descriptor-allocation/read failures leave native cosmetic behavior unchanged.

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

## Multiplayer correction

The integration is restored on Lite after the rollback. The old local-owner helper bound to `Gameplay.get_LocalBrPawn` at `0x8016060`. It is specific to Battle Royale and cannot supply the Multiplayer owner needed for the three cosmetic lookups. The binding now uses `Gameplay.get_LocalPlayerPawn` at `0x8012a6c`, verified against the supplied 1.6.59 dump and executable prefix.

The skin worker retries metadata installation up to 40 times and presentation installation up to 40 times after authorization. These retries use the existing 1.5-second worker interval and preserve partial hook-install progress. The three cosmetic pages show the last resolution result alongside catalog counts.

The host regression checks the installed helper address, missing-local-player handling, normal owner resolution and the prior catalog, license, configuration and resource-lifetime cases. Phone startup, actual HUD appearance, downloaded resource availability and the earlier reported native crash remain unverified. This correction addresses a verified lookup error. It does not establish the cause of the earlier startup crash.
