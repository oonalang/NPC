#include "Trial.h"

#include <ctime>

namespace license::trial {
namespace {

constexpr std::uint64_t secondsPerDay = 86400;
constexpr std::uint64_t expiryOverride = 0;

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

}

std::uint64_t Expiry() noexcept {
    if (expiryOverride) return expiryOverride;
    return buildEpoch + static_cast<std::uint64_t>(freeDays) * secondsPerDay;
}

bool Active() noexcept {
    const std::time_t wall = std::time(nullptr);
    if (wall <= 0) return false;
    const auto now = static_cast<std::uint64_t>(wall);
    return now >= buildEpoch && now < Expiry();
}

void Crash() noexcept {
    __builtin_trap();
}

}
