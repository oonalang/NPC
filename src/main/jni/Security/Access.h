#pragma once
#include <cstdint>
#include <type_traits>

namespace license {
inline bool isLogin = false;
enum class Capability : std::uint32_t { Menu = 1, Features = 2, Premium = 4 };
inline constexpr std::uint32_t allCapabilities = 7;
bool Allows(Capability capability) noexcept;
std::uint64_t BootMillis() noexcept;

template<auto Handler, auto Original> struct Guard;
template<class R, class... Args, R(*Handler)(Args...), R(**Original)(Args...)>
struct Guard<Handler, Original> {
    static R Invoke(Args... args) {
        if (*Original) {
            if (Allows(Capability::Features)) return Handler(args...);
            return (*Original)(args...);
        }
        if constexpr (!std::is_void_v<R>) return R{};
    }
};
}
