#pragma once
#include "../../ImGui/imgui.h"
#include "../../Trigger/WorldDiagnostics.h"

inline void RenderTriggerDiagnosticRevision() {
    ImGui::PushTextWrapPos(0.f);
    ImGui::TextDisabled("Trace build: %s",PhantomTrigger::WorldDiagnosticRevision);
    ImGui::PopTextWrapPos();
}

inline void RenderTriggerDiagnostics(PhantomTrigger::DecisionStatus status,
                                     const PhantomTrigger::WorldDiagnostic& sample,
                                     std::uint64_t now) {
    RenderTriggerDiagnosticRevision();
    if(status!=PhantomTrigger::DecisionStatus::WorldBlocked && status!=PhantomTrigger::DecisionStatus::QueryUnavailable) return;
    char text[384];
    if(PhantomTrigger::FormatWorldDiagnostic(text,sizeof(text),sample,now)) ImGui::TextWrapped("%s",text);
    else ImGui::TextWrapped("No matching world failure sample");
}

inline void RenderTriggerDiagnostics(PhantomTrigger::DecisionStatus status) {
    if(status!=PhantomTrigger::DecisionStatus::WorldBlocked && status!=PhantomTrigger::DecisionStatus::QueryUnavailable) {
        RenderTriggerDiagnosticRevision();
        return;
    }
    RenderTriggerDiagnostics(status,PhantomTrigger::GetWorldDiagnostic(),PhantomTrigger::WorldDiagnosticClock());
}
