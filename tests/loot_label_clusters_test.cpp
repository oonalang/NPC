#include "../src/main/jni/System/Hooks/LootLabelClusters.h"
#include <cassert>
#include <limits>
#include <vector>

int main() {
    using loot_labels::Assign;
    using loot_labels::Group;
    std::vector<Group> groups;
    assert(Assign(groups, 100.0f, 100.0f, 20.0f, 0, true) == 0);
    assert(Assign(groups, 110.0f, 111.0f, 30.0f, 1, true) == 0);
    assert(groups.size() == 1);
    assert(groups[0].count == 2 && groups[0].representative == 0);
    assert(Assign(groups, 120.0f, 121.0f, 7.0f, 2, true) == 0);
    assert(groups[0].representative == 2 && groups[0].distance == 7.0f);
    assert(Assign(groups, 490.0f, 450.0f, 9.0f, 3, true) == 1);
    assert(Assign(groups, 120.0f, 122.0f, 8.0f, 4, false) == 2);
    assert(Assign(groups, std::numeric_limits<float>::quiet_NaN(), 50.0f, 2.0f, 5, true) == -1);
    assert(groups.size() == 3);
    std::vector<Group> limited;
    for (std::size_t i = 0; i < 120; ++i)
        assert(Assign(limited, static_cast<float>(i * 200), 0.0f, 1.0f, i, false) == static_cast<int>(i));
    assert(Assign(limited, 29000.0f, 0.0f, 1.0f, 121, false) == -1);
    return 0;
}
