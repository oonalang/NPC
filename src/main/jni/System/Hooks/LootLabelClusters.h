#pragma once

#include <cmath>
#include <cstddef>
#include <vector>

namespace loot_labels {

struct Group {
    float x = 0.0f;
    float y = 0.0f;
    float distance = 0.0f;
    std::size_t representative = 0;
    unsigned count = 0;
};

inline int Assign(std::vector<Group>& groups, float x, float y,
                  float distance, std::size_t itemIndex, bool clustering) {
    if (!std::isfinite(x) || !std::isfinite(y) ||
        !std::isfinite(distance) || distance < 0.0f) return -1;

    if (clustering) {
        for (std::size_t i = 0; i < groups.size(); ++i) {
            Group& group = groups[i];
            if (std::fabs(group.x - x) < 140.0f &&
                std::fabs(group.y - y) < 31.0f) {
                ++group.count;
                if (distance < group.distance) {
                    group.x = x;
                    group.y = y;
                    group.distance = distance;
                    group.representative = itemIndex;
                }
                return static_cast<int>(i);
            }
        }
    }
    if (groups.size() >= 120) return -1;
    groups.push_back(Group{x, y, distance, itemIndex, 1});
    return static_cast<int>(groups.size() - 1);
}

}
