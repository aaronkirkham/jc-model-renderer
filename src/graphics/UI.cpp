#include <graphics/UI.h>
#include <Window.h>
#include <Settings.h>
#include <imgui.h>
#include <imgui_tabs.h>
#include <json.hpp>

#include <fonts/fontawesome_icons.h>
#include <graphics/imgui/imgui_rotate.h>
#include <graphics/imgui/imgui_tabscrollcontent.h>
#include <graphics/Camera.h>

#include <jc3/FileLoader.h>
#include <jc3/formats/AvalancheArchive.h>
#include <jc3/formats/RenderBlockModel.h>
#include <jc3/ModelManager.h>

#include <import_export/ImportExportManager.h>

#include <gtc/type_ptr.hpp>

#include <atomic>
#include <shellapi.h>

#define DIRECTORY_LIST_WIDTH 400

extern bool g_DrawBoundingBoxes;
extern bool g_ShowModelLabels;

extern AvalancheArchive* g_CurrentLoadedArchive;
extern fs::path g_JC3Directory;

static bool g_ShowAllArchiveContents = false;
static bool g_ShowAboutWindow = false;
static bool g_ShowBackgroundColourPicker = false;

#ifdef DEBUG
static bool g_CheckForUpdatesEnabled = false;
#else
static bool g_CheckForUpdatesEnabled = true;
#endif

void UI::Render()
{
    const auto& window_size = Window::Get()->GetSize();

    if (ImGui::BeginMainMenuBar())
    {
        m_MainMenuBarHeight = ImGui::GetWindowHeight();

        if (ImGui::BeginMenu("File"))
        {
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

            if (ImGui::Checkbox("Show bounding boxes", &g_DrawBoundingBoxes)) {
                Settings::Get()->SetValue("draw_bounding_boxes", g_DrawBoundingBoxes);
            }

            if (ImGui::Checkbox("Show model labels", &g_ShowModelLabels)) {
                Settings::Get()->SetValue("show_model_labels", g_ShowModelLabels);
            }

            if (ImGui::Button("Background colour")) {
                g_ShowBackgroundColourPicker = !g_ShowBackgroundColourPicker;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Camera"))
        {
            if (ImGui::MenuItem(ICON_FA_UNDO "  Reset")) {
                Camera::Get()->ResetToDefault();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem(ICON_FA_INFO_CIRCLE "  About")) {
                g_ShowAboutWindow = !g_ShowAboutWindow;
            }

            if (ImGui::MenuItem(ICON_FA_REFRESH  "  Check for updates", nullptr, false, g_CheckForUpdatesEnabled)) {
                CheckForUpdates(true);
            }

            if (ImGui::MenuItem(ICON_FA_GITHUB "  View on GitHub")) {
                ShellExecuteA(nullptr, "open", "https://github.com/aaronkirkham/jc3-rbm-renderer", nullptr, nullptr, SW_SHOWNORMAL);
            }

            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }

    if (g_ShowAboutWindow) {
        ImGui::OpenPopup("About");
    }
    else if (g_ShowBackgroundColourPicker) {
        ImGui::OpenPopup("BGColPicker");
    }

    // About
    if (ImGui::BeginPopupModal("About", &g_ShowAboutWindow, (ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)))
    {
        ImGui::SetWindowSize({ 400, 400 });

        static int32_t current_version[3] = { VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION };

        ImGui::Text("Just Cause 3 Render Block Model (.rbm) Renderer");
        ImGui::Text("Version %d.%d.%d", current_version[0], current_version[1], current_version[2]);
        ImGui::Text("https://github.com/aaronkirkham/jc3-rbm-renderer");

        ImGui::Separator();

        ImGui::BeginChild("Scrolling", {}, true);

        ImGui::TextWrapped("Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the \"Software\"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:\n\n"

            "The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.\n\n"

            "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.");

        ImGui::EndChild();

        ImGui::EndPopup();
    }

    // Background colour picker
    if (ImGui::BeginPopupModal("BGColPicker", &g_ShowBackgroundColourPicker, (ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)))
    {
        ImGui::SetWindowSize({ 400, 400 });

        auto col = Renderer::Get()->GetClearColour();
        if (ImGui::ColorPicker3("bg col", glm::value_ptr(col))) {
            Renderer::Get()->SetClearColour(col);
        }

        if (ImGui::Button("Reset To Default")) {
            Renderer::Get()->SetClearColour(g_DefaultClearColour);
        }

        ImGui::EndPopup();
    }

    // Scene rendering
    // TODO: once ImGui has support for viewports/tabs/docking some of the UI will change a bit
    // https://github.com/ocornut/imgui/issues/1542
    // https://github.com/ocornut/imgui/issues/261

    // Stats
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::Begin("Stats", nullptr, (ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings));
    {
        ImGui::SetWindowPos({ 10, (window_size.y - 35 - 24) });
        ImGui::Text("%.01f fps (%.02f ms) (%.0f x %.0f)", ImGui::GetIO().Framerate, (1000.0f / ImGui::GetIO().Framerate), window_size.x, window_size.y);
    }
    ImGui::End();

    // Status
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::SetNextWindowSize({ 500, window_size.y });
    ImGui::Begin("Status", nullptr, (ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings));
    {
        const auto size = ImGui::GetWindowSize();
        ImGui::SetWindowPos({ ((window_size.x - DIRECTORY_LIST_WIDTH) - size.x - 10), (window_size.y - size.y - 10) });

        static const auto item_spacing = 24.0f;
        uint32_t current_index = 0;

        std::lock_guard<std::recursive_mutex> _lk{ m_StatusTextsMutex };

        // render all status texts
        for (const auto& status : m_StatusTexts) {
            const auto& text_size = ImGui::CalcTextSize(status.second.c_str());

            ImGui::SetCursorPos({ (size.x - text_size.x - 25), (window_size.y - 20 - (item_spacing * current_index)) });
            RenderSpinner(status.second);

            ++current_index;
        }
    }
    ImGui::End();
    
    RenderFileTreeView();
}

void RenderDirectoryList(json* current, std::string prev = "", bool open_folders = false)
{
    if (!current) {
        return;
    }

    if (current->is_object()) {
        for (auto it = current->begin(); it != current->end(); ++it) {
            auto current_key = it.key();
            
            if (current_key != "/") {
                auto id = ImGui::GetID(current_key.c_str());
                auto is_open = ImGui::GetStateStorage()->GetInt(id);
                 
                if (ImGui::TreeNodeEx(current_key.c_str(), ImGuiTreeNodeFlags_None, "%s  %s", is_open ? ICON_FA_FOLDER_OPEN : ICON_FA_FOLDER, current_key.c_str())) {
                    auto next = &current->operator[](current_key);
                    RenderDirectoryList(next, prev.length() ? prev + "/" + current_key : current_key, open_folders);

                    ImGui::TreePop();
                }
            }
            else {
                auto next = &current->operator[](current_key);
                RenderDirectoryList(next, prev, open_folders);
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

                std::string file_path;

                if (prev.length() == 0) {
                    file_path = filename;
                }
                else if (prev.length() == 1) {
                    file_path = prev + filename;
                }
                else {
                    file_path = prev + "/" + filename;
                }

                // tooltips
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip(filename.c_str());
                }

                // fire file selected events
                if (ImGui::IsItemClicked()) {
                    UI::Get()->Events().FileTreeItemSelected(file_path);
                }

                // context menu
                UI::Get()->RenderContextMenu(file_path);
            }
        }
    }
}

// TODO: move the texture view stuff into here.
void UI::RenderFileTreeView()
{
    const auto& models = ModelManager::Get()->GetModels();

    const auto draw_render_blocks_ui = (models.size() > 0);
    bool skip_render_blocks_ui = false;
    const auto& window_size = Window::Get()->GetSize();
    const ImGuiWindowFlags window_flags = (ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);

    // render the archive directory list
    ImGui::SetNextWindowBgAlpha(1.0f);
    ImGui::Begin("Archive Directory List", nullptr, window_flags | ImGuiWindowFlags_NoTitleBar);
    {
        ImGui::SetWindowPos({ window_size.x - DIRECTORY_LIST_WIDTH, m_MainMenuBarHeight });
        ImGui::SetWindowSize({ DIRECTORY_LIST_WIDTH, (window_size.y - m_MainMenuBarHeight) });

        ImGui::BeginTabBar("Directory List Tabs", ImGuiTabBarFlags_NoReorder);
        {
            // file explorer tab
            if (ImGuiCustom::TabItemScroll("File Explorer")) {
                RenderDirectoryList(FileLoader::Get()->GetDirectoryList()->GetStructure());
                ImGuiCustom::EndTabItemScroll();
            }

            // render blocks
            if (draw_render_blocks_ui && ImGuiCustom::TabItemScroll("Models")) {
                uint32_t model_index = 0;
                for (auto& model : models) {
                    uint32_t render_block_index = 0;
                    const auto& filename = model->GetFileName();

                    std::stringstream unique_model_id;
                    unique_model_id << model_index << "-" << filename;

                    ImGui::PushID(unique_model_id.str().c_str());

                    // render the collapsing header
                    const auto is_header_open = ImGui::CollapsingHeader(filename.c_str());

                    // context menu
                    if (ImGui::BeginPopupContextItem("Context Menu")) {
                        if (ImGui::Selectable(ICON_FA_WINDOW_CLOSE "  Close")) {
                            // NOTE: the RenderBlockModel destructor will clean everything up for us
                            // really don't like any of this..

                            delete model;
                        }

                        ImGui::EndPopup();
                    }

                    // render the current file info
                    if (model && is_header_open) {
                        const auto& render_blocks = model->GetRenderBlocks();
                        for (auto& render_block : render_blocks) {
                            // model hover stuff
                            // TODO: IsItemHovered seems broken here
                            // model->SetRenderBlockColour(render_block, ImGui::IsItemHovered() ? glm::vec4{ 1, 0, 0, 0.3 } : glm::vec4{ 1, 1, 1, 1 });

                            std::stringstream unique_block_id;
                            unique_block_id << render_block_index << "-" << render_block->GetTypeName();

                            // render the current render block info
                            if (ImGui::TreeNodeEx(unique_block_id.str().c_str(), 0, render_block->GetTypeName())) {
                                ImGui::Text(ICON_FA_COGS "  Attributes");

                                // draw render block ui
                                render_block->DrawUI();

                                ImGui::Text(ICON_FA_PICTURE_O "  Textures");

                                // draw render block textures
                                const auto& textures = render_block->GetTextures();
                                if (!textures.empty()) {
                                    ImGui::Columns(3, 0, false);

                                    for (const auto& texture : textures) {
                                        const auto is_loaded = texture->IsLoaded();

                                        auto window_size = Window::Get()->GetSize();
                                        auto aspect_ratio = (window_size.x / window_size.y);

                                        auto width = ImGui::GetWindowWidth() / ImGui::GetColumnsCount();
                                        auto texture_size = ImVec2(width, (width / aspect_ratio));

                                        // draw the texture name
                                        if (is_loaded) {
                                            ImGui::Text(texture->GetPath().filename().string().c_str());
                                        }
                                        else {
                                            static auto red = ImGui::GetColorU32({ 1, 0, 0, 1 });
                                            ImGui::PushStyleColor(ImGuiCol_Text, red);
                                            ImGui::Text(texture->GetPath().filename().string().c_str());
                                            ImGui::PopStyleColor();
                                        }

                                        ImGui::BeginGroup();

                                        // draw the texture image
                                        auto srv = is_loaded ? texture->GetSRV() : TextureManager::Get()->GetMissingTexture()->GetSRV();
                                        ImGui::Image(srv, texture_size);

                                        // tooltip
                                        if (ImGui::IsItemHovered()) {
                                            ImGui::SetTooltip(texture->GetPath().filename().string().c_str());
                                        }

                                        // context menu
                                        if (is_loaded) {
                                            RenderContextMenu(texture->GetPath(), ImGui::GetColumnIndex());
                                        }

                                        ImGui::EndGroup();

                                        ImGui::NextColumn();
                                    }

                                    ImGui::Columns();
                                }

                                ImGui::TreePop();
                            }

                            ++render_block_index;
                        }
                    }

                    ImGui::PopID();

                    ++model_index;
                }

                ImGuiCustom::EndTabItemScroll();
            }

            // current archive tab
            if (g_CurrentLoadedArchive) {
                std::stringstream title;
                title << ICON_FA_FOLDER_OPEN << "  " << g_CurrentLoadedArchive->GetStreamArchive()->m_Filename.filename().string().c_str();

                const auto tab_is_open = ImGuiCustom::TabItemScroll(title.str().c_str());

                // render the tab context menu
                ImGui::PushID("archive-tab-ctx-menu");
                {
                    if (ImGui::BeginPopupContextItem("Context Menu")) {
                        if (ImGui::Selectable(ICON_FA_WINDOW_CLOSE "  Close Archive")) {
                            delete g_CurrentLoadedArchive;
                            TextureManager::Get()->Flush();
                            skip_render_blocks_ui = true;
                        }
                        ImGui::EndPopup();
                    }
                }
                ImGui::PopID();

                // draw the directory list
                if (tab_is_open && g_CurrentLoadedArchive) {
                    RenderDirectoryList(g_CurrentLoadedArchive->GetDirectoryList()->GetStructure());
                    ImGuiCustom::EndTabItemScroll();
                }
            }
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

void UI::RenderSpinner(const std::string& str)
{
    ImGuiCustom::ImRotateStart();
    ImGui::Text(ICON_FA_SPINNER);
    ImGuiCustom::ImRotateEnd(-0.005f * GetTickCount());

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