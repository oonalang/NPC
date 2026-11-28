#include "../src/main/jni/Security/LicenseResponse.h"
#include <cassert>
#include <iostream>

int main() {
    using license::Failure;
    using license::ParseLicenseResponse;
    const auto rejection = [](long status, const std::string& body, const std::string& expected) {
        const auto response = ParseLicenseResponse(status, "application/json", body);
        assert(response.failure == Failure::Rejected);
        assert(response.payload.empty() && response.signature.empty());
        assert(response.error.find(expected) != std::string::npos);
    };
    rejection(403, R"({"status":false,"reason":"Access key expired"})", "Access key expired");
    rejection(400, R"({"status":false,"reason":"Malformed request"})", "Malformed request");
    rejection(426, R"({"status":false,"reason":"Client update required"})", "Client update required");
    rejection(403, R"({"status":false,"reason":""})", "License server rejected the request");
    rejection(403, R"({"status":false,"reason":12})", "License server rejected the request");
    rejection(403, nlohmann::json({{"status", false}, {"reason", std::string(257, 'x')}}).dump(),
              "License server rejected the request");
    rejection(403, "<html>Forbidden</html>", "License server rejected the request");
    rejection(403, R"({"status":true,"data":{"payload":"fixture","sig":"fixture"}})",
              "License server rejected the request");
    rejection(200, "", "D1");
    rejection(200, "\x1f\x8b", "gzip");
    rejection(200, "[]", "D2");
    rejection(200, R"({"status":1})", "D2");
    rejection(200, R"({"status":true})", "D3");
    rejection(200, R"({"status":true,"data":{"token":"legacy"}})", "session protocol v2");
    rejection(200, R"({"status":false,"reason":"Access key rejected"})", "Access key rejected");

    std::string page = "<!DOCTYPE html><html><head><TITLE>Browser Verification</TITLE></head></html>";
    page.resize(12191, ' ');
    const auto html = ParseLicenseResponse(200, "TEXT/HTML; charset=utf-8", page);
    assert(html.failure == Failure::Rejected);
    assert(html.error.find("HTML page") != std::string::npos);
    assert(html.error.find("YH-D1, HTTP 200, 12191 B, html/markup") != std::string::npos);
    assert(html.error.find("Page title Browser Verification") != std::string::npos);
    rejection(200, "<html><title>incomplete", "HTML page");
    rejection(200, "<html>no title</html>", "HTML page");
    const auto longTitle = ParseLicenseResponse(200, nullptr,
        "<title>" + std::string(1000, 'x') + "</title>");
    assert(longTitle.error.size() < 256);
    const auto controls = ParseLicenseResponse(200, "text/html", "<title>Line\nTwo\x01</title>");
    assert(controls.error.find("Page title Line Two") != std::string::npos);
    assert(controls.error.find('\x01') == std::string::npos);

    const auto unavailable = ParseLicenseResponse(503, "application/json",
        R"({"status":true,"data":{"payload":"fixture","sig":"fixture"}})");
    assert(unavailable.failure == Failure::Transient && unavailable.payload.empty());
    const auto envelope = ParseLicenseResponse(200, "application/json",
        R"({"status":true,"data":{"payload":"fixture","sig":"fixture"}})");
    assert(envelope.failure == Failure::None);
    assert(envelope.payload == "fixture" && envelope.signature == "fixture");
    std::cout << "License response parsing preserves rejection reasons and diagnoses HTML pages\n";
}
