# ESP visibility, throwables and BR loot testing

Source baseline: NPC `main`, based on 1.6.59 layouts and the existing rendering code.

## Visibility-based ESP colors

- Enable **Visibility Check** in ESP Settings. Visible players and bots retain their configured ESP line and box colors. A blocked head sample uses **Blocked ESP Color**, initially white.
- The visibility ray starts at the active camera and ends at the projected player's head. The world trace uses the same 1.6.59 world-collision function already present in `Trigger/NativeQueries.cpp` at RVA `0x66F2708`. It does not claim to distinguish all glass, smoke, partial cover, or moving geometry.
- The renderer budgets at most three new world traces per render frame, only considers enemies within 220 game-world units, and reuses fresh samples for roughly 130–190 ms. Samples are cleared when the match pointer changes or visibility checking is turned off. If a sample cannot be obtained, the existing player/bot color is used.
- Check visibility states while stepping behind cover, peeking, respawning, and switching MP/BR. Compare FPS with the visibility setting disabled and enabled.

## Throwable danger alerts

- Reuse the existing projectile tick hook and 1.6.59 radius/position fields; do not import offsets from other game builds.
- Each tracked projectile retains up to 24 path points. At most 96 projectile states are stored, 12 are rendered per frame, and outdated entries expire after three seconds without updates.
- Danger-circle rendering uses 30 segments. Nearby threats show distance and estimated native radius. Urgent proximity receives a red warning; ordinary proximity receives amber.
- A shared mutex protects the tracker across game and render threads; the enable state is published through an atomic.
- Validate grenades close and far, multiple projectiles, throwables that explode, and projected trajectories. An estimated radius is not guaranteed to match every game damage mechanic.

## BR Item ESP: verify before enabling label grouping

The new **Loot callbacks / Accepted / Rejected / Labels / Grouped** diagnostic row makes the pickup pipeline observable without pretending that the user-facing toggle proves tracking works.

1. Enable ESP Items (BR). Stand near a known dropped weapon and ammunition with the loot category filters enabled.
2. Confirm **Loot callbacks** increases. A zero callback count suggests the native tick hooks are not executing or the setting is not reaching the game thread.
3. Confirm **Accepted** increases and **Tracked pickups** is nonzero. A growing **Rejected** count with no accepted pickups suggests invalid item fields or pickup class assumptions.
4. Check that visible labels match actual items. Confirm search, category filters and distance limits.
5. Only then enable **Group overlapping loot labels**. The closest label represents a nearby screen-space group, followed by `(+N)`. Label grouping is off by default.

The optional grouping is bounded at 120 candidates and reuses the original immediate rendering path while disabled. Neither the initial picker fields nor the name getters have been runtime verified.

## Other BR frame-time cleanup

The previously per-enemy FOV ring, target Aim Line search, and vehicle ESP drawing now execute once per rendering frame. Previously these passes could repeat once for every on-screen enemy, multiplying work in busy BR scenes.

## Validation

Run the host-only grouping test with a C++17 compiler:

```sh
sh tests/run-esp-label-tests.sh
```

The source and static integration checks have been inspected; native ARM64 compilation, the host C++ execution and on-device FPS measurements have not been performed in this review. Keep this as a draft until tested in both MP and BR.
