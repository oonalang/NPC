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
    if(status!=PhantomTrigger::DecisionStatus::WorldBlocked && status!=PhantomTrigger::DecisionStatus::QueryUnavailable) {
        if(sample.sequence && now>=sample.timeMillis && now-sample.timeMillis<=PhantomTrigger::WorldDiagnosticFreshnessMillis)
            ImGui::TextWrapped("Last world query: %u us | rays %u+%u",sample.elapsedMicros,
                static_cast<unsigned>(sample.fastQueries),static_cast<unsigned>(sample.contactQueries));
        return;
    }
    char text[384];
    if(PhantomTrigger::FormatWorldDiagnostic(text,sizeof(text),sample,now)) ImGui::TextWrapped("%s",text);
    else ImGui::TextWrapped("No matching world failure sample");
}

inline void RenderTriggerDiagnostics(PhantomTrigger::DecisionStatus status) {
    RenderTriggerDiagnostics(status,PhantomTrigger::GetWorldDiagnostic(),PhantomTrigger::WorldDiagnosticClock());
}
