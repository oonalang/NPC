#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "../src/main/jni/Skin/KillIcon.h"

namespace {

using namespace kill_icon;

using Buffer = std::array<unsigned char, 0x60>;

template <typename Array>
void Put(Array& buffer, std::size_t offset, std::uint32_t value) {
    std::memcpy(buffer.data() + offset, &value, sizeof(value));
}

std::uint32_t Get(const Buffer& buffer, std::size_t offset) {
    std::uint32_t value = 0;
    std::memcpy(&value, buffer.data() + offset, sizeof(value));
    return value;
}

Buffer Inventory(std::uint32_t item, std::uint32_t skin, std::uint32_t blueprint, std::uint32_t icon) {
    Buffer buffer{};
    Put(buffer, 0x14, item);
    Put(buffer, 0x34, skin);
    Put(buffer, 0x38, blueprint);
    Put(buffer, inventoryKillIcon, icon);
    return buffer;
}

std::array<std::uint32_t, 3> Keys(const Buffer& buffer) {
    return {kill_icon::ReadField(buffer.data(), 0x14),
            kill_icon::ReadField(buffer.data(), 0x38),
            kill_icon::ReadField(buffer.data(), 0x34)};
}

}

int main() {
    using namespace kill_icon;

    assert(ValidIcon(600000001u));
    assert(ValidIcon(609999999u));
    assert(!ValidIcon(0u));
    assert(!ValidIcon(1u));
    assert(!ValidIcon(610000001u));
    assert(!ValidIcon(680000021u));

    Remember(0u, 600000001u);
    assert(IconFor(0u) == 0u);
    Remember(101u, 610000001u);
    assert(IconFor(101u) == 0u);
    Remember(101u, 600000001u);
    assert(IconFor(101u) == 600000001u);
    Remember(101u, 0u);
    assert(IconFor(101u) == 0u);

    {
        Buffer inventory = Inventory(11, 12, 13, 19);
        Remember(13u, 600000007u);
        assert(Sync(inventory.data(), Keys(inventory)));
        assert(Get(inventory, inventoryKillIcon) == 600000007u);

        for (int i = 0; i < 3; ++i) {
            assert(Sync(inventory.data(), Keys(inventory)));
            assert(Get(inventory, inventoryKillIcon) == 600000007u);
        }

        Remember(13u, 0u);
        assert(!Sync(inventory.data(), Keys(inventory)));
        assert(Get(inventory, inventoryKillIcon) == 19u);
    }

    {
        Buffer inventory = Inventory(21, 22, 23, 0);
        Remember(22u, 600000031u);
        assert(Sync(inventory.data(), Keys(inventory)));
        assert(Get(inventory, inventoryKillIcon) == 600000031u);

        Remember(23u, 600000041u);
        assert(Sync(inventory.data(), Keys(inventory)));
        assert(Get(inventory, inventoryKillIcon) == 600000041u);

        Remember(22u, 0u);
        Remember(23u, 0u);
        assert(!Sync(inventory.data(), Keys(inventory)));
        assert(Get(inventory, inventoryKillIcon) == 0u);
    }

    {
        Buffer first = Inventory(31, 32, 33, 7);
        Buffer second = Inventory(41, 42, 43, 5);
        Remember(33u, 600000051u);
        Remember(43u, 600000061u);

        assert(Sync(first.data(), Keys(first)));
        assert(Get(first, inventoryKillIcon) == 600000051u);

        assert(Sync(second.data(), Keys(second)));
        assert(Get(second, inventoryKillIcon) == 600000061u);
        assert(Get(first, inventoryKillIcon) == 600000051u);
    }

    {
        Buffer inventory = Inventory(51, 52, 53, 3);
        Remember(53u, 600000071u);
        assert(Sync(inventory.data(), Keys(inventory)));
        Put(inventory, inventoryKillIcon, 9u);
        assert(Sync(inventory.data(), Keys(inventory)));
        assert(Get(inventory, inventoryKillIcon) == 600000071u);
        Remember(53u, 0u);
        assert(!Sync(inventory.data(), Keys(inventory)));
        assert(Get(inventory, inventoryKillIcon) == 9u);
    }

    {
        Buffer inventory = Inventory(61, 62, 63, 2);
        Remember(63u, 600000081u);
        assert(Sync(inventory.data(), Keys(inventory)));
        Forget();
        assert(Get(inventory, inventoryKillIcon) == 600000081u);
        Remember(63u, 0u);
        assert(!Sync(inventory.data(), Keys(inventory)));
        assert(Get(inventory, inventoryKillIcon) == 600000081u);
    }

    {
        std::array<unsigned char, 0x80> conf{};
        Put(conf, weaponAttrKillIcon, 600000091u);
        assert(Captured(conf.data()) == 600000091);
        Put(conf, weaponAttrKillIcon, 0x1234u);
        assert(Captured(conf.data()) == 0);
        assert(Captured(nullptr) == 0);
    }

    std::printf("kill icon checks passed\n");
    return 0;
}
