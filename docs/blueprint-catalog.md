# Blueprint rarity catalog

Rarity filtering reads the captured WeaponConfConfig records directly. The
1.6.59 dump identifies the ID at 0x40 and ColorID at 0x22. This catalog can be
published before full cosmetic equip profiles exist, so selecting Mythic and
Legendary does not make the list depend on unrelated cosmetic asset tables.

Constructor capture can precede deserialization. Incomplete rows are retried at
a bounded 500 ms cadence; a matching constructor count only stops rebuilding
after every captured row has an ID. Unknown or conflicting rarities remain
excluded from explicit tier selections.

Equip interception still requires a complete matching profile. Missing profiles
fall through to the original game handler. Full profile loading retains its
resource readiness checks to avoid writing zero model or effect references.
Variant profiles initialize their base inventory IDs explicitly.

Run the catalog regression with:

```sh
c++ -std=c++17 tests/blueprint_catalog_test.cpp -o /tmp/blueprint-catalog-test
/tmp/blueprint-catalog-test
```

It covers Mythic and Legendary filtering before cosmetic profiles are ready,
late metadata with an unchanged constructor count, deferred application, and
rejection of an unselected Epic entry. Actual list refresh and equipment behavior
still require an in-game check.
