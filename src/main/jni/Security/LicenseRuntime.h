#pragma once
#include "Lease.h"

namespace license {
enum class Failure { None, Transient, Rejected };
struct Response {
    Failure failure = Failure::Rejected;
    std::string payload, signature, error;
    bool osn = false;
    std::string osnToken{};
    std::uint64_t osnIssued = 0, osnLicenseUntil = 0;
};
using Transport = Response(*)(const Request&);
std::string Authenticate(const std::string& key, const std::string& serial, Transport transport);
std::uint64_t LicenseUntil() noexcept;
std::string LastError();
}
