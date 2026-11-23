#pragma once
#include "../../Lite/Widgets.h"
#include <algorithm>
#include <cctype>
#include <cstring>

#if defined(ETHNIR_LITE) && ETHNIR_LITE
#include "../../Lite/Keyboard.h"
inline void RenderVirtualKeyboard(const char* id,char* buffer,size_t capacity,bool* open) {
    lite_keyboard::Render(id,buffer,capacity,open);
}
#else
inline void RenderVirtualKeyboard(const char* id, char* buffer, size_t capacity, bool* open) {
    if (!open || !*open || !buffer || capacity < 2) return;
    static bool uppercase = false;
    static bool symbols = false;
    auto& io = ImGui::GetIO();
    const float height = ImMin(250.f, io.DisplaySize.y * .48f);
    ImGui::SetNextWindowPos({0, io.DisplaySize.y - height}, ImGuiCond_Always);
    ImGui::SetNextWindowSize({io.DisplaySize.x, height}, ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {10, 10});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {4, 4});
    ImGui::PushStyleColor(ImGuiCol_WindowBg, { .055f, .055f, .06f, 1.f });
    ImGui::Begin(id, nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
    const float previousScale = lite_widgets::scale;
    lite_widgets::scale = 1.f;
    ImGui::SetWindowFontScale(1.f);
    const char* rows[]{"1234567890", symbols ? "!@#$%^&*()" : "qwertyuiop",
        symbols ? "_-+=:;/?." : "asdfghjkl", symbols ? "[]{}<>~|\\" : "zxcvbnm"};
    const float rowHeight = ImMax(22.f, (ImGui::GetContentRegionAvail().y - 16.f) / 5.f);
    const float keyWidth = (ImGui::GetContentRegionAvail().x - 36.f) / 10.f;
    for (int row = 0; row < 4; ++row) {
        ImGui::PushID(row);
        const int count = static_cast<int>(std::strlen(rows[row]));
        for (int column = 0; column < count; ++column) {
            if (column) ImGui::SameLine();
            char key[]{static_cast<char>(uppercase ? std::toupper(static_cast<unsigned char>(rows[row][column])) : rows[row][column]), 0};
            if (lite_widgets::Button(key, {keyWidth, rowHeight})) {
                const auto length = std::strlen(buffer);
                if (length + 1 < capacity) { buffer[length] = key[0]; buffer[length + 1] = 0; }
            }
        }
        ImGui::PopID();
    }
    const float actionWidth = (ImGui::GetContentRegionAvail().x - 20.f) / 6.f;
    if (lite_widgets::Button(uppercase ? "abc" : "ABC", {actionWidth, rowHeight})) uppercase = !uppercase;
    ImGui::SameLine();
    if (lite_widgets::Button(symbols ? "abc" : "#+=", {actionWidth, rowHeight})) symbols = !symbols;
    ImGui::SameLine();
    if (lite_widgets::Button("Space", {actionWidth, rowHeight})) {
        const auto length = std::strlen(buffer);
        if (length + 1 < capacity) { buffer[length] = ' '; buffer[length + 1] = 0; }
    }
    ImGui::SameLine();
    if (lite_widgets::Button("Del", {actionWidth, rowHeight})) {
        auto length = std::strlen(buffer);
        if (length) {
            --length;
            while (length && (static_cast<unsigned char>(buffer[length]) & 0xc0u) == 0x80u) --length;
            buffer[length] = 0;
        }
    }
    ImGui::SameLine();
    if (lite_widgets::Button("Clear", {actionWidth, rowHeight})) buffer[0] = 0;
    ImGui::SameLine();
    if (lite_widgets::Button("Done", {actionWidth, rowHeight})) *open = false;
    lite_widgets::scale = previousScale;
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}
#endif
