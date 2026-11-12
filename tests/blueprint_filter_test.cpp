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

    assert(TierFromColorId(5) == Mythic);
    assert(TierFromColorId(4) == Legendary);
    assert(TierFromColorId(3) == Epic);
    assert(TierFromColorId(0) == Common);
    assert(TierFromColorId(1) == Common);
    assert(TierFromColorId(2) == Common);
    assert(TierFromColorId(-1) == 0);
    assert(TierFromColorId(6) == 0);

    SetMask(Epic);
    assert(!Contains(101));
    assert(!Allows(101));
    assert(!Allows(999));

    TierCatalog entries;
    entries[101] = Mythic;
    entries[102] = Legendary;
    entries[103] = Epic;
    entries[104] = Common;
    entries[105] = 0;
    RecordTier(entries, 106, Epic);
    RecordTier(entries, 106, Epic);
    RecordTier(entries, 107, Epic);
    RecordTier(entries, 107, Legendary);
    RecordTier(entries, 107, Epic);
    RecordTier(entries, 108, Legendary);
    RecordTier(entries, 108, Epic);
    RecordTier(entries, 109, 0);
    RecordTier(entries, 109, Epic);
    RecordTier(entries, 0, Epic);
    Publish(std::move(entries));

    assert(Allows(103));
    assert(Allows(106));
    assert(!Allows(101));
    assert(!Allows(102));
    assert(!Allows(104));
    assert(!Allows(105));
    assert(!Allows(107));
    assert(!Allows(108));
    assert(!Allows(109));
    assert(!Allows(999));
    assert(!Allows(0));
    assert(Preview() == "Epic");

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
    assert(!Allows(105));
    assert(!Allows(999));
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
