#include "Trial.h"

#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>
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

constexpr int MonthNumber(const char* date) noexcept {
    return date[0] == 'J' ? (date[1] == 'a' ? 1 : (date[2] == 'n' ? 6 : 7))
         : date[0] == 'F' ? 2
         : date[0] == 'M' ? (date[2] == 'r' ? 3 : 5)
         : date[0] == 'A' ? (date[1] == 'p' ? 4 : 8)
         : date[0] == 'S' ? 9
         : date[0] == 'O' ? 10
         : date[0] == 'N' ? 11 : 12;
}

constexpr unsigned DayOfMonth(const char* date) noexcept {
    return date[4] == ' ' ? static_cast<unsigned>(date[5] - '0')
                          : static_cast<unsigned>((date[4] - '0') * 10 + (date[5] - '0'));
}

constexpr int Year(const char* date) noexcept {
    return (date[7] - '0') * 1000 + (date[8] - '0') * 100 +
           (date[9] - '0') * 10 + (date[10] - '0');
}

constexpr std::uint64_t DaysFromCivil(int year, unsigned month, unsigned day) noexcept {
    year -= month <= 2u;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yearOfEra = static_cast<unsigned>(year - era * 400);
    const unsigned adjustedMonth = month > 2u ? month - 3u : month + 9u;
    const unsigned dayOfYear = (153u * adjustedMonth + 2u) / 5u + day - 1u;
    const unsigned dayOfEra = yearOfEra * 365u + yearOfEra / 4u - yearOfEra / 100u + dayOfYear;
    return static_cast<std::uint64_t>(era) * 146097u + dayOfEra - 719468u;
}

constexpr std::uint64_t BuildEpoch() noexcept {
    const char* time = __TIME__;
    const unsigned seconds = static_cast<unsigned>(time[0] - '0') * 36000u +
                             static_cast<unsigned>(time[1] - '0') * 3600u +
                             static_cast<unsigned>(time[3] - '0') * 600u +
                             static_cast<unsigned>(time[4] - '0') * 60u +
                             static_cast<unsigned>(time[6] - '0') * 10u +
                             static_cast<unsigned>(time[7] - '0');
    return DaysFromCivil(Year(__DATE__), MonthNumber(__DATE__), DayOfMonth(__DATE__)) *
               secondsPerDay + seconds;
}

const std::uint64_t buildEpoch = BuildEpoch();

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

struct State {
    std::uint64_t start = 0;
    std::uint64_t highWater = 0;
};

State LoadState() {
    State state;
    const auto path = StatePath();
    if (path.empty()) return state;
    const int descriptor = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (descriptor < 0) return state;
    char buffer[64]{};
    const auto count = ::read(descriptor, buffer, sizeof(buffer) - 1);
    ::close(descriptor);
    if (count <= 0) return state;
    unsigned long long start = 0;
    unsigned long long highWater = 0;
    if (std::sscanf(buffer, "%llu %llu", &start, &highWater) != 2) return {};
    state.start = start;
    state.highWater = highWater;
    return state;
}

bool SaveState(const State& state) {
    const auto path = StatePath();
    if (path.empty()) return false;
    const int descriptor = ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
    if (descriptor < 0) return false;
    char buffer[64];
    const int length = std::snprintf(buffer, sizeof(buffer), "%llu %llu",
        static_cast<unsigned long long>(state.start),
        static_cast<unsigned long long>(state.highWater));
    const bool written = length > 0 &&
        ::write(descriptor, buffer, static_cast<std::size_t>(length)) == length;
    ::close(descriptor);
    return written;
}

std::uint64_t LibraryMtime() {
    Dl_info info{};
    if (!dladdr(reinterpret_cast<void*>(&WallNow), &info) || !info.dli_fname) return 0;
    struct stat status{};
    if (::stat(info.dli_fname, &status) != 0) return 0;
    return status.st_mtime > 0 ? static_cast<std::uint64_t>(status.st_mtime) : 0;
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
    if (expiryOverride) {
        config.window = {buildEpoch, expiryOverride};
        highWater.store(wall, std::memory_order_relaxed);
        persisted.store(wall, std::memory_order_relaxed);
        return config;
    }
    auto state = LoadState();
    bool stored = state.start != 0;
    if (!stored) {
        const State fresh{wall, wall};
        stored = SaveState(fresh);
        if (stored) state = fresh;
        else state = {wall, wall};
    }
    std::uint64_t anchor = buildEpoch;
    if (stored) {
        anchor = state.start;
    } else {
        const auto mtime = LibraryMtime();
        if (mtime > anchor) anchor = mtime;
    }
    config.window = MakeWindow(buildEpoch, anchor);
    const auto high = state.highWater > wall ? state.highWater : wall;
    highWater.store(high, std::memory_order_relaxed);
    persisted.store(state.highWater, std::memory_order_relaxed);
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
                persisted.store(now, std::memory_order_relaxed);
                SaveState({config.window.start, now});
            }
        }
    }
    return Within(config.window, now);
}

void Crash() noexcept {
    __builtin_trap();
}

}
