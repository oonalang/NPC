#include "Trial.h"

#include <dlfcn.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <mutex>
#include <string>

namespace license::trial {
namespace {

constexpr std::uint64_t expiryOverride = 0;
constexpr std::uint64_t persistInterval = 300;

std::uint64_t WallNow() noexcept {
    const std::time_t value = std::time(nullptr);
    return value > 0 ? static_cast<std::uint64_t>(value) : 0;
}

std::uint64_t BootNow() noexcept {
    timespec now{};
    if (::clock_gettime(CLOCK_BOOTTIME, &now) != 0 || now.tv_sec < 0) return 0;
    return static_cast<std::uint64_t>(now.tv_sec);
}

std::string LibraryDirectory() {
    Dl_info info{};
    if (!dladdr(reinterpret_cast<void*>(&WallNow), &info) || !info.dli_fname) return {};
    const std::string path(info.dli_fname);
    const auto slash = path.find_last_of('/');
    if (slash == std::string::npos) return {};
    return path.substr(0, slash);
}

std::string StatePath() {
    static const std::string path = [] {
        const auto directory = LibraryDirectory();
        if (!directory.empty() && ::access(directory.c_str(), W_OK) == 0)
            return directory + "/.trial-state";
        const char* temporary = std::getenv("TMPDIR");
        if (temporary && *temporary) return std::string(temporary) + "/.trial-state";
        return std::string{};
    }();
    return path;
}

std::string BackupPath() {
    static const std::string path = [] {
        const auto directory = LibraryDirectory();
        if (!directory.empty() && ::access(directory.c_str(), W_OK) == 0)
            return directory + "/.trial-hw";
        const char* temporary = std::getenv("TMPDIR");
        if (temporary && *temporary) return std::string(temporary) + "/.trial-hw";
        return std::string{};
    }();
    return path;
}

struct State {
    std::uint64_t start = 0;
    std::uint64_t highWater = 0;
};

State LoadState() {
    State state;
    auto readPair = [&](const std::string& path) {
        if (path.empty()) return;
        const int descriptor = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
        if (descriptor < 0) return;
        char buffer[64]{};
        const auto count = ::read(descriptor, buffer, sizeof(buffer) - 1);
        ::close(descriptor);
        if (count <= 0) return;
        unsigned long long start = 0;
        unsigned long long highWater = 0;
        if (std::sscanf(buffer, "%llu %llu", &start, &highWater) == 2) {
            if (start > state.start) state.start = start;
            if (highWater > state.highWater) state.highWater = highWater;
        }
    };
    readPair(StatePath());
    readPair(BackupPath());
    return state;
}

bool SaveState(const State& state) {
    const auto path = StatePath();
    const auto backup = BackupPath();
    bool written = false;
    auto writePair = [&](const std::string& target) {
        if (target.empty()) return;
        const int descriptor = ::open(target.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
        if (descriptor < 0) return;
        char buffer[64];
        const int length = std::snprintf(buffer, sizeof(buffer), "%llu %llu",
            static_cast<unsigned long long>(state.start),
            static_cast<unsigned long long>(state.highWater));
        if (length > 0 && ::write(descriptor, buffer, static_cast<std::size_t>(length)) == length)
            written = true;
        ::close(descriptor);
    };
    writePair(path);
    writePair(backup);
    return written;
}

struct Config {
    Window window{};
    std::uint64_t sessionWall = 0;
    std::uint64_t sessionBoot = 0;
};

std::atomic<std::uint64_t> highWater{0};
std::atomic<std::uint64_t> persisted{0};
std::mutex persistMutex;

Config Build() {
    Config config;
    const auto wall = WallNow();
    const auto boot = BootNow();
    config.sessionWall = wall;
    config.sessionBoot = boot;
    if (wall == 0 || boot == 0) return config;
    if (expiryOverride) {
        config.window = {wall, expiryOverride};
        highWater.store(wall, std::memory_order_relaxed);
        persisted.store(wall, std::memory_order_relaxed);
        return config;
    }
    const auto state = LoadState();
    const auto start = FirstRunStart(state.start, state.highWater, wall);
    const auto high = state.highWater > wall ? state.highWater : wall;
    if (!SaveState({start, high})) return config;
    config.window = {start, start + windowSeconds};
    highWater.store(high, std::memory_order_relaxed);
    persisted.store(high, std::memory_order_relaxed);
    return config;
}

Config& Current() {
    static Config config = Build();
    return config;
}

}

std::uint64_t Expiry() noexcept {
    return Current().window.expiry;
}

bool Active() noexcept {
    auto& config = Current();
    if (config.window.start == 0) return false;
    const auto wall = WallNow();
    const auto boot = BootNow();
    const auto high = highWater.load(std::memory_order_relaxed);
    const auto now = EffectiveNow(wall, boot, config.sessionWall, config.sessionBoot, high);
    if (now > high) {
        highWater.store(now, std::memory_order_relaxed);
        const auto saved = persisted.load(std::memory_order_relaxed);
        if (now >= saved + persistInterval) {
            std::lock_guard<std::mutex> lock(persistMutex);
            if (now >= persisted.load(std::memory_order_relaxed) + persistInterval) {
                if (!SaveState({config.window.start, now})) return false;
                persisted.store(now, std::memory_order_relaxed);
            }
        }
    }
    return Within(config.window, now);
}


}
