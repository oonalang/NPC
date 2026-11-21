#pragma once

#include <cstdint>

namespace PhantomTrigger {

enum class Mode : std::uint8_t { Both, ADSOnly };


enum class FireMode : std::uint8_t {
    Unsupported,
    Automatic,
    Single,
    BurstAutomatic,
    BurstSingle,
    SingleRelease
};

struct Settings {
    bool enabled = false;
    Mode mode = Mode::Both;
    bool showDot = false;
};


struct ContextIdentity {
    std::uint64_t match = 0;
    std::uint64_t pawn = 0;
    std::uint64_t weapon = 0;
    std::uint64_t component = 0;
    std::uint64_t inputMode = 0;

    constexpr bool operator==(const ContextIdentity& other) const noexcept {
        return match == other.match && pawn == other.pawn &&
               weapon == other.weapon && component == other.component &&
               inputMode == other.inputMode;
    }

    constexpr bool operator!=(const ContextIdentity& other) const noexcept {
        return !(*this == other);
    }
};

struct Frame {
    ContextIdentity identity{};
    std::uint64_t updateId = 0;
    bool contextValid = false;
    bool bindingsReady = false;

    bool manualActive = false;
    bool aiming = false;
    bool fullyAimed = false;
    bool ammoAvailable = false;
    bool hardBlocked = false;

    bool targetValid = false;
    bool shotReady = false;
    std::uint64_t shotSerial = 0;
    FireMode fireMode = FireMode::Unsupported;
    std::uint64_t nowMillis = 0;
    int clipAmmo = -1;
};

struct Intent {
    bool held = false;
    bool down = false;
    bool up = false;

    bool passthrough = true;
    bool commitRelease = false;
};



class Controller {
public:
    static constexpr std::uint64_t shotAcknowledgementTimeoutMillis = 500;
    bool HasOwnedInput() const noexcept { return phase_ == Phase::Held; }
    bool HasOwnedInputFor(const ContextIdentity& identity) const noexcept {
        return hasContext_ && identity_ == identity && HasOwnedInput();
    }
    bool HasPendingShot() const noexcept { return phase_ == Phase::AwaitShot; }
    bool IsRearming() const noexcept { return phase_ == Phase::Rearm || phase_ == Phase::RejectedRelease; }
    bool NeedsFreshTarget(const Frame& frame) const noexcept {
        if (!hasContext_ || identity_ != frame.identity) return true;
        if (hasUpdate_ && frame.updateId <= lastUpdate_) return false;
        if (phase_ == Phase::RejectedRelease) return false;
        if (phase_ == Phase::AwaitShot) return ShotObserved(frame);
        return !NeedsPressCleanup(frame);
    }
    void RejectPendingShot() noexcept {
        if (phase_ == Phase::AwaitShot) phase_ = Phase::RejectedRelease;
    }
    void Reset() noexcept {
        *this = Controller{};
    }

    Intent Step(const Settings& settings, const Frame& frame) noexcept {
        if (!hasContext_ || identity_ != frame.identity) {

            Reset();
            hasContext_ = true;
            identity_ = frame.identity;
            mode_ = settings.mode;
            fireMode_ = frame.fireMode;
        } else if (hasUpdate_ && frame.updateId <= lastUpdate_) {

            return {};
        }

        hasUpdate_ = true;
        lastUpdate_ = frame.updateId;

        const bool modeChanged = mode_ != settings.mode ||
                                 fireMode_ != frame.fireMode;
        mode_ = settings.mode;
        fireMode_ = frame.fireMode;

        if (frame.manualActive) {

            phase_ = Phase::Idle;
            return {};
        }

        if (phase_ == Phase::RejectedRelease) {
            phase_ = Phase::Rearm;
            return {};
        }

        if (phase_ == Phase::AwaitShot) {
            if (ShotObserved(frame)) {
                phase_ = Phase::Rearm;
            } else if (AcknowledgementExpired(frame)) {
                phase_ = Phase::Rearm;
                return {false, false, false, false};
            } else return {};
        }

        if (NeedsPressCleanup(frame)) {
            phase_ = Phase::Rearm;
            return {false, false, true, false};
        }

        if ((modeChanged && IsOwned()) || !Eligible(settings, frame)) {
            return ReleaseOwned();
        }

        if (IsAutomatic(frame.fireMode)) {
            const bool wasHeld = IsOwned();
            if (!wasHeld && !frame.shotReady) return {};
            phase_ = Phase::Held;


            return {true, !wasHeld, false, false};
        }

        if (frame.fireMode == FireMode::SingleRelease && phase_ == Phase::Held) {
            if (!frame.shotReady)
                return {true, false, false, false};
            RememberSubmission(frame);
            phase_ = Phase::AwaitShot;
            return {false, false, true, false, true};
        }

        if (phase_ == Phase::Held) {
            return {true, false, false, false};
        }



        if (phase_ == Phase::Rearm) {
            phase_ = Phase::Idle;
        }
        if (!frame.shotReady) {
            return {};
        }

        RememberSubmission(frame);
        phase_ = Phase::Held;
        return {true, true, false, false};
    }

private:
    enum class Phase : std::uint8_t { Idle, Held, AwaitShot, RejectedRelease, Rearm };

    bool NeedsPressCleanup(const Frame& frame) const noexcept {
        return phase_ == Phase::Held &&
            (frame.fireMode == FireMode::Single || frame.fireMode == FireMode::BurstSingle) &&
            (ShotObserved(frame) || AcknowledgementExpired(frame));
    }

    bool ShotObserved(const Frame& frame) const noexcept {
        return frame.shotSerial != submittedShotSerial_ ||
            (submittedClipAmmo_ >= 0 && frame.clipAmmo >= 0 && frame.clipAmmo < submittedClipAmmo_);
    }
    bool AcknowledgementExpired(const Frame& frame) const noexcept {
        return frame.nowMillis >= submittedMillis_ &&
            frame.nowMillis - submittedMillis_ >= shotAcknowledgementTimeoutMillis;
    }
    void RememberSubmission(const Frame& frame) noexcept {
        submittedShotSerial_ = frame.shotSerial;
        submittedMillis_ = frame.nowMillis;
        submittedClipAmmo_ = frame.clipAmmo;
    }

    static bool IsAutomatic(FireMode mode) noexcept {
        return mode == FireMode::Automatic || mode == FireMode::BurstAutomatic;
    }

    static bool IsSupported(FireMode mode) noexcept {
        return IsAutomatic(mode) || mode == FireMode::Single ||
               mode == FireMode::BurstSingle || mode == FireMode::SingleRelease;
    }

    static bool Eligible(const Settings& settings, const Frame& frame) noexcept {
        const bool validMode = settings.mode == Mode::Both ||
                               settings.mode == Mode::ADSOnly;
        const bool adsReady = (settings.mode == Mode::Both ||
                               (frame.aiming && frame.fullyAimed)) &&
                              (frame.fireMode != FireMode::SingleRelease ||
                               (frame.aiming && frame.fullyAimed));
        return settings.enabled && validMode && frame.contextValid &&
               frame.bindingsReady && frame.ammoAvailable && !frame.hardBlocked &&
               frame.targetValid && IsSupported(frame.fireMode) && adsReady;
    }

    bool IsOwned() const noexcept {
        return phase_ == Phase::Held;
    }

    Intent ReleaseOwned() noexcept {
        const bool wasHeld = IsOwned();
        phase_ = Phase::Idle;
        return wasHeld ? Intent{false, false, true, false} : Intent{};
    }

    ContextIdentity identity_{};
    std::uint64_t lastUpdate_ = 0;
    std::uint64_t submittedShotSerial_ = 0;
    std::uint64_t submittedMillis_ = 0;
    int submittedClipAmmo_ = -1;
    Mode mode_ = Mode::Both;
    FireMode fireMode_ = FireMode::Unsupported;
    Phase phase_ = Phase::Idle;
    bool hasContext_ = false;
    bool hasUpdate_ = false;
};

}
