#include <graphics/UI.h>
#include <Window.h>
#include <Settings.h>
#include <jc3/FileLoader.h>
#include <fonts/fontawesome_icons.h>
#include <graphics/imgui/imgui_rotate.h>
#include <graphics/Camera.h>

#include <imgui.h>
#include <json.hpp>
#include <jc3/formats/AvalancheArchive.h>

extern bool g_DrawBoundingBoxes;
extern bool g_DrawDebugInfo;
extern AvalancheArchive* g_CurrentLoadedArchive;
static bool g_ShowAllArchiveContents = false;
extern bool g_ShowModelLabels = true;

void UI::Render()
{
    bool open_settings_modal = false;

    if (ImGui::BeginMainMenuBar())
    {
        m_MainMenuBarHeight = ImGui::GetWindowHeight();

        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit")) {
                TerminateProcess(GetCurrentProcess(), 0);
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Renderer"))
        {
            static bool wireframe = false;
            if (ImGui::Checkbox("Wireframe", &wireframe)) {
                Renderer::Get()->SetFillMode(wireframe ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID);
            }

            if (ImGui::Checkbox("Bounding Boxes", &g_DrawBoundingBoxes)) {
                Settings::Get()->SetValue("draw_bounding_boxes", g_DrawBoundingBoxes);
            }

            if (ImGui::Checkbox("Debug Info", &g_DrawDebugInfo)) {
                Settings::Get()->SetValue("draw_debug_info", g_DrawDebugInfo);
            }

            if (ImGui::Checkbox("Show Model Labels", &g_ShowModelLabels)) {
                Settings::Get()->SetValue("show_model_labels", g_ShowModelLabels);
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Camera"))
        {
            if (ImGui::MenuItem("Reset")) {
                Camera::Get()->ResetToDefault();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Archive", (g_CurrentLoadedArchive != nullptr)))
        {
            static bool show_all_archvie_contents = false;
            if (ImGui::Checkbox("Show All Contents", &show_all_archvie_contents)) {
                std::vector<std::string> only_include;
                if (!show_all_archvie_contents) {
                    only_include.emplace_back(".rbm");
                }

                g_CurrentLoadedArchive->GetDirectoryList()->Parse(g_CurrentLoadedArchive->GetStreamArchive(), only_include);
            }

            if (ImGui::MenuItem("Export To...")) {
                Window::Get()->ShowFolderSelection("Select a folder to export the archive to.", [](const std::string& selected) {
                    if (g_CurrentLoadedArchive) {
                        g_CurrentLoadedArchive->ExportArchive(selected);
                    }
                });
            }

            if (ImGui::MenuItem("Close")) {
                delete g_CurrentLoadedArchive;
                TextureManager::Get()->Flush();
            }

            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }

    // Stats
    ImGui::Begin("Stats", nullptr, ImVec2(), 0.0f, (ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings));
    {
        auto windowSize = Window::Get()->GetSize();

        ImGui::SetWindowPos(ImVec2(10, windowSize.y - 40));
        ImGui::Text("%.01f FPS (%.02f ms) (%.0f x %.0f)", ImGui::GetIO().Framerate, (1000.0f / ImGui::GetIO().Framerate), windowSize.x, windowSize.y);
    }
    ImGui::End();

    RenderFileTreeView();
}

void RenderDirectoryList(json* current, bool open_folders = false)
{
    if (current->is_object()) {
        for (auto it = current->begin(); it != current->end(); ++it) {
            auto current_key = it.key();
            
            if (current_key != "/") {
                ImGuiTreeNodeFlags flags = 0;

                // open the root directories
                if (open_folders || (current_key == "editor" || current_key == "entities" || current_key == "models" || current_key == "locations")) {
                    flags |= ImGuiTreeNodeFlags_DefaultOpen;
                }

                auto id = ImGui::GetID(current_key.c_str());
                auto is_open = ImGui::GetStateStorage()->GetInt(id, flags & ImGuiTreeNodeFlags_DefaultOpen);
                 
                if (ImGui::TreeNodeEx(current_key.c_str(), flags, "%s %s", is_open ? ICON_FA_FOLDER_OPEN : ICON_FA_FOLDER, current_key.c_str())) {
                    auto next = &current->operator[](current_key);
                    RenderDirectoryList(next, open_folders);

                    ImGui::TreePop();
                }
            }
            else {
                auto next = &current->operator[](current_key);
                RenderDirectoryList(next, open_folders);
            }
        }
    }
    else {
        if (current->is_string()) {
            __debugbreak();
        }
        else {
            for (auto& leaf : *current) {
                auto filename = leaf.get<std::string>();
                auto is_archive = (filename.find(".ee") != std::string::npos || filename.find(".bl") != std::string::npos || filename.find(".nl") != std::string::npos);

                ImGui::TreeNodeEx(filename.c_str(), (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen), "%s %s", is_archive ? ICON_FA_FILE_ARCHIVE_O : ICON_FA_FILE_O, filename.c_str());

                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip(filename.c_str());
                }

                if (ImGui::IsItemClicked()) {
                    UI::Get()->Events().FileTreeItemSelected(filename);
                }

                ImGui::PushID(filename.c_str());
                if (ImGui::BeginPopupContextItem("Context Menu"))
                {
                    std::stringstream ss;
                    ss << ICON_FA_FLOPPY_O << " Save as...";

                    static bool selected = false;

                    ImGui::Selectable(ss.str().c_str(), &selected);

                    ImGui::EndPopup();
                }
                ImGui::PopID();
            }
        }
    }
}

void UI::RenderFileTreeView()
{
    std::stringstream title;
    if (g_CurrentLoadedArchive) {
        title << ICON_FA_FOLDER_OPEN << " " << g_CurrentLoadedArchive->GetStreamArchive()->m_Filename.filename().string().c_str();
    }
    else {
        title << "Entity Explorer";
    }

    ImGui::Begin(title.str().c_str(), nullptr, ImVec2(), 1.0f, (ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings));
    {
        auto size = Window::Get()->GetSize();
        ImGui::SetWindowPos(ImVec2(size.x - 400, m_MainMenuBarHeight));
        ImGui::SetWindowSize(ImVec2(400, (size.y - m_MainMenuBarHeight)));

        // should we render the root entity explorer?
        if (!g_CurrentLoadedArchive) {
            RenderDirectoryList(FileLoader::Get()->GetDirectoryList()->GetStructure());
        }
        else {
            RenderDirectoryList(g_CurrentLoadedArchive->GetDirectoryList()->GetStructure(), false);
        }
    }
    ImGui::End();
}

void UI::RenderSpinner(const std::string& str)
{
    ImRotateStart();
    ImGui::Text(ICON_FA_SPINNER);
    ImRotateEnd(-0.005f * GetTickCount());

    ImGui::SameLine();
    ImGui::Text(str.c_str());
}