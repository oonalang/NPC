#pragma once

#include "LicenseRuntime.h"
#include "../AstralPrtctn/json.hpp"
#include <algorithm>
#include <string_view>

namespace license {
inline std::string LicenseResponseDiagnostic(const char* reason, long status,
                                             const char* contentType, std::string_view body) {
    std::string type = contentType ? contentType : "";
    const auto lower = [](unsigned char character) {
        return character >= 'A' && character <= 'Z' ? character + ('a' - 'A') : character;
    };
    std::transform(type.begin(), type.end(), type.begin(), lower);
    const char* media = "other";
    if (type.empty()) media = "none";
    else if (type.find("json") != std::string::npos) media = "json";
    else if (type.find("html") != std::string::npos) media = "html";
    else if (type.find("text/") != std::string::npos) media = "text";

    const auto start = body.find_first_not_of(" \t\r\n");
    const char* shape = "other";
    if (start == std::string_view::npos) shape = "empty";
    else if (body.size() >= 2 && static_cast<unsigned char>(body[0]) == 0x1f &&
             static_cast<unsigned char>(body[1]) == 0x8b) shape = "gzip";
    else if (body[start] == '<') shape = "markup";
    else if (body[start] == '{' || body[start] == '[') shape = "json-like";

    const bool html = start != std::string_view::npos && body[start] == '<';
    std::string message = html ? "License server returned an HTML page" : "Invalid license response";
    message += std::string(" [YH-") + reason + ", HTTP " + std::to_string(status) +
        ", " + std::to_string(body.size()) + " B, " + media + "/" + shape + "]";
    if (html) {
        std::string markup(body);
        std::transform(markup.begin(), markup.end(), markup.begin(), lower);
        const auto titleStart = markup.find("<title>");
        if (titleStart != std::string::npos) {
            const auto textStart = titleStart + 7;
            const auto titleEnd = markup.find("</title>", textStart);
            if (titleEnd != std::string::npos) {
                std::string title;
                for (auto index = textStart; index < titleEnd && title.size() < 64; ++index) {
                    const unsigned char character = body[index];
                    if (character >= 32 && character < 127) title += static_cast<char>(character);
                    else if (character == '\r' || character == '\n' || character == '\t') title += ' ';
                }
                if (!title.empty()) message += " Page title " + title;
            }
        }
    }
    return message;
}

inline Response ParseLicenseResponse(long status, const char* contentType, std::string_view body) {
    Response response;
    if (status >= 500) {
        response.failure = Failure::Transient;
        response.error = "License connection unavailable";
        return response;
    }
    const auto value = nlohmann::json::parse(body.begin(), body.end(), nullptr, false);
    const bool validStatus = value.is_object() && value.contains("status") && value["status"].is_boolean();
    if (status != 200) {
        response.error = "License server rejected the request";
        if (validStatus && !value["status"].get<bool>() && value.contains("reason") && value["reason"].is_string()) {
            const auto reason = value["reason"].get<std::string>();
            if (!reason.empty() && reason.size() <= 256) response.error = reason;
        }
    } else if (!validStatus) {
        response.error = LicenseResponseDiagnostic(value.is_discarded() ? "D1" : "D2", status, contentType, body);
    } else if (!value["status"].get<bool>()) {
        response.error = value.contains("reason") && value["reason"].is_string() ?
            value["reason"].get<std::string>() : "License rejected";
        if (response.error.empty() || response.error.size() > 256) response.error = "License rejected";
    } else if (value.contains("data") && value["data"].is_object()) {
        const auto& data = value["data"];
        if (data.contains("payload") && data["payload"].is_string() &&
            data.contains("sig") && data["sig"].is_string()) {
            response.payload = data["payload"].get<std::string>();
            response.signature = data["sig"].get<std::string>();
            response.failure = Failure::None;
        } else response.error = "Client and license panel must both support session protocol v2";
    } else response.error = LicenseResponseDiagnostic("D3", status, contentType, body);
    return response;
}
}
