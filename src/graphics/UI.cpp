#include <Settings.h>
#include <Window.h>
#include <graphics/UI.h>
#include <imgui.h>
#include <json.hpp>

#include <graphics/Camera.h>
#include <graphics/imgui/fonts/fontawesome5_icons.h>
#include <graphics/imgui/imgui_buttondropdown.h>
#include <graphics/imgui/imgui_rotate.h>
#include <graphics/imgui/imgui_tabscrollcontent.h>

#include "widgets/Widget_ArchiveExplorer.h"
#include "widgets/Widget_FileExplorer.h"
#include "widgets/Widget_ModelExplorer.h"
#include "widgets/Widget_Viewport.h"

#include <Jc3/RenderBlockFactory.h>
#include <jc3/FileLoader.h>
#include <jc3/formats/AvalancheArchive.h>
#include <jc3/formats/RenderBlockModel.h>
#include <jc3/formats/RuntimeContainer.h>

#include <import_export/ImportExportManager.h>

#include <gtc/type_ptr.hpp>

#include <atomic>
#include <shellapi.h>

extern bool     g_DrawBoundingBoxes;
extern bool     g_ShowModelLabels;
extern fs::path g_JC3Directory;

static bool g_ShowAllArchiveContents = false;
static bool g_ShowAboutWindow        = false;

#ifdef DEBUG
static bool g_CheckForUpdatesEnabled = false;
#else
static bool g_CheckForUpdatesEnabled = true;
#endif

UI::UI()
{
    Window::Get()->Events().DragEnter.connect([&](const fs::path& filename) {
        DEBUG_LOG("DragEnter");

        m_IsDragDrop      = true;
        m_DragDropPayload = filename.generic_string();
    });

    static const auto ResetDragDrop = [&] {
        m_IsDragDrop      = false;
        m_DragDropPayload = "";
        DEBUG_LOG("DragLeave/DragDropped");
    };

    Window::Get()->Events().DragLeave.connect(ResetDragDrop);
    Window::Get()->Events().DragDropped.connect(ResetDragDrop);

    // add widgets
    m_Widgets.emplace_back(std::make_unique<Widget_FileExplorer>());
    m_Widgets.emplace_back(std::make_unique<Widget_ModelExplorer>());
    m_Widgets.emplace_back(std::make_unique<Widget_ArchiveExplorer>());
    m_Widgets.emplace_back(std::make_unique<Widget_Viewport>());
}

void UI::Render(RenderContext_t* context)
{
    const auto viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", nullptr,
                 ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar
                     | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
                     | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus
                     | ImGuiWindowFlags_NoBackground);
    ImGui::PopStyleVar();

    // handle drag drop payloads
    if (m_IsDragDrop && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceExtern)) {
        DragDropPayload payload;
        payload.type = DROPPAYLOAD_UNKNOWN;
        payload.data = m_DragDropPayload.c_str();

        ImGui::SetDragDropPayload("_123_", (void*)&payload, sizeof(payload), ImGuiCond_Once);
        ImGui::Text(m_DragDropPayload.c_str());
        ImGui::EndDragDropSource();
    }

    // setup dock
    ImGuiID dockspace_id = ImGui::GetID("MyDockspace");
    if (!ImGui::DockBuilderGetNode(dockspace_id)) {
        // reset dock
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, viewport->Size);

        auto dock_main_id  = dockspace_id;
        auto dock_right_id = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.3f, nullptr, &dock_main_id);

        ImGui::DockBuilderDockWindow("Viewport", dock_main_id);
        ImGui::DockBuilderDockWindow("File Explorer", dock_right_id);
        ImGui::DockBuilderDockWindow("Archives", dock_right_id);
        ImGui::DockBuilderDockWindow("Models", dock_right_id);
        ImGui::DockBuilderFinish(dockspace_id);
    }

    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruDockspace);

    // render widgets
    for (const auto& widget : m_Widgets) {
        if (widget->Begin()) {
            widget->Render(context);
            widget->End();
        }
    }

    // main menu bar
    if (ImGui::BeginMenuBar()) {
        // file
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem(ICON_FA_FOLDER "  Select JC3 path")) {
                SelectJustCause3Directory();
            }

            if (ImGui::MenuItem(ICON_FA_WINDOW_CLOSE "  Exit")) {
                Window::Get()->BeginShutdown();
            }

            ImGui::EndMenu();
        }

#if 0
        // create
        if (ImGui::BeginMenu("Create")) {
            if (ImGui::MenuItem("Render Block Model")) {
            }

            if (ImGui::MenuItem("Avalanche Archive")) {
                AvalancheArchive::make("test.ee", false);
            }

            ImGui::EndMenu();
        }
#endif

        // renderer
        if (ImGui::BeginMenu("Renderer")) {
            if (ImGui::BeginMenu("Visualize")) {
                if (ImGui::MenuItem("Diffuse", nullptr, m_CurrentActiveGBuffer == 0)) {
                    m_CurrentActiveGBuffer = 0;
                }

                if (ImGui::MenuItem("Normal", nullptr, m_CurrentActiveGBuffer == 1)) {
                    m_CurrentActiveGBuffer = 1;
                }

                if (ImGui::MenuItem("Properties", nullptr, m_CurrentActiveGBuffer == 2)) {
                    m_CurrentActiveGBuffer = 2;
                }

                if (ImGui::MenuItem("PropertiesEx", nullptr, m_CurrentActiveGBuffer == 3)) {
                    m_CurrentActiveGBuffer = 3;
                }

                ImGui::EndMenu();
            }

            static bool wireframe = false;
            if (ImGui::MenuItem("Wireframe", nullptr, wireframe)) {
                wireframe = !wireframe;
                Renderer::Get()->SetFillMode(wireframe ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID);
            }

            if (ImGui::MenuItem(ICON_FA_VECTOR_SQUARE "  Show Bounding Boxes", nullptr, g_DrawBoundingBoxes)) {
                g_DrawBoundingBoxes = !g_DrawBoundingBoxes;
                Settings::Get()->SetValue("draw_bounding_boxes", g_DrawBoundingBoxes);
            }

            if (ImGui::MenuItem(ICON_FA_FONT "  Show Model Labels", nullptr, g_ShowModelLabels)) {
                g_ShowModelLabels = !g_ShowModelLabels;
                Settings::Get()->SetValue("show_model_labels", g_ShowModelLabels);
            }

            if (ImGui::BeginMenu(ICON_FA_EYE_DROPPER "  Background")) {
                auto clear_colour = Renderer::Get()->GetClearColour();
                if (ImGui::ColorPicker3("Colour", glm::value_ptr(clear_colour))) {
                    Renderer::Get()->SetClearColour(clear_colour);
                }

                if (ImGui::Button("Reset To Default")) {
                    Renderer::Get()->SetClearColour(g_DefaultClearColour);
                }

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        // camera
        if (ImGui::BeginMenu("Camera")) {
            if (ImGui::BeginMenu(ICON_FA_CAMERA "  Focus On", RenderBlockModel::Instances.size() != 0)) {
                for (const auto& model : RenderBlockModel::Instances) {
                    if (ImGui::MenuItem(model.second->GetFileName().c_str())) {
                        Camera::Get()->FocusOn(model.second.get());
                    }
                }

                ImGui::EndMenu();
            }

            ImGui::EndMenu();
        }

        // help
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem(ICON_FA_INFO_CIRCLE "  About")) {
                g_ShowAboutWindow = !g_ShowAboutWindow;
            }

            if (ImGui::MenuItem(ICON_FA_SYNC "  Check for updates", nullptr, false, g_CheckForUpdatesEnabled)) {
                CheckForUpdates(true);
            }

            if (ImGui::MenuItem(ICON_FA_EXTERNAL_LINK_ALT "  View on GitHub")) {
                ShellExecuteA(nullptr, "open", "https://github.com/aaronkirkham/jc3-rbm-renderer", nullptr, nullptr,
                              SW_SHOWNORMAL);
            }

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    ImGui::End();

#if 0
    const auto& window_size = Window::Get()->GetSize();

    if (_is_dragging) {
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceExtern)) {
            static uint32_t test = 0x10;
            ImGui::SetDragDropPayload("_ADD_FILE", &test, 1);

            ImGui::Text(_dragdrop_filename.filename().string().c_str());

            ImGui::EndDragDropSource();
        }
    }

    // open the about popup
    if (g_ShowAboutWindow) {
        ImGui::OpenPopup("About");
    }

    // About
    if (ImGui::BeginPopupModal("About", &g_ShowAboutWindow, (ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))) {
        ImGui::SetWindowSize({400, 400});

        static int32_t current_version[3] = {VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION};

        ImGui::Text("Just Cause 3 Render Block Model Renderer");
        ImGui::Text("Version %d.%d.%d", current_version[0], current_version[1], current_version[2]);
        ImGui::Text("https://github.com/aaronkirkham/jc3-rbm-renderer");

        ImGui::Separator();

        ImGui::BeginChild("Scrolling", {}, true);

        ImGui::TextWrapped(
            "Permission is hereby granted, free of charge, to any person obtaining a copy of this software and "
            "associated documentation files (the \"Software\"), to deal in the Software without restriction, including "
            "without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell "
            "copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the "
            "following conditions:\n\n"

            "The above copyright notice and this permission notice shall be included in all copies or substantial "
            "portions of the Software.\n\n"

            "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT "
            "LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO "
            "EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER "
            "IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR "
            "THE USE OR OTHER DEALINGS IN THE SOFTWARE.");

        ImGui::EndChild();

        ImGui::EndPopup();
    }

    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::SetNextWindowPos({0, m_MainMenuBarHeight});
    ImGui::SetNextWindowSize({window_size.x - m_SidebarWidth, window_size.y - m_MainMenuBarHeight});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});

    // draw scene view
    if (ImGui::Begin("Scene", nullptr,
                     (ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs
                      | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings
                      | ImGuiWindowFlags_NoTitleBar))) {
        const auto& size = ImGui::GetWindowSize();
        m_SceneWidth     = size.x;

        // update camera projection if needed
        Camera::Get()->UpdateWindowSize({size.x, size.y});

        m_SceneDrawList = ImGui::GetWindowDrawList();

        ImGui::Image(Renderer::Get()->GetGBufferSRV(m_CurrentActiveGBuffer), ImGui::GetWindowSize());
        ImGui::End();
    }

    ImGui::PopStyleVar();

    

    // Stats
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::SetNextWindowPos({10, (window_size.y - 35 - 24)});
    if (ImGui::Begin("Stats", nullptr,
                     (ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs
                      | ImGuiWindowFlags_NoSavedSettings))) {
        ImGui::Text("%.01f fps (%.02f ms) (%.0f x %.0f)", ImGui::GetIO().Framerate,
                    (1000.0f / ImGui::GetIO().Framerate), window_size.x, window_size.y);
        ImGui::End();
    }

    // Status
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::SetNextWindowSize({window_size.x, window_size.y});
    ImGui::Begin("Status", nullptr,
                 (ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoScrollbar
                  | ImGuiWindowFlags_NoSavedSettings));
    {
        const auto size = ImGui::GetWindowSize();
        ImGui::SetWindowPos({((window_size.x - m_SidebarWidth) - size.x - 10), (window_size.y - size.y - 10)});

        static constexpr auto ITEM_SPACING  = 24.0f;
        uint32_t              current_index = 0;

        // render all status texts
        std::lock_guard<std::recursive_mutex> _lk{m_StatusTextsMutex};
        for (const auto& status : m_StatusTexts) {
            const auto& text_size = ImGui::CalcTextSize(status.second.c_str());

            ImGui::SetCursorPos({(size.x - text_size.x - 25), (window_size.y - 20 - (ITEM_SPACING * current_index))});
            RenderSpinner(status.second);

            ++current_index;
        }
    }
    ImGui::End();

    // render runtime container stuff
    for (auto it = RuntimeContainer::Instances.begin(); it != RuntimeContainer::Instances.end();) {
        std::stringstream ss;
        ss << "Runtime Container Editor - ";
        ss << (*it).second->GetFileName().filename();

        bool open = true;
        ImGui::SetNextWindowSize({800, 600}, ImGuiCond_Appearing);
        if (ImGui::Begin(
                ss.str().c_str(), &open,
                (ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking))) {
            (*it).second->DrawUI();

            ImGui::End();
        }

        if (!open) {
            std::lock_guard<std::recursive_mutex> _lk{RuntimeContainer::InstancesMutex};
            it = RuntimeContainer::Instances.erase(it);
        } else
            ++it;
    }
#endif
}

// TODO: move the texture view stuff into here.
void UI::RenderFileTreeView()
{
    const auto& window_size = Window::Get()->GetSize();

    // file explorer
    ImGui::Begin("File Explorer", nullptr);
    {
        //FileLoader::Get()->GetDirectoryList()->Draw(nullptr);
    }
    ImGui::End();

    // archives
    ImGui::Begin("Archives", nullptr);
    {
        for (auto it = AvalancheArchive::Instances.begin(); it != AvalancheArchive::Instances.end();) {
            const auto& archive  = (*it).second;
            const auto& filename = archive->GetFilePath().filename();

            // render the current directory
            bool is_still_open = true;
            auto open          = ImGui::CollapsingHeader(filename.string().c_str(), &is_still_open);

#if 0
            // drag drop target
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_ADD_FILE")) {
                    archive->AddDirectory(_dragdrop_filename, _dragdrop_filename.parent_path());
                }

                ImGui::EndDragDropTarget();
            }
#endif

            // context menu
            RenderContextMenu(archive->GetFilePath(), 0, CTX_ARCHIVE);

            if (open && archive->GetDirectoryList()) {
                // draw the directory list
                ImGui::PushID(filename.string().c_str());
                //archive->GetDirectoryList()->Draw(archive.get());
                ImGui::PopID();
            }

            // if the close button was pressed, delete the archive
            if (!is_still_open) {
                std::lock_guard<std::recursive_mutex> _lk{AvalancheArchive::InstancesMutex};
                it = AvalancheArchive::Instances.erase(it);

                // flush texture and shader manager
                TextureManager::Get()->Flush();
                ShaderManager::Get()->Empty();

                /*if (AvalancheArchive::Instances.size() == 0) {
                    m_TabToSwitch = TAB_FILE_EXPLORER;
                }*/

                continue;
            }

            ++it;
        }
    }
    ImGui::End();

    // models
    ImGui::Begin("Models", nullptr);
    {
        for (auto it = RenderBlockModel::Instances.begin(); it != RenderBlockModel::Instances.end();) {
            const auto& filename_with_path = (*it).second->GetPath();
            const auto& filename           = (*it).second->GetFileName();

            // render the current model info
            bool is_still_open = true;
            auto open          = ImGui::CollapsingHeader(filename.c_str(), &is_still_open);

            // context menu
            RenderContextMenu(filename_with_path);

            if (open) {
                uint32_t render_block_index = 0;
                for (auto& render_block : (*it).second->GetRenderBlocks()) {
                    // TODO: highlight the current render block when hovering over the ui

                    // unique block id
                    std::stringstream block_label;
                    block_label << render_block->GetTypeName();
                    block_label << "##" << filename << "-" << render_block_index;

                    // make the things transparent if the block isn't rendering
                    const auto block_visible = render_block->IsVisible();
                    if (!block_visible)
                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.35f);

                    // current block header
                    const auto render_block_open = ImGui::TreeNode(block_label.str().c_str());

                    // block context menu
                    {
                        if (!block_visible)
                            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1);

                        if (ImGui::BeginPopupContextItem()) {
                            if (ImGui::Selectable(block_visible ? ICON_FA_STOP "  Hide Render Block"
                                                                : ICON_FA_PLAY "  Show Render Block")) {
                                render_block->SetVisible(!block_visible);
                            }

                            ImGui::EndPopup();
                        }

                        if (!block_visible)
                            ImGui::PopStyleVar();
                    }

                    // render the current render block info
                    if (render_block_open) {
                        // show import button if the block doesn't have a vertex buffer
                        if (!render_block->GetVertexBuffer()) {
#ifdef ENABLE_OBJ_IMPORT
                            static auto red = ImVec4{1, 0, 0, 1};
                            ImGui::TextColored(red, "This model doesn't have any mesh!");

                            if (ImGuiCustom::BeginButtonDropDown("Import Mesh From")) {
                                const auto& importers = ImportExportManager::Get()->GetImporters(".rbm");
                                for (const auto& importer : importers) {
                                    if (ImGui::MenuItem(importer->GetName(), importer->GetExportExtension())) {
                                        UI::Events().ImportFileRequest(importer, [&](bool success, std::any data) {
                                            auto& [vertices, uvs, normals, indices] =
                                                std::any_cast<std::tuple<floats_t, floats_t, floats_t, uint16s_t>>(
                                                    data);

                                            render_block->SetData(&vertices, &indices, &uvs);
                                            render_block->Create();

                                            Renderer::Get()->AddToRenderList(render_block);
                                        });
                                    }
                                }

                                ImGuiCustom::EndButtonDropDown();
                            }
#endif
                        } else {
                            ImGui::Text(ICON_FA_COGS "  Attributes");

                            // draw render block ui
                            render_block->DrawUI();

                            ImGui::Text(ICON_FA_FILE_IMAGE "  Textures");

                            // draw render block textures
                            const auto& textures = render_block->GetTextures();
                            if (!textures.empty()) {
                                ImGui::Columns(3, 0, false);

                                // draw textures
                                for (const auto& texture : textures) {
                                    const auto  is_loaded = texture->IsLoaded();
                                    const auto& path      = texture->GetPath();

                                    auto aspect_ratio = (window_size.x / window_size.y);
                                    auto width        = ImGui::GetWindowWidth() / ImGui::GetColumnsCount();
                                    auto texture_size = ImVec2(width, (width / aspect_ratio));

                                    // draw the texture name
                                    if (is_loaded) {
                                        ImGui::Text(path.filename().string().c_str());
                                    } else {
                                        static auto red = ImVec4{1, 0, 0, 1};
                                        ImGui::TextColored(red, path.filename().string().c_str());
                                    }

                                    ImGui::BeginGroup();

                                    // draw the texture image
                                    auto srv = is_loaded ? texture->GetSRV()
                                                         : TextureManager::Get()->GetMissingTexture()->GetSRV();
                                    ImGui::Image(srv, texture_size);

                                    // tooltip
                                    if (ImGui::IsItemHovered()) {
                                        ImGui::SetTooltip(path.filename().string().c_str());
                                    }

                                    // open texture preview
                                    if (is_loaded && ImGui::IsItemClicked()) {
                                        TextureManager::Get()->PreviewTexture(texture);
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
                        }

                        ImGui::TreePop();
                    }

                    if (!block_visible)
                        ImGui::PopStyleVar();

                    ++render_block_index;
                }
            }

            // if the close button was pressed, delete the model
            if (!is_still_open) {
                std::lock_guard<std::recursive_mutex> _lk{RenderBlockModel::InstancesMutex};
                it = RenderBlockModel::Instances.erase(it);

                continue;
            }

            ++it;
        }
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
        if (ImGui::Selectable(ICON_FA_SAVE "  Save file")) {
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
        const auto& exporters = ImportExportManager::Get()->GetExporters(filename.extension().string());
        if (exporters.size() > 0) {
            if (ImGui::BeginMenu(ICON_FA_MINUS_CIRCLE "  Export", (exporters.size() > 0))) {
                for (const auto& exporter : exporters) {
                    if (ImGui::MenuItem(exporter->GetName(), exporter->GetExportExtension())) {
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
    static std::atomic_uint64_t unique_ids = {0};
    auto                        id         = ++unique_ids;

    std::lock_guard<std::recursive_mutex> _lk{m_StatusTextsMutex};
    m_StatusTexts[id] = std::move(str);
    return id;
}

void UI::PopStatusText(uint64_t id)
{
    std::lock_guard<std::recursive_mutex> _lk{m_StatusTextsMutex};
    m_StatusTexts.erase(id);
}

const char* UI::GetFileTypeIcon(const fs::path& filename)
{
    const auto& ext = filename.extension();

    if (ext == ".ee" || ext == ".bl" || ext == ".nl" || ext == ".fl")
        return ICON_FA_FILE_ARCHIVE;
    else if (ext == ".dds" || ext == ".ddsc" || ext == ".hmddsc")
        return ICON_FA_FILE_IMAGE;
    else if (ext == ".bank")
        return ICON_FA_FILE_AUDIO;
    else if (ext == ".bikc")
        return ICON_FA_FILE_VIDEO;

    return ICON_FA_FILE;
}

DragDropPayload* UI::GetDropPayload(DragDropPayloadType payload_type)
{
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_123_")) {
            return reinterpret_cast<DragDropPayload*>(payload->Data);
        }

        ImGui::EndDragDropTarget();
    }

    return nullptr;
}

void UI::RegisterContextMenuCallback(const std::vector<std::string>& extensions, ContextMenuCallback fn)
{
    for (const auto& extension : extensions) {
        m_ContextMenuCallbacks[extension] = fn;
    }
}

void UI::DrawText(const std::string& text, const glm::vec3& position, const glm::vec4& colour, bool center)
{
    assert(m_SceneDrawList);

    glm::vec3 screen;
    if (Camera::Get()->WorldToScreen(position, &screen)) {
        if (center) {
            const auto text_size = ImGui::CalcTextSize(text.c_str());
            m_SceneDrawList->AddText(ImVec2{(screen.x - (text_size.x / 2)), (screen.y - (text_size.y / 2))},
                                     ImColor{colour.x, colour.y, colour.z, colour.a}, text.c_str());
        } else {
            m_SceneDrawList->AddText(ImVec2{screen.x, screen.y}, ImColor{colour.x, colour.y, colour.z, colour.a},
                                     text.c_str());
        }
    }
}

void UI::DrawBoundingBox(const BoundingBox& bb, const glm::vec4& colour)
{
    assert(m_SceneDrawList);

    glm::vec3 box[2] = {bb.GetMin(), bb.GetMax()};
    ImVec2    points[8];

    for (auto i = 0; i < 8; ++i) {
        const auto world = glm::vec3{box[(i ^ (i >> 1)) & 1].x, box[(i >> 1) & 1].y, box[(i >> 2) & 1].z};

        glm::vec3 screen;
        Camera::Get()->WorldToScreen(world, &screen);

        points[i] = {screen.x, screen.y};
    }

    const auto col = ImColor{colour.x, colour.y, colour.z, colour.a};

    for (auto i = 0; i < 4; ++i) {
        m_SceneDrawList->AddLine(points[i], points[(i + 1) & 3], col);
        m_SceneDrawList->AddLine(points[4 + i], points[4 + ((i + 1) & 3)], col);
        m_SceneDrawList->AddLine(points[i], points[4 + i], col);
    }
}
