#include "../src/main/jni/BlueprintTierFilter.h"
#include <cassert>
#include <string>
#include <utility>

int main() {
    using namespace BlueprintTierFilter;
    assert(DetectTier("[MYTHIC] AK117") == Mythic);
    assert(DetectTier("[M] Test Weapon") == Mythic);
    assert(DetectTier("[LEGENDARY] DR-H") == Legendary);
    assert(DetectTier("[L] Test Weapon") == Legendary);
    assert(DetectTier("[EPIC] Test Weapon") == Epic);
    assert(DetectTier("[E] Test Weapon") == Epic);
    assert(DetectTier("[COMMON] Test Weapon") == Common);
    assert(DetectTier("[C] Test Weapon") == Common);
    assert(DetectTier("Unknown skin") == 0);

    // A filter selected before the (background-loaded) catalog exists must not
    // hide every blueprint.
    SetMask(Mythic);
    assert(!Contains(101));
    assert(Allows(101));
    assert(Allows(999));

    TierCatalog entries;
    entries[101] = Mythic;
    entries[102] = Legendary;
    entries[103] = Epic;
    entries[104] = Common;
    entries[105] = 0;
    Publish(std::move(entries));

    SetMask(0);
    assert(Allows(101));
    assert(Allows(102));
    assert(Allows(105));
    assert(!Contains(999));
    assert(Contains(105));
    assert(Preview() == "All blueprints");

    SetMask(Mythic);
    assert(Allows(101));
    assert(!Allows(102));
    // Untagged and unknown blueprints stay visible rather than disappearing.
    assert(Allows(105));
    assert(Allows(999));
    assert(Preview() == "Mythic");

    Toggle(Legendary);
    assert(Allows(101));
    assert(Allows(102));
    assert(!Allows(103));
    assert(Preview() == "Mythic, Legendary");

    Toggle(Mythic);
    assert(!Allows(101));
    assert(Allows(102));
    assert(Preview() == "Legendary");

    SetMask(0);
    assert(Allows(103));
    assert(Allows(104));
    assert(Allows(105));
    return 0;
}
