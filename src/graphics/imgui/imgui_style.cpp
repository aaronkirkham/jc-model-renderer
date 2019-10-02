#include "fonts/fontawesome5.h"
#include "fonts/fontawesome5_icons.h"
#include "fonts/roboto_medium.h"

#include <imgui.h>

// https://gist.github.com/ongamex/4ee36fb23d6c527939d0f4ba72144d29
void SetupImGuiStyle()
{
    auto& io    = ImGui::GetIO();
    auto& style = ImGui::GetStyle();

    style.GrabRounding      = 0.0f;
    style.WindowRounding    = 0.0f;
    style.ScrollbarRounding = 3.0f;
    style.FrameRounding     = 0.0f;
    style.WindowTitleAlign  = ImVec2(0.5f, 0.5f);
    style.ItemSpacing.y     = 6.0f;
    style.WindowBorderSize  = 0.0f;

    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = nullptr;
    // io.ConfigDockingWithShift = false;

    style.Colors[ImGuiCol_Text]                 = ImVec4(0.73f, 0.73f, 0.73f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled]         = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_WindowBg]             = ImVec4(0.26f, 0.26f, 0.26f, 0.95f);
    style.Colors[ImGuiCol_ChildWindowBg]        = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    style.Colors[ImGuiCol_PopupBg]              = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_Border]               = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
    style.Colors[ImGuiCol_BorderShadow]         = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
    style.Colors[ImGuiCol_FrameBg]              = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive]        = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    style.Colors[ImGuiCol_TitleBg]              = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive]        = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg]            = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_CheckMark]            = ImVec4(0.78f, 0.78f, 0.78f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab]           = ImVec4(0.74f, 0.74f, 0.74f, 0.55f);
    style.Colors[ImGuiCol_SliderGrabActive]     = ImVec4(0.74f, 0.74f, 0.74f, 1.00f);
    style.Colors[ImGuiCol_Button]               = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered]        = ImVec4(0.43f, 0.43f, 0.43f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive]         = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
    style.Colors[ImGuiCol_Header]               = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered]        = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive]         = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    // style.Colors[ImGuiCol_Column]               = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    // style.Colors[ImGuiCol_ColumnHovered]        = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    // style.Colors[ImGuiCol_ColumnActive]         = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip]           = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_ResizeGripHovered]    = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_ResizeGripActive]     = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_PlotLines]            = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered]     = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram]        = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg]       = ImVec4(0.32f, 0.52f, 0.65f, 1.00f);
    style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.20f, 0.20f, 0.20f, 0.50f);
    style.Colors[ImGuiCol_Tab]                  = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
    style.Colors[ImGuiCol_TabActive]            = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_TabHovered]           = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_Separator]            = ImVec4(0.50f, 0.50f, 0.50f, 0.50f);

    // add default font
    // io.Fonts->AddFontDefault();
    io.Fonts->AddFontFromMemoryCompressedBase85TTF(roboto_medium_compressed_data, 15.0f);

    // merge in icons from Font Awesome
    static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
    ImFontConfig         icons_config;
    icons_config.MergeMode  = true;
    icons_config.PixelSnapH = true;
    io.Fonts->AddFontFromMemoryCompressedTTF(fontawesome_5_compressed_data, fontawesome_5_compressed_size, 12.0f,
                                             &icons_config, icons_ranges);
}
