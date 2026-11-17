#pragma once

#include "../../Trigger/TriggerOverlay.h"

inline void RenderTriggerCustomizationTab(float childWidth, float childHeight, float columnGap,
                                          bool& hideRecordEnabled, bool hideRecordUnavailable,
                                          void (*retrySetup)()) {
    using runtime_preview_menu::ChildFrame;
    using runtime_preview_menu::BeginContentChild;
    using runtime_preview_menu::EndContentChild;
    custom::BeginGroup();
    {
        const ChildFrame left = BeginContentChild("TRIGGERBOT", ImVec2(childWidth, childHeight));
        auto triggerSettings = PhantomTrigger::GetSettings();
        if (custom::Checkbox("Triggerbot", &triggerSettings.enabled) &&
            triggerSettings.enabled && retrySetup && PhantomTrigger::GetRuntimeStatus() ==
                PhantomTrigger::RuntimeStatus::WaitingForBinaryVerification)
            retrySetup();
        static const char* triggerModes[] = {"Both", "ADS Only"};
        int triggerMode = triggerSettings.mode == PhantomTrigger::Mode::ADSOnly ? 1 : 0;
        custom::Combo("Triggerbot Mode", &triggerMode, triggerModes, IM_ARRAYSIZE(triggerModes), -1);
        triggerSettings.mode = triggerMode == 1 ? PhantomTrigger::Mode::ADSOnly : PhantomTrigger::Mode::Both;
        custom::Checkbox("Hide Record", &hideRecordEnabled);
        PhantomTrigger::SetSettings(triggerSettings);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(.78f, .75f, .86f, 1.f));
        if (hideRecordUnavailable)
            ImGui::TextWrapped("Hide Record is unsupported on this device");
        if (retrySetup && PhantomTrigger::GetRuntimeStatus() ==
                PhantomTrigger::RuntimeStatus::WaitingForBinaryVerification &&
            custom::Button("Retry trigger setup", ImVec2(0, 0)))
            retrySetup();
        ImGui::PopStyleColor();
        EndContentChild(left);
    }
    custom::EndGroup();

    ImGui::SameLine(0.0f, columnGap);

    custom::BeginGroup();
    {
        const ChildFrame right = BeginContentChild("DOT CROSSHAIR", ImVec2(childWidth, childHeight));
        auto triggerSettings = PhantomTrigger::GetSettings();
        custom::Checkbox("Dot Crosshair", &triggerSettings.showDot);
        PhantomTrigger::SetSettings(triggerSettings);
        auto& dotAppearance = TriggerDotStyle::appearance;
        custom::SliderFloat("Dot Size", &dotAppearance.radius, 1.0f, 12.0f, "%.1f");
        custom::SliderFloat("Outline Size", &dotAppearance.outline, 0.0f, 5.0f, "%.1f");
        custom::SliderFloat("Dot Opacity", &dotAppearance.opacity, 0.1f, 1.0f, "%.2f");
        ImGui::TextColored(ImVec4(.78f, .75f, .86f, 1.f), "Dot Color");
        ImGui::SameLine();
        ImGui::ColorEdit3("##DotColor", dotAppearance.color, ImGuiColorEditFlags_NoInputs);
        TriggerDotStyle::Normalize(dotAppearance);
        ImGui::Spacing();
        ImGui::TextColored(main_runtime_theme::GetAccentVec4(), "PREVIEW");
        const ImVec2 previewPos = ImGui::GetCursorScreenPos();
        const ImVec2 available = ImGui::GetContentRegionAvail();
        const ImVec2 previewSize(ImMax(1.0f, available.x),
                                 ImMin(110.0f, ImMax(48.0f, available.y - 8.0f)));
        ImDrawList* previewDraw = ImGui::GetWindowDrawList();
        previewDraw->AddRectFilled(previewPos, previewPos + previewSize, IM_COL32(8, 11, 18, 240), 8.0f);
        previewDraw->AddLine(ImVec2(previewPos.x + previewSize.x * 0.5f, previewPos.y + 10.0f),
                             ImVec2(previewPos.x + previewSize.x * 0.5f, previewPos.y + previewSize.y - 10.0f),
                             IM_COL32(75, 88, 110, 100));
        previewDraw->AddLine(ImVec2(previewPos.x + 10.0f, previewPos.y + previewSize.y * 0.5f),
                             ImVec2(previewPos.x + previewSize.x - 10.0f, previewPos.y + previewSize.y * 0.5f),
                             IM_COL32(75, 88, 110, 100));
        PhantomTrigger::DrawStyledDot(previewDraw, previewPos + previewSize * 0.5f);
        ImGui::Dummy(previewSize);
        EndContentChild(right);
    }
    custom::EndGroup();
}
