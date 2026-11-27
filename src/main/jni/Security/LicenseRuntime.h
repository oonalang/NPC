#pragma once
#include "Lease.h"

namespace license {
inline constexpr bool isLogin = true;
enum class Failure { None, Transient, Rejected };
struct Response {
    Failure failure = Failure::Rejected;
    std::string payload, signature, error;
};
using Transport = Response(*)(const Request&);
std::string Authenticate(const std::string& key, const std::string& serial, Transport transport);
std::uint64_t LicenseUntil() noexcept;
std::string LastError();
}
