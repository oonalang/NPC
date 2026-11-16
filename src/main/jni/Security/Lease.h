#pragma once
#include "Access.h"
#include "../Includes/PanelLicense.h"
#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>

namespace license {
inline constexpr std::uint64_t leaseSeconds = 180, refreshSeconds = 60, retrySeconds = 10;
inline constexpr std::uint64_t clockSkewSeconds = 30, requestTimeoutMillis = 30000;
inline constexpr char audience[] = "CODMGR", build[] = "1";
struct Request {
    std::string key, serial, nonce;
    std::uint64_t wall = 0, tick = 0, generation = 0;
};
struct Claims {
    std::uint64_t issued = 0, until = 0, licenseUntil = 0;
    std::uint32_t capabilities = 0;
};
inline bool Hex(const std::string& value) noexcept {
    if (value.size() != 64) return false;
    for (unsigned char c : value) if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    return true;
}
inline bool Credentials(const std::string& key, const std::string& serial) noexcept {
    if (key.empty() || key.size() > 128 || serial.size() < 8 || serial.size() > 64) return false;
    for (unsigned char c : key) if (c < 32 || c == 127 || c == '|') return false;
    for (unsigned char c : serial)
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-')) return false;
    return true;
}
inline bool Verify(const Request& request, const std::string& payload, const std::string& signature,
                   std::uint64_t wall, std::uint64_t tick, Claims& output,
                   const char* publicKey = PanelLicense::kPublicKey, const char* keyId = PanelLicense::kKeyId) {
    if (!publicKey || !keyId || !Credentials(request.key,request.serial) || !Hex(request.nonce) ||
        !request.wall || !request.tick || !request.generation || !wall || tick < request.tick ||
        tick - request.tick > requestTimeoutMillis || payload.size() > 1024) return false;
    std::array<std::string,11> parts;
    std::size_t start = 0;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        const auto end = payload.find('|',start);
        if ((i+1 < parts.size()) != (end != std::string::npos)) return false;
        parts[i] = payload.substr(start,end == std::string::npos ? end : end-start);
        if (end != std::string::npos) start = end+1;
    }
    Claims claims;
    std::uint64_t capabilities = 0;
    if (parts[0] != "v2" || parts[1] != audience || parts[2] != build || parts[3] != request.key ||
        parts[4] != request.serial || parts[5] != request.nonce || parts[10] != keyId ||
        !PanelLicense::detail::ParseNumber(parts[6],claims.issued) ||
        !PanelLicense::detail::ParseNumber(parts[7],claims.until) ||
        !PanelLicense::detail::ParseNumber(parts[8],claims.licenseUntil) ||
        !PanelLicense::detail::ParseNumber(parts[9],capabilities)) return false;
    if (!claims.issued || claims.until <= claims.issued || claims.until-claims.issued > leaseSeconds ||
        claims.licenseUntil < claims.until || claims.licenseUntil > std::uint64_t(INT64_MAX) || claims.until <= wall ||
        (claims.issued > wall && claims.issued-wall > clockSkewSeconds) ||
        (request.wall > claims.issued && request.wall-claims.issued > clockSkewSeconds) ||
        !capabilities || (capabilities & ~std::uint64_t(allCapabilities)) != 0 ||
        tick-request.tick >= (claims.until-claims.issued)*1000) return false;
    if (PanelLicense::detail::VerifySignature(payload,signature,publicKey) != PanelLicense::Verification::Valid) return false;
    claims.capabilities = static_cast<std::uint32_t>(capabilities);
    output = claims;
    return true;
}

class Session {
    struct Lease {
        Claims claims;
        std::uint64_t began, deadline;
    };
    std::shared_ptr<const Lease> lease_;
    mutable std::mutex mutex_;
    std::uint64_t generation_ = 0;
    Request pending_;
    const std::string publicKey_, keyId_;
public:
    explicit Session(const char* publicKey = PanelLicense::kPublicKey, const char* keyId = PanelLicense::kKeyId)
        : publicKey_(publicKey), keyId_(keyId) {}
    Request Begin(std::string key, std::string serial, std::string nonce,
                  std::uint64_t wall, std::uint64_t tick, bool refresh = false) {
        std::lock_guard<std::mutex> lock(mutex_);
        ++generation_;
        pending_ = {};
        if (!refresh) std::atomic_store(&lease_,std::shared_ptr<const Lease>{});
        if (!Credentials(key,serial) || !Hex(nonce) || !wall || !tick) return {};
        pending_ = {std::move(key),std::move(serial),std::move(nonce),wall,tick,generation_};
        return pending_;
    }
    bool Accept(const Request& request, const std::string& payload, const std::string& signature,
                std::uint64_t wall, std::uint64_t tick) {
        Claims claims;
        const bool verified = Verify(request,payload,signature,wall,tick,claims,publicKey_.c_str(),keyId_.c_str());
        std::lock_guard<std::mutex> lock(mutex_);
        if (!request.generation || request.generation != pending_.generation || request.nonce != pending_.nonce ||
            request.key != pending_.key || request.serial != pending_.serial || request.tick != pending_.tick ||
            request.wall != pending_.wall) return false;
        pending_ = {};
        if (!verified || request.tick > UINT64_MAX-(claims.until-claims.issued)*1000) {
            std::atomic_store(&lease_,std::shared_ptr<const Lease>{});return false;
        }
        std::shared_ptr<const Lease> next = std::make_shared<Lease>(Lease{claims,request.tick,
            request.tick+(claims.until-claims.issued)*1000});
        std::atomic_store(&lease_,std::move(next));
        return true;
    }
    void Reject(const Request& request, bool transient = false) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!request.generation || request.generation != pending_.generation) return;
        pending_ = {};
        if (!transient) std::atomic_store(&lease_,std::shared_ptr<const Lease>{});
    }
    bool Allows(Capability capability, std::uint64_t tick) const noexcept {
        const auto lease = std::atomic_load(&lease_);
        const auto bit = static_cast<std::uint32_t>(capability);
        return lease && bit && (bit & ~allCapabilities) == 0 &&
            (lease->claims.capabilities & bit) == bit && tick >= lease->began && tick < lease->deadline;
    }
    std::uint64_t LicenseUntil() const noexcept {
        const auto lease = std::atomic_load(&lease_);return lease ? lease->claims.licenseUntil : 0;
    }
};
}
