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
static bool     g_ShowAboutWindow = false;

#ifdef DEBUG
static bool g_CheckForUpdatesEnabled = false;
#else
static bool g_CheckForUpdatesEnabled = true;
#endif

UI::UI()
{
    Window::Get()->Events().DragEnter.connect([&](const fs::path& filename) {
        m_IsDragDrop                = true;
        m_DragDropPayload           = filename.generic_string();
        ImGui::GetIO().MouseDown[0] = true;
    });

    static const auto ResetDragDrop = [&] {
        m_IsDragDrop                = false;
        ImGui::GetIO().MouseDown[0] = false;
    };

    Window::Get()->Events().DragLeave.connect(ResetDragDrop);
    Window::Get()->Events().DragDropped.connect(ResetDragDrop);

    // add widgets
    m_Widgets.emplace_back(std::make_unique<Widget_FileExplorer>());
    m_Widgets.emplace_back(std::make_unique<Widget_ArchiveExplorer>());
    m_Widgets.emplace_back(std::make_unique<Widget_ModelExplorer>());
    m_Widgets.emplace_back(std::make_unique<Widget_Viewport>());
}

void UI::Render(RenderContext_t* context)
{
    const auto viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr,
                 ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar
                     | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
                     | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus
                     | ImGuiWindowFlags_NoBackground);
    ImGui::PopStyleVar();

    // handle external drag drop payloads
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

        // fps counter
        {
            char buffer[10];
            sprintf(buffer, "%.01f fps", ImGui::GetIO().Framerate);

            const auto& text_size = ImGui::CalcTextSize(buffer);
            ImGui::SameLine(ImGui::GetWindowWidth() - text_size.x - 10);
            ImGui::TextColored(ImVec4(1, 1, 1, .5), buffer);
        }

        ImGui::EndMenuBar();
    }

    ImGui::End();

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

#if 0
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

void UI::RenderBlockTexture(const std::string& title, Texture* texture)
{
    assert(texture);

    const auto  col_width          = (ImGui::GetWindowWidth() / ImGui::GetColumnsCount());
    const auto& filename_with_path = texture->GetPath();

    ImGui::BeginGroup();
    {
        ImGui::Text(title.c_str());
        ImGui::Image(texture->GetSRV(), ImVec2(col_width, col_width / 2), ImVec2(0, 0), ImVec2(1, 1),
                     ImVec4(1, 1, 1, 1), ImVec4(0, 0, 0, 1));

        // tooltip
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(texture->GetPath().filename().string().c_str());
        }

        // open texture preview
        /*if (is_loaded && ImGui::IsItemClicked()) {
            TextureManager::Get()->PreviewTexture(texture);
        }*/

        // context menu
        RenderContextMenu(filename_with_path, ImGui::GetColumnIndex(), ContextMenuFlags::CTX_TEXTURE);

        // dragdrop payload
        if (const auto payload = UI::Get()->GetDropPayload(DROPPAYLOAD_UNKNOWN)) {
            DEBUG_LOG("DropPayload (" << title << "): " << payload->data);
            texture->LoadFromFile(payload->data);
        }
    }
    ImGui::EndGroup();
    ImGui::NextColumn();
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
    DragDropPayload* result = nullptr;
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_123_")) {
            result = reinterpret_cast<DragDropPayload*>(payload->Data);
        }

        ImGui::EndDragDropTarget();
    }

    return result;
}

void UI::RegisterContextMenuCallback(const std::vector<std::string>& extensions, ContextMenuCallback fn)
{
    for (const auto& extension : extensions) {
        m_ContextMenuCallbacks[extension] = fn;
    }
}

void UI::DrawText(const std::string& text, const glm::vec3& position, const glm::vec4& colour, bool center)
{
    assert(SceneDrawList);

    const auto& screen = Camera::Get()->WorldToScreen(position);
    if (center) {
        const auto text_size = ImGui::CalcTextSize(text.c_str());
        SceneDrawList->AddText(ImVec2{(screen.x - (text_size.x / 2)), (screen.y - (text_size.y / 2))},
                               ImColor{colour.x, colour.y, colour.z, colour.a}, text.c_str());
    } else {
        SceneDrawList->AddText(ImVec2{screen.x, screen.y}, ImColor{colour.x, colour.y, colour.z, colour.a},
                               text.c_str());
    }
}

void UI::DrawBoundingBox(const BoundingBox& bb, const glm::vec4& colour)
{
    assert(SceneDrawList);

    glm::vec3 box[2] = {bb.GetMin(), bb.GetMax()};
    ImVec2    points[8];

    for (auto i = 0; i < 8; ++i) {
        const auto world = glm::vec3{box[(i ^ (i >> 1)) & 1].x, box[(i >> 1) & 1].y, box[(i >> 2) & 1].z};

        const auto& screen = Camera::Get()->WorldToScreen(world);
        points[i]          = {screen.x, screen.y};
    }

    const auto col = ImColor{colour.x, colour.y, colour.z, colour.a};

    for (auto i = 0; i < 4; ++i) {
        SceneDrawList->AddLine(points[i], points[(i + 1) & 3], col);
        SceneDrawList->AddLine(points[4 + i], points[4 + ((i + 1) & 3)], col);
        SceneDrawList->AddLine(points[i], points[4 + i], col);
    }
}
