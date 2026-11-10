#pragma once

#include <openssl/evp.h>
#include <openssl/pem.h>

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace PanelLicense {

// Public verification key corresponding to the supplied panel's signing key.
constexpr char kPublicKey[] =
    "-----BEGIN PUBLIC KEY-----\n"
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAxhAQ9NVfZYbVQ+5YW6CI\n"
    "Tmh/z8bDlzFmZOTrSD9gLUEwT2C8e7N9plBGUiZj156BAS+ESSoXIf/qoj/TKvmg\n"
    "x3YhgvRwi2Rnq6zlbTuZBvbwuuMV3gTw0SvmZKZGpdlcnPW1OABhedjDjQMhY4CD\n"
    "RROwqfjoVGbdjv0L6tQ2XSh1CwQwdJ4XgwSHTOUOw551X3cr9zVoOEfvn0/YGxF/\n"
    "ygjTwUbIcE3gblQbZ5kHcouzYysWjnUbIwi3NirC1O8cEWi4nhaNxMFp2BqiiiO1\n"
    "OsPPUUInoYTjyTwZg9N9+/Jqrg/+yodcrhj6irALEoBsAJ0iupPGpRtZKg+ypn+l\n"
    "KwIDAQAB\n"
    "-----END PUBLIC KEY-----\n";

enum class Verification {
    Valid, InvalidInput, InvalidPayload, Expired, InvalidSignature, CryptoFailure
};

inline std::string OutcomeText(bool valid, const std::string& reason) {
    if (valid) return "OK";
    return reason.empty() || reason == "OK" ? "License rejected" : reason;
}

namespace detail {
inline bool ParseNumber(const std::string& text, std::uint64_t& out) noexcept {
    if (text.empty() || (text.size() > 1 && text[0] == '0')) return false;
    out = 0;
    for (char c : text) {
        if (c < '0' || c > '9') return false;
        const unsigned digit = static_cast<unsigned>(c - '0');
        if (out > (std::numeric_limits<std::uint64_t>::max() - digit) / 10) return false;
        out = out * 10 + digit;
    }
    return true;
}

inline bool DecodeSignature(const std::string& value, std::vector<unsigned char>& out) {
    if (value.empty() || value.size() > 1024 || value.size() % 4 != 0) return false;
    std::size_t padding = 0;
    if (value.back() == '=') ++padding;
    if (value.size() >= 2 && value[value.size() - 2] == '=') ++padding;
    for (std::size_t i = 0; i < value.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(value[i]);
        if (i >= value.size() - padding) {
            if (c != '=') return false;
        } else if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                     (c >= '0' && c <= '9') || c == '+' || c == '/')) return false;
    }
    out.resize(value.size() / 4 * 3);
    const int len = EVP_DecodeBlock(out.data(),
        reinterpret_cast<const unsigned char*>(value.data()), static_cast<int>(value.size()));
    if (len < 0 || static_cast<std::size_t>(len) < padding) return false;
    out.resize(static_cast<std::size_t>(len) - padding);
    // EVP_DecodeBlock accepts noncanonical padding bits. Require one encoding.
    std::vector<unsigned char> canonical((out.size() + 2) / 3 * 4 + 1);
    const int written = EVP_EncodeBlock(canonical.data(), out.data(), static_cast<int>(out.size()));
    return written >= 0 && static_cast<std::size_t>(written) == value.size() &&
           value.compare(0, value.size(), reinterpret_cast<const char*>(canonical.data()),
                         static_cast<std::size_t>(written)) == 0;
}
}

// A valid signature authenticates the exact expected key, device, expiry and build.
// now and responseExpires are Unix seconds; the caller must obtain now from a valid clock.
inline Verification Verify(const std::string& payload, const std::string& signatureBase64,
                           const std::string& expectedKey, const std::string& expectedSerial,
                           std::uint64_t responseExpires, std::uint64_t now,
                           const char* publicKey = kPublicKey) {
    if (!publicKey || expectedKey.empty() || expectedKey.size() > 128 ||
        expectedKey.find('|') != std::string::npos || expectedKey.find('\0') != std::string::npos ||
        expectedSerial.size() < 8 || expectedSerial.size() > 64 || now == 0 ||
        payload.size() > 512) return Verification::InvalidInput;
    for (unsigned char c : expectedSerial) {
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '-')) return Verification::InvalidInput;
    }

    std::string parts[5];
    std::size_t start = 0;
    for (unsigned i = 0; i < 5; ++i) {
        const auto end = payload.find('|', start);
        if ((i < 4 && end == std::string::npos) || (i == 4 && end != std::string::npos))
            return Verification::InvalidPayload;
        parts[i] = payload.substr(start, end == std::string::npos ? end : end - start);
        if (end != std::string::npos) start = end + 1;
    }
    std::uint64_t expires = 0;
    if (parts[0] != "v1" || parts[1] != expectedKey || parts[2] != expectedSerial ||
        !detail::ParseNumber(parts[3], expires) || expires != responseExpires || parts[4] != "1")
        return Verification::InvalidPayload;
    if (expires <= now) return Verification::Expired;

    std::vector<unsigned char> signature;
    if (!detail::DecodeSignature(signatureBase64, signature)) return Verification::InvalidSignature;
    BIO* bio = BIO_new_mem_buf(publicKey, -1);
    if (!bio) return Verification::CryptoFailure;
    EVP_PKEY* key = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    if (!key) return Verification::CryptoFailure;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) { EVP_PKEY_free(key); return Verification::CryptoFailure; }
    const int initialized = EVP_DigestVerifyInit(ctx, nullptr, EVP_sha256(), nullptr, key);
    const int updated = initialized == 1 ? EVP_DigestVerifyUpdate(ctx, payload.data(), payload.size()) : 0;
    const int verified = updated == 1 ? EVP_DigestVerifyFinal(ctx, signature.data(), signature.size()) : 0;
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(key);
    if (initialized != 1 || updated != 1) return Verification::CryptoFailure;
    return verified == 1 ? Verification::Valid : Verification::InvalidSignature;
}
}
