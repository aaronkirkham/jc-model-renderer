#include <imgui.h>

namespace ImGuiCustom
{
bool TabItemScroll(const char* label, bool selected = false, bool* p_open = nullptr, ImGuiTabItemFlags flags = 0)
{
    if (selected) {
        flags |= ImGuiTabItemFlags_SetSelected;
    }

    // disabled style
    if (flags & ImGuiItemFlags_Disabled)
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));

    // TODO: tab items should not be clickable when disabled flag is set.
    auto res = ImGui::BeginTabItem(label, p_open, flags);

    // disabled style
    if (flags & ImGuiItemFlags_Disabled)
        ImGui::PopStyleColor();

    if (res) {
        // setup the child window id
        std::string _label = label;
        _label.append(" Scroll Container");

        // child background will always match the window background
        ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
        ImGui::BeginChild(_label.c_str());
    }

    return res;
}

void EndTabItemScroll()
{
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::EndTabItem();
}
}; // namespace ImGuiCustom
