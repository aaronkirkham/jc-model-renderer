#include <imgui.h>
#include <imgui_internal.h>

namespace ImGuiCustom
{
bool BeginButtonDropDown(const char* label)
{
    bool pressed = ImGui::Button(label);

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const float  x      = ImGui::GetCursorPosX();
    const float  y      = ImGui::GetCursorPosY();

    ImGui::SetNextWindowPos({window->Pos.x + x, window->Pos.y + y});

    if (pressed) {
        ImGui::OpenPopup(label);
    }

    if (ImGui::BeginPopup(label)) {
        return true;
    }

    return false;
}

void EndButtonDropDown()
{
    ImGui::EndPopup();
}
}; // namespace ImGuiCustom
