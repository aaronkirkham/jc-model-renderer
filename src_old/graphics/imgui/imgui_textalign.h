#pragma once

#include <imgui.h>

enum ImGuiTextAlign_ {
    ImGuiTextAlign_Left   = 0,
    ImGuiTextAlign_Center = 1,
    ImGuiTextAlign_Right  = 2,
};

namespace ImGuiCustom
{
void TextAligned(ImGuiTextAlign_ alignment, const char* fmt, ...)
{
    ImGuiWindow* window     = ImGui::GetCurrentWindowRead();
    float        cursor_pos = window->WindowPadding.x;

    ImGuiContext& g         = *GImGui;
    const char*   text_end  = g.TempBuffer + ImFormatString(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), fmt);
    const auto&   text_size = ImGui::CalcTextSize(g.TempBuffer, text_end);

    switch (alignment) {
        case ImGuiTextAlign_Center:
            cursor_pos = ((window->Size.x / 2) - (text_size.x / 2));
            break;

        case ImGuiTextAlign_Right:
            cursor_pos = ((window->Size.x - text_size.x) - window->WindowPadding.x);
            break;
    }

    ImGui::SetCursorPosX(cursor_pos);
    ImGui::TextUnformatted(g.TempBuffer, text_end);
}
}; // namespace ImGuiCustom
