#pragma once

#include <imgui.h>
#include <imgui_internal.h>

namespace ImGuiCustom
{
inline bool BeginButtonDropDown(const char* label)
{
    bool pressed = ImGui::Button(label);

    auto        window = ImGui::GetCurrentWindow();
    const auto& style  = ImGui::GetStyle();
    const float x      = ImGui::GetCursorPosX();
    const float y      = ImGui::GetCursorPosY();

    ImGui::SetNextWindowPos({window->Pos.x + x, window->Pos.y + y - style.ItemSpacing.y});

    if (pressed) {
        ImGui::OpenPopup(label);
    }

    if (ImGui::BeginPopup(label)) {
        return true;
    }

    return false;
}

inline void EndButtonDropDown()
{
    ImGui::EndPopup();
}

template <size_t N> inline bool DropDownFlags(uint32_t& flags, const std::array<const char*, N>& labels)
{
    bool was_any_changed = false;
    for (int i = 0; i < N; ++i) {
        if (strlen(labels[i]) > 0 && ImGui::MenuItem(labels[i], nullptr, ((flags >> i) & 1))) {
            flags ^= (1 << i);
            was_any_changed = true;
        }
    }

    return was_any_changed;
}
}; // namespace ImGuiCustom