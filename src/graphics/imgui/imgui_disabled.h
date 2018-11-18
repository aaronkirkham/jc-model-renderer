#pragma once

#include <imgui.h>
#include <imgui_internal.h>

namespace ImGuiCustom
{
static bool disabled_state = false;

inline void PushDisabled()
{
    ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    disabled_state = true;
}

inline void PushDisabled(bool state)
{
    if (state) {
        PushDisabled();
    }

    disabled_state = state;
}

inline void PopDisabled()
{
    if (disabled_state) {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
}
}; // namespace ImGuiCustom
