# ESP Items and ESP Placed Nades

## Scope

The new ESP page exposes two independent switches:

- ESP Items (BR): displays nearby dropped item labels, distances, and category colors.
- ESP Placed Nades (MP): displays detected placed projectiles and their distances.

Both switches start off and use the existing license-gated ESP render pipeline. Config profiles preserve each switch, its category filters, and range limits.

## BR item categories

Category values match the 1.6.59 `EBRDroppedPickUpType` enum:

| Value | Category | Default |
| --- | --- | --- |
| 1 | Weapons | On |
| 2 | Ammunition / clips | On |
| 3 | Equipment | On |
| 4 | Mods and attachments | On |
| 5-8 | Medical, throwables, skills, and perks | Off |

Loot name getters are mapped for weapons, clips, equipment, equip parts, medicine, throwing, skills, and perks. When an in-game item name cannot be resolved, the label shows the item category and numeric ID. Search filters the displayed label, including the item ID.

The item tracker reads `OpenWorldDroppedPickUpNormal`'s item data and tracks eligible visible world drops through its tick methods. In-box items are excluded. Only active tracker entries are rendered. As the class hierarchy includes additional overridden methods, full item coverage requires an Android test.

## MP deployables

Class-based matching supports:

- Trip mines
- C4 and tank-battle C4
- Tactical tripwires and electric tripwires, labeled tripwire/sensor
- Trophy systems
- Tank mines and trap-chip mines
- Drill charges

The default Enemy devices only filter compares the device spawn owner's team to the local pawn team. Devices with no known owner/team will not appear while this filter is enabled; switching it off shows all identified devices. No object is labeled enemy solely from its class name.

Both existing projectile-tracking and placed-device tracking use the same projected world positions. Placed-device markers remain distinct from the existing Throwable Alert trajectory/radius drawing. The tracker uses bounded entry caches and expires devices no longer receiving updates.

## Test checklist

1. Build the Android native library for arm64-v8a and open the mod in a supported 1.6.59 environment.
2. Enter BR, toggle ESP Items, and test dropped weapons, ammo, mods, medical supplies, and equipment by category.
3. Test loot search with a known weapon name and numeric item ID; collect the object and verify its label disappears.
4. Enter MP with a friendly and enemy deployable of each category. Confirm enemy-only behavior, ownership handling, label accuracy, and range filtering.
5. Test trophy and C4 objects after placement, pickup, and destruction; confirm stale labels are removed.
6. Test FPS and memory with many dropped objects and several projectiles at once.
7. Save a Config profile, reload, export it, then import it on another Android device; verify ESP filters and search text restore.

The native methods and class layouts were cross-checked against the uploaded `Dump 1.6.59.cs`. Source verification does not establish a binary match with the uploaded `libunity.so` or successful device functionality.
