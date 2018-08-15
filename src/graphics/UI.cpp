#include <graphics/UI.h>
#include <Window.h>
#include <Settings.h>
#include <imgui.h>
#include <imgui_tabs.h>
#include <json.hpp>

#include <graphics/imgui/fonts/fontawesome_icons.h>
#include <graphics/imgui/imgui_rotate.h>
#include <graphics/imgui/imgui_tabscrollcontent.h>
#include <graphics/Camera.h>

#include <jc3/FileLoader.h>
#include <Jc3/RenderBlockFactory.h>
#include <jc3/formats/AvalancheArchive.h>
#include <jc3/formats/RenderBlockModel.h>
#include <jc3/formats/RuntimeContainer.h>

#include <import_export/ImportExportManager.h>

#include <gtc/type_ptr.hpp>

#include <atomic>
#include <shellapi.h>

#define DIRECTORY_LIST_WIDTH 400

extern bool g_DrawBoundingBoxes;
extern bool g_ShowModelLabels;
extern fs::path g_JC3Directory;

static bool g_ShowAllArchiveContents = false;
static bool g_ShowAboutWindow = false;
static bool g_ShowBackgroundColourPicker = false;

#ifdef DEBUG
static bool g_CheckForUpdatesEnabled = false;
#else
static bool g_CheckForUpdatesEnabled = true;
#endif

static bool _is_dragging = false;
static fs::path _dragdrop_filename = "";

void UI::Render()
{
    static std::once_flag _flag;
    std::call_once(_flag, [&] {
        Window::Get()->Events().DragEnter.connect([&](const fs::path& filename) {
            _is_dragging = true;
            _dragdrop_filename = filename;

            ImGuiIO& io = ImGui::GetIO();
            io.MouseDown[0] = true;
        });

        Window::Get()->Events().DragLeave.connect([&] {
            _is_dragging = false;

            ImGuiIO& io = ImGui::GetIO();
            io.MouseDown[0] = false;
        });

        Window::Get()->Events().DragDropped.connect([&] {
            _is_dragging = false;

            ImGuiIO& io = ImGui::GetIO();
            io.MouseDown[0] = false;
        });
    });

    if (_is_dragging) {
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceExtern)) {
            static uint32_t test = 0x10;
            ImGui::SetDragDropPayload("_ADD_FILE", &test, 1);

            ImGui::Text(_dragdrop_filename.filename().string().c_str());

            ImGui::EndDragDropSource();
        }
    }

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
            if (ImGui::BeginMenu("Focus On", RenderBlockModel::Instances.size() != 0)) {
                for (const auto& model : RenderBlockModel::Instances) {
                    if (ImGui::MenuItem(model.second->GetFileName().c_str())) {
                        Camera::Get()->FocusOn(model.second.get());
                    }
                }

                ImGui::EndMenu();
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
        ImGui::OpenPopup("Background Colour Picker");
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
    if (ImGui::BeginPopupModal("Background Colour Picker", &g_ShowBackgroundColourPicker, (ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)))
    {
        ImGui::SetWindowSize({ 400, 400 });

        auto col = Renderer::Get()->GetClearColour();
        if (ImGui::ColorPicker3("Colour", glm::value_ptr(col))) {
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
    ImGui::SetNextWindowSize({ window_size.x, window_size.y });
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

    // file tree view
    RenderFileTreeView();

    // render runtime container stuff
    for (auto it = RuntimeContainer::Instances.begin(); it != RuntimeContainer::Instances.end(); ) {
        std::stringstream ss;
        ss << "Runtime Container Editor - ";
        ss << (*it).second->GetFileName().filename();

        bool open = true;
        if (ImGui::Begin(ss.str().c_str(), &open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings)) {
            (*it).second->DrawUI();

            ImGui::End();
        }

        if (!open) {
            std::lock_guard<std::recursive_mutex> _lk{ RuntimeContainer::InstancesMutex };
            it = RuntimeContainer::Instances.erase(it);
        }
        else ++it;
    }
}

// TODO: move the texture view stuff into here.
void UI::RenderFileTreeView()
{
    const auto& window_size = Window::Get()->GetSize();

    // render the archive directory list
    ImGui::SetNextWindowBgAlpha(1.0f);
    ImGui::Begin("Archive Directory List", nullptr, (ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar));
    {
        ImGui::SetWindowPos({ window_size.x - DIRECTORY_LIST_WIDTH, m_MainMenuBarHeight });
        ImGui::SetWindowSize({ DIRECTORY_LIST_WIDTH, (window_size.y - m_MainMenuBarHeight) });

        ImGui::BeginTabBar("Directory List Tabs", (ImGuiTabBarFlags_NoReorder | ImGuiTabBarFlags_SizingPolicyEqual));
        {
            // switch active tab if we need to
            if (!m_TabToSwitch.empty()) {
                ImGui::SetTabItemSelected(m_TabToSwitch.c_str());
                m_TabToSwitch.clear();
            }

            // file explorer tab
            if (ImGuiCustom::TabItemScroll("File Explorer")) {
                FileLoader::Get()->GetDirectoryList()->Draw(nullptr);
                ImGuiCustom::EndTabItemScroll();
            }

            // archives
            if (ImGuiCustom::TabItemScroll("Archives", nullptr, AvalancheArchive::Instances.size() == 0 ? ImGuiItemFlags_Disabled : 0)) {
                for (auto it = AvalancheArchive::Instances.begin(); it != AvalancheArchive::Instances.end(); ) {
                    const auto& archive = (*it).second;
                    const auto& filename = archive->GetFilePath().filename();

                    // render the current directory
                    bool is_still_open = true;
                    auto open = ImGui::CollapsingHeader(filename.string().c_str(), &is_still_open);

                    // drag drop target
                    if (ImGui::BeginDragDropTarget()) {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_ADD_FILE")) {
                            archive->AddDirectory(_dragdrop_filename, _dragdrop_filename.parent_path());
                        }

                        ImGui::EndDragDropTarget();
                    }

                    // context menu
                    RenderContextMenu(archive->GetFilePath(), 0, CTX_ARCHIVE);

                    if (open && archive->GetDirectoryList()) {
                        // draw the directory list
                        ImGui::PushID(filename.string().c_str());
                        archive->GetDirectoryList()->Draw(archive.get());
                        ImGui::PopID();
                    }

                    // if the close button was pressed, delete the archive
                    if (!is_still_open) {
                        std::lock_guard<std::recursive_mutex> _lk{ AvalancheArchive::InstancesMutex };
                        it = AvalancheArchive::Instances.erase(it);
                        TextureManager::Get()->Flush();

                        if (AvalancheArchive::Instances.size() == 0) {
                            m_TabToSwitch = "File Explorer";
                        }
  
                        continue;
                    }

                    ++it;
                }

                ImGuiCustom::EndTabItemScroll();
            }

            // render blocks
            if (ImGuiCustom::TabItemScroll("Models", nullptr, RenderBlockModel::Instances.size() == 0 ? ImGuiItemFlags_Disabled : 0)) {
                for (auto it = RenderBlockModel::Instances.begin(); it != RenderBlockModel::Instances.end(); ) {
                    const auto& filename = (*it).second->GetFileName();

                    // render the current model info
                    bool is_not_closed = true;
                    if (ImGui::CollapsingHeader(filename.c_str(), &is_not_closed)) {
                        uint32_t render_block_index = 0;
                        for (auto& render_block : (*it).second->GetRenderBlocks()) {
                            // TODO: highlight the current render block when hovering over the ui

                            // unique block id
                            std::stringstream block_label;
                            block_label << render_block->GetTypeName();
                            block_label << "##" << filename << "-" << render_block_index;

                            // make the things transparent if the block isn't rendering
                            const auto block_visible = render_block->IsVisible();
                            if (!block_visible) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.35f);

                            // current block header
                            const auto render_block_open = ImGui::TreeNode(block_label.str().c_str());

                            // block context menu
                            {
                                if (!block_visible) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1);

                                if (ImGui::BeginPopupContextItem()) {
                                    if (ImGui::Selectable(block_visible ? ICON_FA_STOP "  Hide Render Block" : ICON_FA_PLAY "  Show Render Block")) {
                                        render_block->SetVisible(!block_visible);
                                    }

                                    ImGui::EndPopup();
                                }

                                if (!block_visible) ImGui::PopStyleVar();
                            }

                            // render the current render block info
                            if (render_block_open) {
                                ImGui::Text(ICON_FA_COGS "  Attributes");

                                // draw render block ui
                                render_block->DrawUI();

                                ImGui::Text(ICON_FA_PICTURE_O "  Textures");

                                // draw render block textures
                                const auto& textures = render_block->GetTextures();
                                if (!textures.empty()) {
                                    ImGui::Columns(3, 0, false);

                                    // draw textures
                                    for (const auto& texture : textures) {
                                        const auto is_loaded = texture->IsLoaded();
                                        const auto& path = texture->GetPath();

                                        auto window_size = Window::Get()->GetSize();
                                        auto aspect_ratio = (window_size.x / window_size.y);

                                        auto width = ImGui::GetWindowWidth() / ImGui::GetColumnsCount();
                                        auto texture_size = ImVec2(width, (width / aspect_ratio));

                                        // draw the texture name
                                        if (is_loaded) {
                                            ImGui::Text(path.filename().string().c_str());
                                        }
                                        else {
                                            static auto red = ImVec4{ 1, 0, 0, 1 };
                                            ImGui::TextColored(red, path.filename().string().c_str());
                                        }

                                        ImGui::BeginGroup();

                                        // draw the texture image
                                        auto srv = is_loaded ? texture->GetSRV() : TextureManager::Get()->GetMissingTexture()->GetSRV();
                                        ImGui::Image(srv, texture_size);

                                        // tooltip
                                        if (ImGui::IsItemHovered()) {
                                            ImGui::SetTooltip(path.filename().string().c_str());
                                        }

                                        // context menu
                                        if (is_loaded) {
                                            RenderContextMenu(path, ImGui::GetColumnIndex(), CTX_TEXTURE);
                                        }

                                        ImGui::EndGroup();

                                        ImGui::NextColumn();
                                    }

                                    ImGui::Columns();
                                }

                                ImGui::TreePop();
                            }

                            if (!block_visible) ImGui::PopStyleVar();

                            ++render_block_index;
                        }
                    }

                    // if the close button was pressed, delete the model
                    if (!is_not_closed) {
                        std::lock_guard<std::recursive_mutex> _lk{ RenderBlockModel::InstancesMutex };
                        it = RenderBlockModel::Instances.erase(it);

                        continue;
                    }
                    
                    ++it;
                }

                ImGuiCustom::EndTabItemScroll();
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

void UI::RenderContextMenu(const fs::path& filename, uint32_t unique_id_extra, uint32_t flags)
{
    std::stringstream unique_id;
    unique_id << "context-menu-" << filename << "-" << unique_id_extra;

    ImGui::PushID(unique_id.str().c_str());
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1);

    if (ImGui::BeginPopupContextItem("Context Menu")) {
        // general save file
        if (ImGui::Selectable(ICON_FA_FLOPPY_O "  Save file")) {
            Window::Get()->ShowFolderSelection("Select a folder to save the file to.", [&](const fs::path& selected) {
                UI::Get()->Events().SaveFileRequest(filename, selected);
            });
        }

#if 0
        // general save to dropzon
        if (ImGui::Selectable(ICON_FA_FLOPPY_O "  Save to dropzone")) {
            DEBUG_LOG("save file request (dropzone)");
            const auto dropzone = g_JC3Directory / "dropzone";
            UI::Get()->Events().SaveFileRequest(filename, dropzone);
        }
#endif

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

#if 0
        // archive specific stuff
        if (flags & CTX_FILE && flags & CTX_ARCHIVE) {
            if (ImGui::Selectable(ICON_FA_TIMES_CIRCLE_O "  Delete file")) {
            }
        }
#endif

        // custom context menus
        auto it = m_ContextMenuCallbacks.find(filename.extension().string());
        if (it != m_ContextMenuCallbacks.end()) {
            (*it).second(filename);
        }


        ImGui::EndPopup();
    }

    ImGui::PopStyleVar();
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

void UI::RegisterContextMenuCallback(const std::vector<std::string>& extensions, ContextMenuCallback fn)
{
    for (const auto& extension : extensions) {
        m_ContextMenuCallbacks[extension] = fn;
    }
}