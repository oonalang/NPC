# Fast Pickup and Pickup Range, BR 1.6.59

## Controls

The ESP options panel now includes a **BR PICKUP** section, independent of the **ESP Items (BR)** checkbox.

- **Fast Pickup (BR)**, off by default. The existing `BRNearbyModule.GetAutoPickUpInterval()` hook returns the lesser of the normal interval and a 45% scaled interval with a 40 ms lower bound. The value is unchanged when disabled or nonfinite.
- **Extended Pickup Range (BR)**, off by default. A **Pickup Range** slider sets 3–15 game units, initialized at 6. These units correspond to the world-distance values used by the game and are presented as meters in the UI.

## Native implementation

The 1.6.59 dump identifies:

- `BRNearbyModule.GetAutoPickUpInterval()` at RVA `0x90F6DE8` and the base virtual method at `0x652EC48`
- `BRDroppedPickUpMgr.Tick(float)` at RVA `0x89FF498`
- `BRDroppedPickUpMgr.InitMaxPickUpDistance()` at RVA `0x8A00DA0`
- Inherited manager fields: `OpenWorldDroppedPickUpMgr.MaxPickUpHorizontalDistanceSquare` offset `0x70`, `MaxPickUpVerticalDistance` offset `0x74`

The BR-only auto-pickup getter is adjusted without modifying manual pickup requests. Range updates modify the horizontal squared threshold only, preserving the original vertical threshold and avoiding lower-than-native limits. Disabling range restores the original horizontal value when it still matches the last value assigned by this feature. Manager initialization and address changes reset baseline state. The game thread owns manager updates; the menu publishes settings atomically.

All native calls are wrapped with the existing licensed feature guard. No changes are made to MP-specific pickup hooks.

## Important limits

The 1.6.59 dump establishes symbols and field layouts, not live behavior or server-side authorization. Increasing the client's local pickup range does not guarantee the server accepts a pickup from farther away. The game may have other checks beyond these fields.

Fast Pickup changes the cadence of **automatic** pickup, not guaranteed manual tap speed. The game may throttle requests elsewhere, and a 40 ms lower bound avoids a zero-interval request loop.

No 1.6.59 binary equality assertion, host test execution, Android compilation, or in-match verification has been made in this static review.

## Test checklist

1. Enable Fast Pickup, leave Pickup Range disabled, then test dropping and auto-acquiring items at the normal distance. Compare against disabled behavior.
2. Enable Extended Pickup Range, set 6 units, and test pickup at the native range boundary. Increase only when the earlier distance works.
3. Check the **Auto pickup checks** and **Range updates** diagnostics. Zero counters during BR activity suggest hooks are not being invoked.
4. Disable both controls and verify the original pickup distance and interval return.
5. Change the slider while inside BR, exit the match and rejoin, then repeat.
6. Repeat in MP. The BR-only controls should not alter MP pickup.
7. Watch native crash logs, latency, network errors and FPS throughout.

Run the host-native regression script:

```sh
sh tests/run-pickup-tests.sh
```

The test verifies interval calculations, state restoration, new manager identity, bounds and unchanged vertical distance. It does not test server-side range acceptance.
