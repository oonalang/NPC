#include "../src/main/jni/Security/Trial.h"

#include <cstdio>

using license::trial::EffectiveNow;
using license::trial::MakeWindow;
using license::trial::Within;
using license::trial::windowSeconds;

namespace {

constexpr std::uint64_t build = 1794879171;
constexpr std::uint64_t day = 86400;

unsigned checks = 0;
unsigned failures = 0;

void Require(bool condition, const char* message) {
    ++checks;
    if (!condition) {
        std::printf("FAIL %s\n", message);
        ++failures;
    }
}

void WindowLength() {
    const auto window = MakeWindow(build, build);
    Require(window.start == build, "an on-time build starts at its build stamp");
    Require(window.expiry == build + windowSeconds, "the window lasts exactly three days");
    Require(windowSeconds == 3 * day, "three days is 259200 seconds");
}

void Boundary() {
    const auto window = MakeWindow(build, build + 30 * day);
    Require(window.start == build + 30 * day, "a stale build waits for its first run");
    Require(window.expiry == window.start + windowSeconds, "the first run opens a full window");
    Require(!Within(window, window.start - 1), "the window excludes the second before it opens");
    Require(Within(window, window.start), "the window includes its opening second");
    Require(Within(window, window.start + 1), "the window includes the second after it opens");
    Require(Within(window, window.start + windowSeconds - 1), "the window includes its final second");
    Require(!Within(window, window.start + windowSeconds), "the window excludes its expiry second");
    Require(!Within(window, window.start + windowSeconds + day), "the window stays closed afterwards");
}

void StaleBuild() {
    const std::uint64_t firstRun = build + 200 * day;
    const auto window = MakeWindow(build, firstRun);
    Require(Within(window, firstRun), "a build made months ago is usable on its first run");
    Require(Within(window, firstRun + 2 * day), "a stale build gets the full trial");
    Require(!Within(window, firstRun + 4 * day), "a stale build still expires after three days");
}

void MissingEmulation() {
    const auto window = MakeWindow(build, 0);
    Require(window.start == build, "a build without an install anchor cannot start early");
    Require(window.expiry == build + windowSeconds, "a build without an anchor keeps three days");
    Require(!Within(window, build - 1), "an anchored fallback rejects time before the build");
}

void Override() {
    const std::uint64_t pinned = build + 7 * day;
    const auto window = MakeWindow(build, pinned);
    Require(Within(window, pinned), "an explicit expiry anchors at its first run");
    Require(!Within(window, pinned + windowSeconds), "an explicit expiry still bounds the trial");
    const license::trial::Window dead{build, build - day};
    Require(!Within(dead, build), "an expiry before the build stamp is dead on arrival");
    Require(!Within(dead, build - day), "an expiry before the build stamp never opens");
}

void ClockRollback() {
    const std::uint64_t sessionWall = 1794879171;
    const std::uint64_t sessionBoot = 500;
    Require(EffectiveNow(sessionWall + 60, sessionBoot + 60, sessionWall, sessionBoot, 0) ==
                sessionWall + 60, "a forward clock advances the trial");
    Require(EffectiveNow(sessionWall - 5000, sessionBoot + 60, sessionWall, sessionBoot, 0) ==
                sessionWall + 60, "a rolled-back wall clock cannot rewind the trial");
    Require(EffectiveNow(sessionWall - 5000, sessionBoot + day, sessionWall, sessionBoot, 0) ==
                sessionWall + day, "boot time keeps counting through a clock rollback");
    Require(EffectiveNow(sessionWall - 5000, sessionBoot + 60, sessionWall, sessionBoot,
                sessionWall + 4000) == sessionWall + 4000,
            "a recorded high-water mark outranks a rolled-back clock");
}

void RestartRollback() {
    const std::uint64_t storedHigh = build + 3 * day + 200;
    const std::uint64_t sessionWall = storedHigh - 1000;
    Require(EffectiveNow(sessionWall, 50, sessionWall, 50, storedHigh) == storedHigh,
            "a restart cannot roll the trial back below the stored high-water mark");
    Require(!Within(MakeWindow(build, build), EffectiveNow(sessionWall, 50, sessionWall, 50, storedHigh)),
            "the stored high-water mark can push a trial past its expiry");
}

}

int main() {
    WindowLength();
    Boundary();
    StaleBuild();
    MissingEmulation();
    Override();
    ClockRollback();
    RestartRollback();
    if (failures) {
        std::printf("%u of %u trial window checks failed\n", failures, checks);
        return 1;
    }
    std::printf("Passed %u trial window checks\n", checks);
    return 0;
}
