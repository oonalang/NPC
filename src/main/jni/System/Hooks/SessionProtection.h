#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <limits>

namespace session_protection {
inline constexpr unsigned maximumAttempts = 3;
inline constexpr std::array<std::uint64_t, 2> retryDelaysMs{1000, 3000};
inline constexpr bool unobservedReleaseValue = false;
struct Preference { bool enabled; std::uint64_t revision; };
inline std::atomic<std::uint64_t> preference{0};
inline void Publish(bool enabled) noexcept {
    auto previous = preference.load(std::memory_order_relaxed);
    while (bool(previous & 1) != enabled && !preference.compare_exchange_weak(previous,
        ((previous + 2) & ~std::uint64_t{1}) | std::uint64_t(enabled), std::memory_order_release,
        std::memory_order_relaxed)) {}
}
inline Preference Read() noexcept {
    const auto value = preference.load(std::memory_order_acquire);
    return {bool(value & 1), value >> 1};
}

enum class State { Off, WaitingForLogin, Pending, Requested, Restored, Unconfirmed, Failed };
enum class Backend { Waiting, Ready, Unsupported, HookFailed };
enum class Outcome { Queued, Rejected, Unconfirmed };
inline std::atomic<State> reported{State::Off};
inline std::atomic<Backend> backend{Backend::Waiting};
inline std::atomic<bool> available{false};
inline const char* StatusText() noexcept {
    const auto compatibility = backend.load();
    if (compatibility == Backend::Unsupported) return "Unavailable for this game build";
    if (compatibility == Backend::HookFailed) return "Session controls unavailable";
    if (compatibility != Backend::Ready) return "Waiting for game session controls";
    switch (reported.load()) {
        case State::Off: return Read().enabled ? "Waiting for CODM login" : "Off";
        case State::WaitingForLogin: return "Waiting for CODM login";
        case State::Pending: return "Session request pending";
        case State::Requested: return "Protection request sent (server decides)";
        case State::Restored: return "Restore request sent";
        case State::Unconfirmed: return "Could not confirm the session request";
        case State::Failed: return "Session request failed; toggle to retry";
    }
    return "Session controls unavailable";
}

struct Identity {
    std::uintptr_t handler = 0, connector = 0;
    std::uint64_t generation = 0;
    bool Valid() const noexcept { return handler && connector && generation; }
    bool operator==(const Identity& other) const noexcept {
        return handler == other.handler && connector == other.connector && generation == other.generation;
    }
    bool operator!=(const Identity& other) const noexcept { return !(*this == other); }
};
struct Action {
    bool valid = false, value = false;
    std::uint64_t serial = 0;
    Identity identity{};
};

class Controller {
    Identity identity_{};
    bool baselineKnown_ = false, baseline_ = unobservedReleaseValue, override_ = false, dirty_ = false;
    bool wanted_ = false;
    std::uint64_t revision_ = std::numeric_limits<std::uint64_t>::max(), serial_ = 0, due_ = 0;
    unsigned attempts_ = 0;
    State state_ = State::Off;

    void Result(bool value, Outcome result, std::uint64_t now) noexcept {
        if (result == Outcome::Rejected) {
            if (attempts_ >= maximumAttempts) { dirty_ = false; state_ = State::Failed; }
            else {
                dirty_ = true;
                const unsigned index = attempts_ ? attempts_ - 1 : 0;
                due_ = now + retryDelaysMs[index < retryDelaysMs.size() ? index : retryDelaysMs.size() - 1];
                state_ = State::Pending;
            }
            return;
        }
        if (result == Outcome::Queued) override_ = value != baseline_;
        else if (value != baseline_) override_ = true;
        dirty_ = false;
        state_ = result == Outcome::Unconfirmed ? State::Unconfirmed : wanted_ ? State::Requested : State::Restored;
    }
public:
    State Status() const noexcept { return state_; }
    bool HasBaseline() const noexcept { return baselineKnown_; }
    bool NeedsAttention() const noexcept { return override_ || dirty_; }
    bool Active() const noexcept { return identity_.Valid(); }
    Identity Context() const noexcept { return identity_; }
    void ObserveGameRequest(bool value) noexcept {
        const bool changed = baselineKnown_ && value != baseline_;
        baseline_ = value;
        baselineKnown_ = true;
        if (changed && !wanted_ && override_) { dirty_ = true; attempts_ = 0; due_ = 0; ++serial_; }
    }
    void Disconnected(bool enabled) noexcept {
        identity_ = {};
        baselineKnown_ = false;
        baseline_ = unobservedReleaseValue;
        override_ = dirty_ = false;
        attempts_ = 0;
        ++serial_;
        state_ = enabled ? State::WaitingForLogin : State::Off;
    }
    Action Step(Identity identity, bool authenticated, Preference setting, std::uint64_t now) noexcept {
        if (!authenticated || !identity.Valid()) { Disconnected(setting.enabled); return {}; }
        if (identity != identity_) {
            if (identity_.Valid()) { baselineKnown_ = false; baseline_ = unobservedReleaseValue; }
            identity_ = identity;
            override_ = false;
            revision_ = std::numeric_limits<std::uint64_t>::max();
            ++serial_;
        }
        if (revision_ != setting.revision) {
            revision_ = setting.revision;
            wanted_ = setting.enabled;
            dirty_ = wanted_ || override_;
            attempts_ = 0;
            due_ = now;
            ++serial_;
        }
        if (!wanted_ && !override_) { dirty_ = false; state_ = State::Off; return {}; }
        if (!dirty_ || now < due_) return {};
        ++attempts_;
        state_ = State::Pending;
        return {true, wanted_ || baseline_, ++serial_, identity_};
    }
    void Complete(Action action, Outcome result, std::uint64_t now) noexcept {
        if (!action.valid || action.identity != identity_ || action.serial != serial_) return;
        Result(action.value, result, now);
    }
    void NaturalResult(Preference setting, bool effective, Outcome result, std::uint64_t now) noexcept {
        if (!Active()) return;
        wanted_ = setting.enabled;
        if (revision_ != setting.revision) attempts_ = 0;
        revision_ = setting.revision;
        ++serial_;
        if (!wanted_ && !override_) { dirty_ = false; state_ = State::Off; return; }
        if (!attempts_) attempts_ = 1;
        Result(effective, result, now);
    }
};

using HookInstaller = int(*)(void*, void*, void**);
bool Install(std::uintptr_t base, HookInstaller install) noexcept;
}
