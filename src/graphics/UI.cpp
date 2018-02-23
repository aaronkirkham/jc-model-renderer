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

#include <import_export/ImportExportManager.h>

#include <atomic>

extern bool g_DrawBoundingBoxes;
extern bool g_DrawDebugInfo;
extern AvalancheArchive* g_CurrentLoadedArchive;
static bool g_ShowAllArchiveContents = false;
extern bool g_ShowModelLabels = true;
extern fs::path g_JC3Directory;

void UI::Render()
{
    const auto& window_size = Window::Get()->GetSize();

    if (ImGui::BeginMainMenuBar())
    {
        m_MainMenuBarHeight = ImGui::GetWindowHeight();

        if (ImGui::BeginMenu("File"))
        {
            /*const auto& importers = ImportExportManager::Get()->GetImporters();
            const auto& exporters = ImportExportManager::Get()->GetExporters();

            if (ImGui::BeginMenu(ICON_FA_PLUS_CIRCLE "  Import", (importers.size() > 0))) {
                for (const auto& importer : importers) {
                    if (ImGui::MenuItem(importer->GetName(), importer->GetOutputExtension())) {
                    }
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu(ICON_FA_MINUS_CIRCLE "  Export", (exporters.size() > 0))) {
                for (const auto& exporter : exporters) {
                    if (ImGui::MenuItem(exporter->GetName(), exporter->GetOutputExtension())) {
                    }
                }

                ImGui::EndMenu();
            }

            ImGui::Separator();*/

            if (ImGui::MenuItem(ICON_FA_FOLDER "  Select JC3 path")) {
            }

            if (ImGui::MenuItem(ICON_FA_WINDOW_CLOSE "  Exit")) {
                // TODO: change me
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

#if 0
            if (ImGui::MenuItem("Export To...")) {
                UI::Get()->Events().ExportFileRequest(g_CurrentLoadedArchive->GetFileName(), nullptr);
            }
#endif

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
        ImGui::SetCursorPos(ImVec2(20, window_size.y - 35));
        ImGui::Text("%.01f fps (%.02f ms) (%.0f x %.0f)", ImGui::GetIO().Framerate, (1000.0f / ImGui::GetIO().Framerate), window_size.x, window_size.y);
    }
    ImGui::End();

    // Status
    ImGui::Begin("Status", nullptr, ImVec2(), 0.0f, (ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings));
    {
        static auto item_spacing = 24.0f;
        uint32_t current_index = 0;

        std::lock_guard<std::recursive_mutex> _lk{ m_StatusTextsMutex };

        // render all status texts
        for (const auto& status : m_StatusTexts) {
            const auto& text_size = ImGui::CalcTextSize(status.second.c_str());

            ImGui::SetCursorPos(ImVec2(window_size.x - text_size.x - 440, window_size.y - 35 - (item_spacing * current_index)));
            RenderSpinner(status.second);

            ++current_index;
        }
    }
    ImGui::End();
    
    RenderFileTreeView();
}

void RenderDirectoryList(json* current, bool open_folders = false)
{
    if (!current) {
        return;
    }

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
                 
                if (ImGui::TreeNodeEx(current_key.c_str(), flags, "%s  %s", is_open ? ICON_FA_FOLDER_OPEN : ICON_FA_FOLDER, current_key.c_str())) {
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
#ifdef DEBUG
            __debugbreak();
#endif
        }
        else {
            for (auto& leaf : *current) {
                auto filename = leaf.get<std::string>();
                auto is_archive = (filename.find(".ee") != std::string::npos || filename.find(".bl") != std::string::npos || filename.find(".nl") != std::string::npos);

                ImGui::TreeNodeEx(filename.c_str(), (ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen), "%s %s", is_archive ? ICON_FA_FILE_ARCHIVE_O : ICON_FA_FILE_O, filename.c_str());

                // tooltips
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip(filename.c_str());
                }

                // fire file selected events
                if (ImGui::IsItemClicked()) {
                    UI::Get()->Events().FileTreeItemSelected(filename);
                }

                // context menu
                UI::Get()->RenderContextMenu(filename);
            }
        }
    }
}

void UI::RenderFileTreeView()
{
    std::stringstream title;
    if (g_CurrentLoadedArchive) {
        title << ICON_FA_FOLDER_OPEN << "  " << g_CurrentLoadedArchive->GetStreamArchive()->m_Filename.filename().string().c_str();
    }
    else {
        title << "File Explorer";
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
        else if (g_CurrentLoadedArchive->GetDirectoryList()) {
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

void UI::RenderContextMenu(const fs::path& filename, uint32_t unique_id_extra)
{
    std::stringstream unique_id;
    unique_id << "context-menu-" << filename << "-" << unique_id_extra;

    ImGui::PushID(unique_id.str().c_str());

    if (ImGui::BeginPopupContextItem("Context Menu")) {
        // general save file
        if (ImGui::Selectable(ICON_FA_FLOPPY_O "  Save file...")) {
            UI::Get()->Events().SaveFileRequest(filename);
        }

        // exporters
        const auto& exporters = ImportExportManager::Get()->GetExportersForExtension(filename.extension().string());
        if (exporters.size() > 0) {
            if (ImGui::BeginMenu(ICON_FA_MINUS_CIRCLE "  Export", (exporters.size() > 0))) {
                for (const auto& exporter : exporters) {
                    if (ImGui::MenuItem(exporter->GetName(), exporter->GetOutputExtension())) {
                        UI::Get()->Events().ExportFileRequest(filename, exporter);
                    }
                }

                ImGui::EndMenu();
            }
        }

        ImGui::EndPopup();
    }

    ImGui::PopID();
}

uint64_t UI::PushStatusText(const std::string& str)
{
    static std::atomic_uint64_t unique_ids = { 0 };
    auto id = ++unique_ids;

    std::lock_guard<std::recursive_mutex> _lk{ m_StatusTextsMutex };
    m_StatusTexts[id] = std::move(str);
    return id;
}

void UI::PopStatusText(uint64_t id)
{
    std::lock_guard<std::recursive_mutex> _lk{ m_StatusTextsMutex };
    m_StatusTexts.erase(id);
}