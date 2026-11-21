#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <elf.h>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <vector>

struct Vector2 { float x, y; };
struct Vector3 { float x, y, z; };
#include "../src/main/jni/System/Core/SaveConfig.h"

float menu[4]{.6f, .4f, .9f, 1.f};
bool isJumpAdjustmentEnabled = false, SnowB = false, isSpeedHackEnabled = false;
float jumpHeightMultiplier = 1.f, SnowBsize = 0.f, speedHackMultiplier = 1.f;

using Manager = std::array<unsigned char, 0x80>;

static void Float(Manager& manager, std::size_t offset, float value) {
    std::memcpy(manager.data() + offset, &value, sizeof(value));
}

static Manager Stock() {
    Manager manager{};
    manager.fill(0x5a);
    Float(manager, pickup::scanIntervalOffset, .4f);
    Float(manager, pickup::horizontalRangeOffset, 4.f);
    Float(manager, pickup::verticalRangeOffset, 1.5f);
    return manager;
}

static void Overrides() {
    using namespace pickup;
    auto manager = Stock();
    const auto before = manager;
    bool reachable = false;
    const auto checkDistance = [&] { reachable = 9.f <= Field<float>(manager.data(), horizontalRangeOffset); };
    Invoke(manager.data(), {}, checkDistance);
    assert(!reachable);
    Invoke(manager.data(), {false, 2.f}, checkDistance);
    assert(reachable && manager == before);
    Invoke(manager.data(), {}, checkDistance);
    assert(!reachable);
    int calls = 0;
    for (float range : {1.f, 1.25f, 2.f, 3.f}) {
        for (bool fast : {false, true}) {
            for (int repeat = 0; repeat < 3; ++repeat) {
                Invoke(manager.data(), {fast, range}, [&] {
                    ++calls;
                    assert(std::fabs(Field<float>(manager.data(), horizontalRangeOffset) - 4.f * range * range) < .00001f);
                    assert(std::fabs(Field<float>(manager.data(), verticalRangeOffset) - 1.5f * range) < .00001f);
                    assert(Field<float>(manager.data(), scanIntervalOffset) == (fast ? fastInterval : .4f));
                    assert(std::memcmp(manager.data(), before.data(), scanIntervalOffset) == 0);
                    assert(std::memcmp(manager.data() + verticalRangeOffset + 4,
                        before.data() + verticalRangeOffset + 4, manager.size() - verticalRangeOffset - 4) == 0);
                });
                assert(manager == before && !activeManager);
            }
        }
    }
    assert(calls == 24);
    auto other = Stock();
    Invoke(manager.data(), {true, 2.f}, [&] {
        Invoke(manager.data(), {true, 3.f}, [&] {
            assert(Field<float>(manager.data(), horizontalRangeOffset) == 16.f);
        });
        Invoke(other.data(), {true, 3.f}, [&] { assert(other == before); });
        assert(activeManager == manager.data());
    });
    assert(manager == before && other == before && !activeManager);
    try {
        Invoke(manager.data(), {true, 2.f}, [] { throw std::runtime_error("fixture"); });
        assert(false);
    } catch (const std::runtime_error&) {
        assert(manager == before && !activeManager);
    }
    Invoke(manager.data(), {true, 2.f}, [&] {
        Float(manager, horizontalRangeOffset, 12.f);
        Float(manager, scanIntervalOffset, .8f);
    });
    assert(Field<float>(manager.data(), horizontalRangeOffset) == 12.f);
    assert(Field<float>(manager.data(), scanIntervalOffset) == .8f);
    assert(Field<float>(manager.data(), verticalRangeOffset) == 1.5f);
    Invoke(nullptr, {true, 3.f}, [&] { ++calls; });
    assert(calls == 25);

    const float huge = std::numeric_limits<float>::max();
    assert(ScaleLimit(huge, 9.f) == huge && ScaleLimit(-1.f, 9.f) == -1.f);
    assert(ScaleLimit(0.f, 9.f) == 0.f && std::isnan(ScaleLimit(NAN, 9.f)));
    assert(PickupInterval(.01f, true) == .01f && PickupInterval(0.f, true) == 0.f);
    assert(PickupInterval(-1.f, true) == -1.f && std::isnan(PickupInterval(NAN, true)));
    assert(PickupInterval(.4f, false) == .4f);
    assert(NormalizeRange(-2.f) == 1.f && NormalizeRange(99.f) == 3.f);
    assert(NormalizeRange(NAN) == 1.f && NormalizeRange(INFINITY) == 1.f);
    Publish(true, 2.37f);
    assert(Read().fast && Read().range == 2.37f);
    Publish(false, INFINITY);
    assert(!Read().Active() && Read().range == 1.f);
    std::puts("Pickup overrides passed for squared radius, timing, restoration, nested calls and invalid limits");
}

static void Configuration(const char* directory) {
    androidFilesDir = std::string(directory) + "/";
    Config.ExtraMenu.FastPickup = true;
    Config.ExtraMenu.PickupRange = 2.4f;
    SaveConfiguration("pickup");
    Config.ExtraMenu.FastPickup = false;
    Config.ExtraMenu.PickupRange = 1.f;
    assert(LoadConfiguration("pickup"));
    assert(Config.ExtraMenu.FastPickup && Config.ExtraMenu.PickupRange == 2.4f);
    assert(pickup::Read().fast && pickup::Read().range == 2.4f);
    nlohmann::json stored;
    { std::ifstream file(androidFilesDir + "configs/pickup.json"); file >> stored; }
    auto write = [&] {
        std::ofstream file(androidFilesDir + "configs/legacy.json");
        file << stored;
    };
    stored["ExtraMenu"].erase("FastPickup");
    stored["ExtraMenu"].erase("PickupRange");
    write();
    assert(LoadConfiguration("legacy") && !Config.ExtraMenu.FastPickup && Config.ExtraMenu.PickupRange == 1.f);
    stored["ExtraMenu"]["FastPickup"] = "yes";
    stored["ExtraMenu"]["PickupRange"] = "far";
    write();
    assert(LoadConfiguration("legacy") && !pickup::Read().Active());
    stored["ExtraMenu"]["FastPickup"] = true;
    stored["ExtraMenu"]["PickupRange"] = 99.f;
    write();
    assert(LoadConfiguration("legacy") && pickup::Read().fast && pickup::Read().range == 3.f);
    stored["ExtraMenu"]["PickupRange"] = -2.f;
    write();
    assert(LoadConfiguration("legacy") && pickup::Read().range == 1.f);
    stored.erase("ExtraMenu");
    write();
    assert(LoadConfiguration("legacy") && !pickup::Read().Active());
    Config.ExtraMenu.PickupRange = INFINITY;
    SaveConfiguration("invalid");
    assert(LoadConfiguration("invalid") && Config.ExtraMenu.PickupRange == 1.f);
    std::puts("Pickup configuration passed for round trips, legacy defaults, malformed values and range bounds");
}

static void Profile(const char* path) {
    std::ifstream source(path, std::ios::binary);
    assert(source);
    Elf64_Ehdr header{};
    source.read(reinterpret_cast<char*>(&header), sizeof(header));
    assert(source && std::memcmp(header.e_ident, ELFMAG, SELFMAG) == 0);
    assert(header.e_ident[EI_CLASS] == ELFCLASS64 && header.e_machine == EM_AARCH64);
    assert(header.e_phentsize == sizeof(Elf64_Phdr) && header.e_phnum > 0 && header.e_phnum <= 512);
    std::vector<Elf64_Phdr> segments(header.e_phnum);
    source.seekg(header.e_phoff);
    source.read(reinterpret_cast<char*>(segments.data()), segments.size() * sizeof(Elf64_Phdr));
    assert(source);
    bool buildMatches = false;
    for (const auto& segment : segments) {
        if (segment.p_type != PT_NOTE || segment.p_filesz > 4096) continue;
        std::vector<unsigned char> bytes(segment.p_filesz);
        source.seekg(segment.p_offset);
        source.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
        assert(source);
        buildMatches = buildMatches || PhantomTrigger::NativeProfileDetail::MatchesBuildIdNote(bytes.data(), bytes.size());
    }
    assert(buildMatches);
    for (const auto& check : pickup::profile) {
        bool matched = false;
        for (const auto& segment : segments) {
            if (segment.p_type != PT_LOAD || (segment.p_flags & (PF_R | PF_X)) != (PF_R | PF_X) ||
                check.rva < segment.p_vaddr || check.rva - segment.p_vaddr + check.expected.size() > segment.p_filesz) continue;
            std::array<unsigned char, 16> bytes{};
            source.seekg(segment.p_offset + check.rva - segment.p_vaddr);
            source.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
            assert(source && bytes == check.expected);
            matched = true;
            break;
        }
        assert(matched);
    }
    std::printf("Pinned ARM64 build ID and all %zu pickup fingerprints passed\n", pickup::profile.size());
}

int main(int argc, char** argv) {
    assert(argc == 2 || argc == 3);
    Overrides();
    Configuration(argv[1]);
    if (argc == 3) Profile(argv[2]);
}
