#include <AvaFormatLib.h>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include "directory_list.h"
#include "settings.h"
#include "util.h"
#include "version.h"
#include "window.h"

#include "graphics/camera.h"
#include "graphics/imgui/fonts/fontawesome5_icons.h"
#include "graphics/imgui/imgui_buttondropdown.h"
#include "graphics/imgui/imgui_rotate.h"
#include "graphics/imgui/imgui_tabscrollcontent.h"
#include "graphics/imgui/imgui_textalign.h"
#include "graphics/renderer.h"
#include "graphics/shader_manager.h"
#include "graphics/texture.h"
#include "graphics/texture_manager.h"
#include "graphics/ui.h"

#include "game/file_loader.h"
#include "game/formats/avalanche_archive.h"
#include "game/formats/avalanche_model_format.h"
#include "game/formats/render_block_model.h"
#include "game/formats/runtime_container.h"
#include "game/render_block_factory.h"

#include "game/jc3/renderblocks/irenderblock.h"
#include "game/jc4/renderblocks/irenderblock.h"

#include "import_export/import_export_manager.h"

extern bool g_IsJC4Mode;
extern bool g_DrawBoundingBoxes;
extern bool g_ShowModelLabels;
static bool g_ShowSettingsWindow = false;
static bool g_ShowAboutWindow    = false;
static bool g_ShowNameHashWindow = false;

#ifdef _DEBUG
static bool g_CheckForUpdatesEnabled = false;
static bool g_ShowImGuiDemoWindow    = false;
#else
static bool g_CheckForUpdatesEnabled = true;
#endif

#define ENABLE_ASSET_CREATOR

UI::UI()
{
    Window::Get()->Events().DragEnter.connect([&](const std::vector<std::filesystem::path>& files) {
        m_IsDragDrop                = true;
        ImGui::GetIO().MouseDown[0] = true;

        assert(files.size() > 0);

        if (files.size() > 1) {
            m_DragDropPayload.type = DragDropPayload_Multi;
            m_DragDropTooltip      = std::to_string(files.size()) + " items";
        } else {
            const auto& file  = files.at(0);
            m_DragDropTooltip = file.filename().string();

            // textures
            if (file.extension() == ".dds" || file.extension() == ".ddsc" || file.extension() == ".hmddsc"
                || file.extension() == ".atx1" || file.extension() == ".atx2") {
                m_DragDropPayload.type = DragDropPayload_Texture;
            }
        }

        m_DragDropPayload.data = std::move(files);
    });

    static const auto ResetDragDrop = [&] {
        m_IsDragDrop                = false;
        m_DragDropTooltip           = "";
        ImGui::GetIO().MouseDown[0] = false;
    };

    Window::Get()->Events().DragLeave.connect(ResetDragDrop);
    Window::Get()->Events().DragDropped.connect([&] {
        // unhandled drag drop
        if (!ImGui::IsDragDropPayloadBeingAccepted()) {
            for (const auto& file : m_DragDropPayload.data) {
                if (!std::filesystem::is_directory(file)) {
                    FileLoader::Get()->ReadFileFromDiskAndRunHandlers(file);
                }
            }
        }

        ResetDragDrop();
    });

    // reset scene mouse state
    Window::Get()->Events().FocusLost.connect([&] { m_SceneMouseState.fill(false); });

    // load game selection icons
    FileBuffer buffer;
    if (Window::Get()->LoadInternalResource(101, &buffer)) {
        m_GameSelectionIcons[0] = std::make_unique<Texture>("jc3-icon.dds", &buffer);
    }

    if (Window::Get()->LoadInternalResource(102, &buffer)) {
        m_GameSelectionIcons[1] = std::make_unique<Texture>("jc4-icon.dds", &buffer);
    }
}

void UI::Shutdown()
{
    m_GameSelectionIcons[0].reset();
    m_GameSelectionIcons[1].reset();
}

void UI::Render(RenderContext_t* context)
{
    const auto& window_size = Window::Get()->GetSize();

    if (m_ShowGameSelection) {
        ImGui::OpenPopup("Game Selection");

        if (ImGui::BeginPopupModal("Game Selection", nullptr,
                                   (ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
                                    | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize
                                    | ImGuiWindowFlags_NoBackground))) {
            static const ImVec2 image_size{64, 64};

            ImGuiCustom::TextAligned(ImGuiTextAlign_Center, "Select Game Version");

            // just cause 3
            if (m_GameSelectionIcons[0] ? ImGui::ImageButton(m_GameSelectionIcons[0]->GetSRV(), image_size)
                                        : ImGui::Button("Just Cause 3")) {
                Window::Get()->SwitchMode(GameMode_JustCause3);
                m_ShowGameSelection = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            // just cause 4
            if (m_GameSelectionIcons[1] ? ImGui::ImageButton(m_GameSelectionIcons[1]->GetSRV(), image_size)
                                        : ImGui::Button("Just Cause 4")) {
                Window::Get()->SwitchMode(GameMode_JustCause4);
                m_ShowGameSelection = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        // return. we don't want to render any other UI when game selection is visible
        return;
    }

    // handle external drag drop payloads
    if (m_IsDragDrop && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceExtern)) {
        // only set the payload if we have no data already set
        const auto _drag_drop_payload = ImGui::GetDragDropPayload();
        if (_drag_drop_payload->Data == nullptr) {
            static const auto data = 0x1;
            ImGui::SetDragDropPayload((const char*)&m_DragDropPayload.type, (void*)&data, sizeof(data), ImGuiCond_Once);
        }

        ImGui::Text(m_DragDropTooltip.c_str());
        ImGui::EndDragDropSource();
    }

    RenderMenuBar();

    // settings modal
    {
        // open settings popup
        if (g_ShowSettingsWindow) {
            ImGui::OpenPopup("Settings");
        }

        // settings
        ImGui::SetNextWindowSize({448, 133});
        if (ImGui::BeginPopupModal("Settings", &g_ShowSettingsWindow,
                                   (ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))) {
            static std::string _jc3_path = Settings::Get()->GetValue<const char*>("jc3_directory", "");
            static std::string _jc4_path = Settings::Get()->GetValue<const char*>("jc4_directory", "");

            // select jc3 path
            ImGui::Text("Just Cause 3 Path");
            ImGui::PushItemWidth(405);
            ImGui::InputText("##jc3_path", &_jc3_path, ImGuiInputTextFlags_ReadOnly);
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::Button("...##select_jc3_path")) {
                Window::Get()->SelectJustCauseDirectory(true, true);
                _jc3_path = Settings::Get()->GetValue<const char*>("jc3_directory", "");
            }

            ImGui::Separator();

            // select jc4 path
            ImGui::Text("Just Cause 4 Path");
            ImGui::PushItemWidth(405);
            ImGui::InputText("##jc4_path", &_jc4_path, ImGuiInputTextFlags_ReadOnly);
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::Button("...##select_jc4_path")) {
                Window::Get()->SelectJustCauseDirectory(true, false);
                _jc4_path = Settings::Get()->GetValue<const char*>("jc4_directory", "");
            }

            ImGui::EndPopup();
        }
    }

    // about modal
    {
        // open the about popup
        if (g_ShowAboutWindow) {
            ImGui::OpenPopup("About");
        }

        // About
        if (ImGui::BeginPopupModal("About", &g_ShowAboutWindow,
                                   (ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))) {
            ImGui::SetWindowSize({400, 400});

            static int32_t current_version[3] = {VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION};

            ImGui::Text("Just Cause Model Renderer");
            ImGui::Text("Version %d.%d.%d", current_version[0], current_version[1], current_version[2]);
            ImGui::Text("https://github.com/aaronkirkham/jc-model-renderer");

            ImGui::Separator();

            ImGui::BeginChild("Scrolling", {}, true);

            ImGui::TextWrapped(
                "Permission is hereby granted, free of charge, to any person obtaining a copy of this software and "
                "associated documentation files (the \"Software\"), to deal in the Software without restriction, "
                "including "
                "without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, "
                "and/or "
                "sell "
                "copies of the Software, and to permit persons to whom the Software is furnished to do so, subject "
                "to "
                "the "
                "following conditions:\n\n"

                "The above copyright notice and this permission notice shall be included in all copies or "
                "substantial "
                "portions of the Software.\n\n"

                "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING "
                "BUT "
                "NOT "
                "LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND "
                "NONINFRINGEMENT.IN "
                "NO "
                "EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, "
                "WHETHER "
                "IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE "
                "SOFTWARE "
                "OR "
                "THE USE OR OTHER DEALINGS IN THE SOFTWARE.");

            ImGui::EndChild();

            ImGui::EndPopup();
        }
    }

    // exporter settings modal
    {
        if (m_ExportSettings.ShowExportSettings) {
            ImGui::OpenPopup("Exporter Settings");
        }

        if (ImGui::BeginPopupModal("Exporter Settings", &m_ExportSettings.ShowExportSettings,
                                   (ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse
                                    | ImGuiWindowFlags_AlwaysAutoResize))) {
            if (m_ExportSettings.Exporter->DrawSettingsUI()) {
                std::string status_text    = "Exporting \"" + m_ExportSettings.Filename.generic_string() + "\"...";
                const auto  status_text_id = PushStatusText(status_text);

                const auto& selected = Window::Get()->ShowFolderSelection("Select a folder to export the file to");
                if (!selected.empty()) {
                    m_ExportSettings.Exporter->Export(
                        m_ExportSettings.Filename, selected, [&, selected, status_text_id](bool success) {
                            m_ExportSettings.ShowExportSettings = false;
                            UI::Get()->PopStatusText(status_text_id);
                            if (!success) {
                                SPDLOG_ERROR("Failed to export \"{}\"", m_ExportSettings.Filename.filename().string());
                                Window::Get()->ShowMessageBox("Failed to export \""
                                                              + m_ExportSettings.Filename.filename().string() + "\".");
                            }
                        });
                }
            }

            ImGui::EndPopup();
        }
    }

    // name hash generator
    if (g_ShowNameHashWindow) {
        ImGui::SetNextWindowSize({370, 85}, ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Name Hash Generator", &g_ShowNameHashWindow,
                         (ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))) {
            static std::string _string = "";
            static uint32_t    _hash   = 0;

            if (ImGui::InputText("Text to Hash", &_string)) {
                _hash = _string.length() > 0 ? ava::hashlittle(_string.c_str()) : 0;
            }

            ImGui::InputInt("Result", (int32_t*)&_hash, 0, 0,
                            ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_ReadOnly);
        }
        ImGui::End();
    }

    // texture creator
    if (m_NewTextureSettings.ShowSettings) {
        ImGui::SetNextWindowSize({800, 600}, ImGuiCond_Appearing);
        if (ImGui::Begin(ICON_FA_FILE_IMAGE "  New Texture", &m_NewTextureSettings.ShowSettings,
                         ImGuiWindowFlags_NoCollapse)) {
            ImGui::Columns(2, nullptr, false);
            {
                const auto col_width = (ImGui::GetWindowWidth() / ImGui::GetColumnsCount());

                // draw texture
                ImGui::BeginGroup();
                {
                    if (m_NewTextureSettings.Texture) {
                        ImGui::Text(m_NewTextureSettings.Texture->GetFileName().string().c_str());
                        ImGui::Image(m_NewTextureSettings.Texture->GetSRV(), ImVec2(col_width, col_width), ImVec2(0, 0),
                                     ImVec2(1, 1), ImVec4(1, 1, 1, 1), ImVec4(0, 0, 0, 1));
                    }

                    // open texture preview
                    /*if (texture->IsLoaded() && ImGui::IsItemClicked()) {
                        TextureManager::Get()->PreviewTexture(texture);
                    }*/

                    // import from
                    if (ImGuiCustom::BeginButtonDropDown("Import Texture From")) {
                        const auto& importers = ImportExportManager::Get()->GetImportersFor(".ddsc");
                        for (const auto& importer : importers) {
                            if (ImGui::MenuItem(importer->GetExportName(), importer->GetExportExtension())) {
                                UI::Events().ImportFileRequest(
                                    importer, [&](bool success, std::filesystem::path filename, std::any data) {
                                        if (success) {
                                            auto& buffer                 = std::any_cast<FileBuffer>(data);
                                            m_NewTextureSettings.Texture = TextureManager::Get()->GetTexture(
                                                filename, &buffer,
                                                TextureManager::TextureCreateFlags_CreateIfNotExists);
                                        }
                                    });
                            }
                        }

                        ImGuiCustom::EndButtonDropDown();
                    }
                }
                ImGui::EndGroup();
                ImGui::NextColumn();

                // draw text
                if (m_NewTextureSettings.Texture) {
                    const auto desc = m_NewTextureSettings.Texture->GetDesc();
                    ImGui::Text("Width: %d", desc.Width);
                    ImGui::Text("Height: %d", desc.Height);
                    ImGui::Text("Format: %d (%s)", desc.Format, TextureManager::GetFormatString(desc.Format));
                    ImGui::Text("MipLevels: %d", desc.MipLevels);

                    // save texture as...
                    if (ImGui::Button(ICON_FA_SAVE "  Save as...")) {
                        FileBuffer buffer;
                        if (FileLoader::Get()->WriteAVTX(m_NewTextureSettings.Texture.get(), &buffer)) {
                            FileSelectionParams params{};
                            params.Filename  = m_NewTextureSettings.Texture->GetFileName().stem().string();
                            params.Extension = ".ddsc";
                            params.Filters.emplace_back(FileSelectionFilter{"Avalanche Texture (*.ddsc)", "*.ddsc"});
                            const auto& selected = Window::Get()->ShowSaveDialog("Save as...", params);

                            if (!selected.empty()) {
                                std::ofstream stream(selected, std::ios::binary);
                                if (stream.fail()) {
                                    Window::Get()->ShowMessageBox("Failed to save file.", MB_ICONEXCLAMATION | MB_OK);
                                    return;
                                }

                                stream.write((char*)buffer.data(), buffer.size());
                                stream.close();
                            }
                        }
                    }
                }
            }
            ImGui::EndColumns();
        }
        ImGui::End();
    }

    // select importer
    {
        static bool             importer_settings_open = m_SelectImporterSettings.ShowImporterSettings;
        static IImportExporter* importer_to_use        = nullptr;
        if (m_SelectImporterSettings.ShowImporterSettings) {
            ImGui::OpenPopup("Select Importer");
            importer_settings_open = true;
        }

        if (ImGui::BeginPopupModal("Select Importer", &importer_settings_open,
                                   (ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse
                                    | ImGuiWindowFlags_AlwaysAutoResize))) {

            ImGui::Text("The file type you're trying to import has multiple importers available.");
            ImGui::Text("Please select the format you wish to import into.\n\n");
            ImGui::Text("Filename: %s", m_SelectImporterSettings.Filename.filename().string().c_str());

            ImGuiWindow* window = ImGui::GetCurrentWindowRead();
            ImGui::PushItemWidth(window->Size.x - (window->WindowPadding.x * 2));

            if (ImGui::BeginCombo("##select-importer-combo", "Select Importer")) {
                for (const auto& importer : m_SelectImporterSettings.Importers) {
                    const auto selected = ImGui::Selectable(importer->GetImportName());

                    if (ImGui::IsItemHovered()) {
                        const auto& exts = util::join(importer->GetImportExtension(), ", ");
                        ImGui::SetTooltip(exts.c_str());
                    }

                    if (selected) {
                        importer_settings_open = false;
                        importer_to_use        = importer;
                    }
                }

                ImGui::EndCombo();
            }

            ImGui::EndPopup();
        }

        // reset after popup is closed
        if (!importer_settings_open && m_SelectImporterSettings.ShowImporterSettings) {
            if (importer_to_use) {
                importer_to_use->Import(m_SelectImporterSettings.Filename, m_SelectImporterSettings.Callback);
                importer_to_use = nullptr;
            }

            m_SelectImporterSettings.ShowImporterSettings = false;
            m_SelectImporterSettings.Filename             = "";
            m_SelectImporterSettings.Importers.clear();
        }
    }

    RenderSceneView(window_size);
    RenderFileTreeView(window_size);

    // Status
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::SetNextWindowSize({window_size.x, window_size.y});
    ImGui::Begin("Status", nullptr,
                 (ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoScrollbar));
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

    // runtime container editor
    for (auto it = RuntimeContainer::Instances.begin(); it != RuntimeContainer::Instances.end();) {
        std::string title = "Runtime Container Editor - " + (*it).second->GetFilePath().filename().string();

        bool open = true;
        ImGui::SetNextWindowSize({800, 600}, ImGuiCond_Appearing);
        if (ImGui::Begin(title.c_str(), &open, ImGuiWindowFlags_NoCollapse)) {
            (*it).second->DrawUI();
            ImGui::End();
        }

        if (!open) {
            std::lock_guard<std::recursive_mutex> _lk{RuntimeContainer::InstancesMutex};
            it = RuntimeContainer::Instances.erase(it);
        } else
            ++it;
    }

#ifdef _DEBUG
    if (g_ShowImGuiDemoWindow) {
        ImGui::ShowDemoWindow(&g_ShowImGuiDemoWindow);
    }
#endif
}

void UI::RenderMenuBar()
{
    if (ImGui::BeginMainMenuBar()) {
        m_MainMenuBarHeight = ImGui::GetWindowHeight();

        // file
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem(ICON_FA_GAMEPAD "  Select Game", nullptr, nullptr, !FileLoader::m_DictionaryLoading)) {
                ShowGameSelection();
            }

            if (ImGui::MenuItem(ICON_FA_COG "  Settings")) {
                g_ShowSettingsWindow = !g_ShowSettingsWindow;
            }

            if (ImGui::MenuItem(ICON_FA_WINDOW_CLOSE "  Exit")) {
                Window::Get()->BeginShutdown();
            }

            ImGui::EndMenu();
        }

        // create
#ifdef ENABLE_ASSET_CREATOR
        if (ImGui::BeginMenu("Create")) {
            if (ImGui::MenuItem("Avalanche Archive", ".ee")) {
                AvalancheArchive::make("new.ee");
            }

            if (ImGui::MenuItem("Render Block Model", ".rbm", nullptr, !g_IsJC4Mode)) {
                RenderBlockModel::make("new.rbm");
            }

            if (ImGui::MenuItem("Texture", ".ddsc")) {
                m_NewTextureSettings.Texture      = nullptr;
                m_NewTextureSettings.ShowSettings = true;
            }

            if (ImGui::MenuItem("Resource Bundle", ".resourcebundle", nullptr, g_IsJC4Mode)) {
                const auto& importers = ImportExportManager::Get()->GetImportersFor(".resourcebundle");
                if (importers.size() > 0) {
                    const auto& selected = Window::Get()->ShowFolderSelection("Select a folder to import");
                    std::thread([&, importer = importers.at(0), selected] { importer->Import(selected); }).detach();
                }
            }

            if (ImGui::MenuItem("Archive Table", ".arc & .tab")) {
                const auto& importers = ImportExportManager::Get()->GetImportersFor(".arc");
                if (importers.size() > 0) {
                    const auto& selected = Window::Get()->ShowFolderSelection("Select a folder to import");
                    std::thread([&, importer = importers.at(0), selected] { importer->Import(selected); }).detach();
                }
            }

            ImGui::EndMenu();
        }
#endif

        // renderer
        if (ImGui::BeginMenu("Renderer")) {
            if (ImGui::BeginMenu(ICON_FA_EYE "  Visualize")) {
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

            // camera
            if (ImGui::BeginMenu(ICON_FA_CAMERA "  Camera")) {
                if (ImGui::BeginMenu("Focus On", (RenderBlockModel::Instances.size() > 0
                                                  || AvalancheModelFormat::Instances.size() > 0))) {
                    for (const auto& model : RenderBlockModel::Instances) {
                        if (ImGui::MenuItem(model.second->GetFileName().c_str())) {
                            Camera::Get()->FocusOn(model.second->GetBoundingBox());
                        }
                    }

                    // TODO: generate a bounding box for the whole model, instead of using the sub-mesh boxes.
                    for (const auto& model : AvalancheModelFormat::Instances) {
                        /*if (ImGui::MenuItem(model.second->GetFileName().c_str())) {
                            Camera::Get()->FocusOn(model.second->GetBoundingBox());
                        }*/

                        /*
                        for (const auto& render_block : model.second->GetRenderBlocks()) {
                            const auto& name = ((jc4::IRenderBlock*)render_block)->m_Name;
                            if (ImGui::MenuItem(name)) {
                                Camera::Get()->FocusOn(model.second->GetBoundingBox());
                            }
                        }
                        */
                    }

                    ImGui::EndMenu();
                }

                ImGui::EndMenu();
            }

            static bool wireframe = false;
            if (ImGui::MenuItem(ICON_FA_SHAPES "  Wireframe", nullptr, wireframe)) {
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

        // tools
        if (ImGui::BeginMenu("Tools")) {
            if (ImGui::MenuItem("Name Hash Generator")) {
                g_ShowNameHashWindow = !g_ShowNameHashWindow;
            }

            ImGui::EndMenu();
        }

        // help
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem(ICON_FA_INFO_CIRCLE "  About")) {
                g_ShowAboutWindow = !g_ShowAboutWindow;
            }

            if (ImGui::MenuItem(ICON_FA_SYNC "  Check for updates", nullptr, false, g_CheckForUpdatesEnabled)) {
                Window::Get()->CheckForUpdates(true);
            }

            if (ImGui::MenuItem(ICON_FA_EXTERNAL_LINK_ALT "  View on GitHub")) {
                Window::Get()->OpenUrl("https://github.com/aaronkirkham/jc-model-renderer");
            }

            if (ImGui::MenuItem(ICON_FA_BUG "  Report a Bug")) {
                Window::Get()->OpenUrl("https://github.com/aaronkirkham/jc-model-renderer/issues/new");
            }

#ifdef _DEBUG
            if (ImGui::MenuItem("Show Demo Window")) {
                g_ShowImGuiDemoWindow = !g_ShowImGuiDemoWindow;
            }
#endif

            ImGui::EndMenu();
        }

        // fps counter
        {
            static char buffer[16];
            sprintf_s(buffer, sizeof(buffer), "%.01f fps", ImGui::GetIO().Framerate);

            const auto& text_size = ImGui::CalcTextSize(buffer);
            ImGui::SameLine(ImGui::GetWindowWidth() - text_size.x - 20);
            ImGui::TextColored(ImVec4(1, 1, 1, .5), buffer);
        }

        ImGui::EndMainMenuBar();
    }
}

void UI::RenderSceneView(const glm::vec2& window_size)
{
    ImGui::SetNextWindowPos({0, m_MainMenuBarHeight});
    ImGui::SetNextWindowSize({window_size.x - m_SidebarWidth, window_size.y - m_MainMenuBarHeight});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});

    const auto result = ImGui::Begin("Scene", nullptr,
                                     (ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
                                      | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus));

    ImGui::PopStyleVar();
    if (result) {
        const auto& size = ImGui::GetWindowSize();
        m_SceneWidth     = size.x;

        // update camera projection if needed
        Camera::Get()->UpdateViewport({size.x, size.y});

        // render gbuffer texture
        ImGui::Image(Renderer::Get()->GetGBufferSRV(m_CurrentActiveGBuffer), ImGui::GetWindowSize());

        // drag drop target
        if (const auto payload = UI::Get()->GetDropPayload()) {
            for (const auto& file : payload->data) {
                if (!std::filesystem::is_directory(file)) {
                    FileLoader::Get()->ReadFileFromDiskAndRunHandlers(file);
                }
            }
        }

        m_SceneDrawList = ImGui::GetWindowDrawList();

        // handle input
        {
            const auto& io        = ImGui::GetIO();
            const auto& mouse_pos = glm::vec2(io.MousePos.x, io.MousePos.y);

            // because we manually toggle the mouse down when drag/dropping from an external source, we need
            // to check this or the camera will be moved
            if (!UI::Get()->IsDragDropping()) {
                // handle mouse press
                for (int i = 0; i < 2; ++i) {
                    if (ImGui::IsMouseDown(i)) {
                        if (!m_SceneMouseState[i] && ImGui::IsItemHovered()) {
                            m_SceneMouseState[i] = true;
                            Camera::Get()->OnMousePress(i, true, mouse_pos);
                        }
                        // handle mouse move
                        else if (m_SceneMouseState[i]) {
                            Camera::Get()->OnMouseMove({-io.MouseDelta.x, -io.MouseDelta.y});
                        }
                    }
                    // handle mouse release
                    else if (m_SceneMouseState[i]) {
                        m_SceneMouseState[i] = false;
                        Camera::Get()->OnMousePress(i, false, mouse_pos);
                    }
                }

                // handle mouse scroll
                if (io.MouseWheel != 0 && ImGui::IsItemHovered()) {
                    Camera::Get()->OnMouseScroll(io.MouseWheel);
                }
            }

#if 0
            const auto render_block = Camera::Get()->Pick(mouse_pos);
            if (render_block) {
                ImGui::SetTooltip(render_block->GetFileName().c_str());
            }
#endif
        }
    }

    ImGui::End();
}

// TODO: move the texture view stuff into here.
void UI::RenderFileTreeView(const glm::vec2& window_size)
{
    // render the archive directory list
    ImGui::SetNextWindowBgAlpha(1.0f);
    ImGui::SetNextWindowPos({m_SceneWidth, m_MainMenuBarHeight});
    ImGui::SetNextWindowSizeConstraints({MIN_SIDEBAR_WIDTH, (window_size.y - m_MainMenuBarHeight)},
                                        {window_size.x / 2, (window_size.y - m_MainMenuBarHeight)});

    // hide resize grip
    // TODO: can probably remove if we can use the NoResize flag with ResizeFromAnySize
    ImGui::PushStyleColor(ImGuiCol_ResizeGrip, {0, 0, 0, 0});
    ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, {0, 0, 0, 0});
    ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, {0, 0, 0, 0});

    ImGui::Begin("Tree View", nullptr,
                 (ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse
                  | ImGuiWindowFlags_NoBringToFrontOnFocus));
    {
        m_SidebarWidth = ImGui::GetWindowSize().x;

        ImGui::BeginTabBar("Tree View Tabs");
        {
            // file explorer tab
            if (ImGuiCustom::TabItemScroll("File Explorer", m_TabToSwitch == TreeViewTab_FileExplorer)) {
                const auto directory_list = FileLoader::Get()->GetDirectoryList();
                if (directory_list) {
                    directory_list->Draw();
                }

                ImGuiCustom::EndTabItemScroll();

                if (m_TabToSwitch == TreeViewTab_FileExplorer) {
                    m_TabToSwitch = TreeViewTab_Total;
                }
            }

            // archives
            if (ImGuiCustom::TabItemScroll("Archives", m_TabToSwitch == TreeViewTab_Archives, nullptr,
                                           AvalancheArchive::Instances.size() == 0 ? ImGuiItemFlags_Disabled : 0)) {
                for (auto it = AvalancheArchive::Instances.begin(); it != AvalancheArchive::Instances.end();) {
                    const auto& archive  = (*it).second;
                    const auto& filepath = archive->GetFilePath();
                    const auto& filename = filepath.filename();

                    // render the current directory
                    bool is_still_open = true;
                    auto open          = ImGui::CollapsingHeader(filename.string().c_str(), &is_still_open);

                    // drag drop target
                    if (const auto payload = UI::Get()->GetDropPayload()) {
                        for (const auto& file : payload->data) {
                            archive->AddFile(file, file.parent_path());
                        }
                    }

                    // context menu
                    RenderContextMenu(filepath, 0, ContextMenuFlags_Archive);

                    if (open && archive->GetDirectoryList()) {
                        // draw the directory list
                        ImGui::PushID(filename.string().c_str());
                        archive->GetDirectoryList()->Draw(archive.get());
                        ImGui::PopID();
                    }

                    // if the close button was pressed, delete the archive
                    if (!is_still_open) {
                        std::lock_guard<std::recursive_mutex> _lk{AvalancheArchive::InstancesMutex};
                        it = AvalancheArchive::Instances.erase(it);

                        // flush texture and shader manager
                        TextureManager::Get()->Flush();
                        ShaderManager::Get()->Empty();

                        if (AvalancheArchive::Instances.size() == 0) {
                            m_TabToSwitch = TreeViewTab_FileExplorer;
                        }

                        continue;
                    }

                    ++it;
                }

                ImGuiCustom::EndTabItemScroll();
                if (m_TabToSwitch == TreeViewTab_Archives) {
                    m_TabToSwitch = TreeViewTab_Total;
                }
            }

            // models
            if (g_IsJC4Mode) {
                RenderModelsTab_AMF();
            } else {
                RenderModelsTab_RBM();
            }
        }
        ImGui::EndTabBar();
    }
    ImGui::End();

    ImGui::PopStyleColor(3);
}

void UI::RenderModelsTab_RBM()
{
    if (ImGuiCustom::TabItemScroll("Models", m_TabToSwitch == TreeViewTab_Models, nullptr,
                                   RenderBlockModel::Instances.size() == 0 ? ImGuiItemFlags_Disabled : 0)) {
        for (auto it = RenderBlockModel::Instances.begin(); it != RenderBlockModel::Instances.end();) {
            const auto& filename_with_path = (*it).second->GetPath();
            const auto& filename           = (*it).second->GetFileName();

            // render the current model info
            bool       is_still_open = true;
            const auto open          = ImGui::CollapsingHeader(filename.c_str(), &is_still_open);

            // context menu
            RenderContextMenu(filename_with_path);

            if (open) {
                auto& render_blocks = (*it).second->GetRenderBlocks();

#ifdef ENABLE_ASSET_CREATOR
                if (render_blocks.empty()) {
                    static auto red = ImVec4{1, 0, 0, 1};
                    ImGui::TextColored(red, "This model doesn't have any Render Blocks!");

                    if (ImGuiCustom::BeginButtonDropDown("Add Render Block")) {
                        // show available render blocks
                        for (const auto& block_name : RenderBlockFactory::GetValidRenderBlocks()) {
                            if (ImGui::MenuItem(block_name)) {
                                const auto render_block = RenderBlockFactory::CreateRenderBlock(block_name);
                                if (render_block) {
                                    render_block->SetOwner((*it).second.get());
                                    render_blocks.emplace_back(render_block);
                                } else {
                                    SPDLOG_ERROR("Failed to create render block with type \"{}\"!", block_name);
                                }
                            }
                        }

                        ImGuiCustom::EndButtonDropDown();
                    }
                }
#endif

                uint32_t index = 0;
                for (const auto& render_block : render_blocks) {
                    // unique id
                    std::string label(render_block->GetTypeName());
                    label.append("##" + filename + "-" + std::to_string(index));

                    // make the things transparent if the block isn't rendering
                    const auto block_visible = render_block->IsVisible();
                    if (!block_visible)
                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.35f);

                    // current block header
                    const auto tree_open = ImGui::TreeNode(label.c_str());

                    // draw render block context menu
                    if (ImGui::BeginPopupContextItem()) {
                        render_block->DrawContextMenu();
                        ImGui::EndPopup();
                    }

                    // render the current render block info
                    if (tree_open) {
                        // show import button if the block doesn't have a vertex buffer
                        if (!render_block->GetVertexBuffer()) {
#ifdef ENABLE_ASSET_CREATOR
                            static auto red = ImVec4{1, 0, 0, 1};
                            ImGui::TextColored(red, "This Render Block doesn't have any mesh!");

                            if (!g_IsJC4Mode) {
                                if (ImGuiCustom::BeginButtonDropDown("Import Mesh From")) {
                                    const auto& importers = ImportExportManager::Get()->GetImportersFor(".rbm");
                                    for (const auto& importer : importers) {
                                        if (ImGui::MenuItem(importer->GetExportName(),
                                                            importer->GetExportExtension())) {
                                            UI::Events().ImportFileRequest(importer, [&](bool                  success,
                                                                                         std::filesystem::path filename,
                                                                                         std::any              data) {
                                                if (success) {
                                                    auto& [vertices, indices, materials] =
                                                        std::any_cast<std::tuple<vertices_t, uint16s_t, materials_t>>(
                                                            data);

                                                    // @TODO: just pass std::any into SetData..
                                                    render_block->SetData(&vertices, &indices, &materials);
                                                    ((jc3::IRenderBlock*)render_block)->Create();

                                                    // @TODO: should be handled by the RenderBlockModel class.
                                                    Renderer::Get()->AddToRenderList(render_block);

                                                    // Update model bounding box
                                                    render_block->GetOwner()->RecalculateBoundingBox();
                                                }
                                            });
                                        }
                                    }

                                    ImGuiCustom::EndButtonDropDown();
                                }
                            }
#endif
                        } else {
                            // draw model attributes ui
                            render_block->DrawUI();
                        }

                        ImGui::TreePop();
                    }

                    ++index;
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

        ImGuiCustom::EndTabItemScroll();
        if (m_TabToSwitch == TreeViewTab_Models) {
            m_TabToSwitch = TreeViewTab_Total;
        }
    }
}

void UI::RenderModelsTab_AMF()
{
    if (ImGuiCustom::TabItemScroll("Models", m_TabToSwitch == TreeViewTab_Models, nullptr,
                                   AvalancheModelFormat::Instances.size() == 0 ? ImGuiItemFlags_Disabled : 0)) {
        for (auto it = AvalancheModelFormat::Instances.begin(); it != AvalancheModelFormat::Instances.end();) {
            const auto& filename_with_path = (*it).second->GetPath();
            const auto& filename           = (*it).second->GetFileName();

            // render the current model info
            bool       is_still_open = true;
            const auto open          = ImGui::CollapsingHeader(filename.c_str(), &is_still_open);

            // context menu
            RenderContextMenu(filename_with_path);

            if (open) {
                const auto& render_blocks = (*it).second->GetRenderBlocks();

                if (render_blocks.size() == 0) {
                    static auto red = ImVec4{1, 0, 0, 1};
                    ImGui::TextColored(red, "This model doesn't have any Render Blocks!");
                }

                uint32_t index = 0;
                for (const auto& render_block : render_blocks) {
                    // unique id
                    std::string label(((jc4::IRenderBlock*)render_block)->m_Name);
                    label.append("##" + filename + "-" + std::to_string(index));

                    // current block header
                    const auto tree_open = ImGui::TreeNode(label.c_str());

                    // draw render block context menu
                    if (ImGui::BeginPopupContextItem()) {
                        render_block->DrawContextMenu();
                        ImGui::EndPopup();
                    }

                    if (tree_open) {
                        // draw model attributes ui
                        render_block->DrawUI();

                        ImGui::TreePop();
                    }

                    ++index;
                }
            }

            // if the close button was pressed, delete the model
            if (!is_still_open) {
                std::lock_guard<std::recursive_mutex> _lk{AvalancheModelFormat::InstancesMutex};
                it = AvalancheModelFormat::Instances.erase(it);

                continue;
            }

            ++it;
        }

        ImGuiCustom::EndTabItemScroll();
        if (m_TabToSwitch == TreeViewTab_Models) {
            m_TabToSwitch = TreeViewTab_Total;
        }
    }
}

void UI::RenderSpinner(const std::string& str)
{
    ImGuiCustom::ImRotateStart();
    ImGui::Text(ICON_FA_SPINNER);
    ImGuiCustom::ImRotateEnd(-0.005f * GetTickCount64());

    ImGui::SameLine();
    ImGui::Text(str.c_str());
}

void UI::RenderContextMenu(const std::filesystem::path& filename, uint32_t unique_id_extra, uint32_t flags)
{
    std::string unique_id = "context-menu-" + filename.generic_string() + "-" + std::to_string(unique_id_extra);

    ImGui::PushID(unique_id.c_str());
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1);

    if (ImGui::BeginPopupContextItem("Context Menu")) {
        // save file
        if (ImGui::Selectable(ICON_FA_SAVE "  Save to...")) {
            const auto& selected = Window::Get()->ShowFolderSelection("Select a folder to save the file to");
            UI::Get()->Events().SaveFileRequest(filename, selected, false);
        }

        // save to dropzone
        if (ImGui::Selectable(ICON_FA_SAVE "  Save to dropzone")) {
            auto dropzone = Window::Get()->GetJustCauseDirectory() / "dropzone";
            UI::Get()->Events().SaveFileRequest(filename, dropzone, true);
        }

        // exporters
        const auto& exporters = ImportExportManager::Get()->GetExportersFor(filename.extension().string());
        if (exporters.size() > 0) {
            ImGui::Separator();

            if (ImGui::BeginMenu(ICON_FA_MINUS_CIRCLE "  Export", (exporters.size() > 0))) {
                for (const auto& exporter : exporters) {
                    if (ImGui::MenuItem(exporter->GetExportName(), exporter->GetExportExtension())) {
                        if (exporter->HasSettingsUI()) {
                            m_ExportSettings.Exporter           = exporter;
                            m_ExportSettings.Filename           = filename;
                            m_ExportSettings.ShowExportSettings = true;
                        } else {
                            UI::Get()->Events().ExportFileRequest(filename, exporter);
                        }
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
            ImGui::Separator();
            (*it).second(filename);
        }

        ImGui::EndPopup();
    }

    ImGui::PopStyleVar();
    ImGui::PopID();
}

void UI::RenderBlockTexture(IRenderBlock* render_block, const std::string& title, std::shared_ptr<Texture> texture)
{
    // TODO: Render an empty box if the texture isn't loaded. In some cases the slot doesn't
    //			have a texture loaded, but supports it. Drag & Drop should work!!

    if (!texture) {
        return;
    }

    const auto  col_width          = (ImGui::GetWindowWidth() / ImGui::GetColumnsCount());
    const auto& filename_with_path = texture->GetFileName();

    ImGui::BeginGroup();
    {
        ImGui::Text(title.c_str());
        ImGui::Image(texture->GetSRV(), ImVec2(col_width, col_width / 2), ImVec2(0, 0), ImVec2(1, 1),
                     ImVec4(1, 1, 1, 1), ImVec4(0, 0, 0, 1));

        // tooltip
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(filename_with_path.filename().string().c_str());
        }

        // open texture preview
        if (texture->IsLoaded() && ImGui::IsItemClicked()) {
            TextureManager::Get()->PreviewTexture(texture);
        }

        // context menu
        RenderContextMenu(filename_with_path, ImGui::GetColumnIndex(), ContextMenuFlags_Texture);

        // dragdrop payload
        if (const auto payload = UI::Get()->GetDropPayload(DragDropPayload_Texture)) {
            SPDLOG_INFO("DropPayload Texture: \"{}\"", title);
            assert(payload->data.size() > 0);

            const auto& texture_filename = payload->data[0];
            texture->LoadFromFile(texture_filename);

            // generate the new file path
            const auto owner = render_block->GetOwner();
            if (owner) {
                auto filename = owner->GetPath().parent_path();
                filename /= "textures" / texture_filename.filename();

                // update the texture name
                texture->SetFileName(filename);

                // add the file to the parent archive
                const auto archive = owner->GetParentArchive();
                if (archive) {
                    archive->AddFile(filename, texture->GetBuffer());
                }
            }
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

DragDropPayload* UI::GetDropPayload()
{
    DragDropPayload* result = nullptr;
    if (ImGui::BeginDragDropTarget()) {
        const ImGuiPayload* payload = ImGui::GetDragDropPayload();
        if (payload && payload->Data != nullptr && payload->DataFrameCount != -1) {
            if (const ImGuiPayload* accepted_payload = ImGui::AcceptDragDropPayload(payload->DataType)) {
                result = &m_DragDropPayload;
            }
        }

        ImGui::EndDragDropTarget();
    }

    return result;
}

DragDropPayload* UI::GetDropPayload(DragDropPayloadType type)
{
    DragDropPayload* result = nullptr;
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* accepted_payload = ImGui::AcceptDragDropPayload((const char*)&type)) {
            result = &m_DragDropPayload;
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
    assert(m_SceneDrawList);

    const auto& screen = Camera::Get()->WorldToScreen(position);
    if (center) {
        const auto text_size = ImGui::CalcTextSize(text.c_str());
        m_SceneDrawList->AddText(ImVec2{(screen.x - (text_size.x / 2)), (screen.y - (text_size.y / 2))},
                                 ImColor{colour.x, colour.y, colour.z, colour.a}, text.c_str());
    } else {
        m_SceneDrawList->AddText(ImVec2{screen.x, screen.y}, ImColor{colour.x, colour.y, colour.z, colour.a},
                                 text.c_str());
    }
}

void UI::DrawBoundingBox(const BoundingBox& bb, const glm::vec4& colour)
{
    assert(m_SceneDrawList);

    glm::vec3 box[2] = {bb.GetMin(), bb.GetMax()};
    ImVec2    points[8];

    for (auto i = 0; i < 8; ++i) {
        const auto world = glm::vec3{box[(i ^ (i >> 1)) & 1].x, box[(i >> 1) & 1].y, box[(i >> 2) & 1].z};

        const auto& screen = Camera::Get()->WorldToScreen(world);
        points[i]          = {screen.x, screen.y};
    }

    const auto col = ImColor{colour.x, colour.y, colour.z, colour.a};

    for (auto i = 0; i < 4; ++i) {
        m_SceneDrawList->AddLine(points[i], points[(i + 1) & 3], col);
        m_SceneDrawList->AddLine(points[4 + i], points[4 + ((i + 1) & 3)], col);
        m_SceneDrawList->AddLine(points[i], points[4 + i], col);
    }
}
