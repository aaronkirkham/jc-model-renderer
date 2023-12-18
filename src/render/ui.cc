#include "pch.h"

#include "ui.h"

#include "app/app.h"
#include "app/os.h"

#include "game/format.h"

#include "render/fonts/fa_solid_900.h"
#include "render/fonts/icons.h"
#include "render/fonts/poppins_600.h"
#include "render/imguiex.h"
#include "render/renderer.h"

#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_win32.h>
#include <imgui_internal.h>

namespace jcmr
{
struct UIImpl final : UI {
    UIImpl(App& app, Renderer& renderer)
        : m_app(app)
        , m_renderer(renderer)
    {
    }

    void init(os::WindowHandle window_handle) override
    {
        auto& context = m_renderer.get_context();

        // renderer.
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplWin32_Init(window_handle);
        ImGui_ImplDX11_Init(context.device, context.device_context);

        auto& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        // load fonts
        const float font_size = (16.0f * (os::get_dpi() / 96.0f));
        io.Fonts->AddFontFromMemoryCompressedTTF(poppins600_compressed_data, poppins600_compressed_size, font_size);

        // merge font awesome icons
        {
            ImFontConfig merge_cfg;
            strcpy_s(merge_cfg.Name, "fa-solid-900.ttf");
            merge_cfg.MergeMode            = true;
            merge_cfg.FontDataOwnedByAtlas = false;
            merge_cfg.GlyphMinAdvanceX     = font_size;

            static const ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};

            io.Fonts->AddFontFromMemoryCompressedTTF(fasolid900_compressed_data, fasolid900_compressed_size,
                                                     (font_size * 0.7f), &merge_cfg, icon_ranges);
        }
    }

    void shutdown() override
    {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    void start_frame() override
    {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void render(RenderContext& context) override
    {
        // ImGui::ShowDemoWindow();

        draw_dockspace();
        draw_main_menu();
        draw_hash_generator_widget();

        // render callbacks
        for (auto& callback : m_render_callbacks) {
            callback(context);
        }

        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    void draw_context_menu(const std::string& filename, game::IFormat* format, u32 flags) override
    {
        // TODO : how do we get the base colour without any PushStyleColor() overriding?
        ImGui::PushStyleColor(ImGuiCol_Text, {1, 1, 1, 1});

        if (ImGui::BeginPopupContextItem()) {
            // try get the handler from the file header magic, if a valid format handler wasn't passed
            if ((flags & E_CONTEXT_FILE) && !format) {
                // TODO : if this fails, don't keep trying.
                format = context_menu_get_format_handler_for_file(filename);
            }

            // save file
            if (ImGui::Selectable(ICON_FA_SAVE " Save to...")) {
                std::filesystem::path path;
                if (os::get_open_folder(&path, os::FileDialogParams{})) {
                    m_app.save_file(format, filename, path);
                }
            }

            // save to dropzone
            if (ImGui::Selectable(ICON_FA_SAVE " Save to dropzone")) {
                m_app.save_file(format, filename, "dropzone");
            }

            // format specific
            if (format) {
                // format exporter
                if (format->can_be_exported()) {
                    ImGui::Separator();

                    // TOOD: format->has_export_settings();

                    if (ImGui::Selectable(ICON_FA_FILE_EXPORT " Export")) {
                        std::filesystem::path path;
                        if (os::get_open_folder(&path, os::FileDialogParams{})) {
                            format->export_to(filename, path);
                        }
                    }
                }

                // context menu callbacks
                format->context_menu_handler(filename);
            }

            ImGui::EndPopup();
        }

        ImGui::PopStyleColor();
    }

    void on_render(RenderCallback_t callback) override { m_render_callbacks.push_back(callback); }

    const u32 get_dockspace_id(DockSpacePosition position) const override
    {
        switch (position) {
            case E_DOCKSPACE_LEFT: return m_dockspace_left;
            case E_DOCKSPACE_RIGHT: return m_dockspace_right;
            case E_DOCKSPACE_RIGHT_BOTTOM: return m_dockspace_right_bottom;
        }

        ASSERT(false);
        return 0xFFffFFff;
    }

  private:
    void draw_dockspace()
    {
        const auto* viewport = ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags host_window_flags = 0;
        host_window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize
                             | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
        host_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        host_window_flags |= ImGuiWindowFlags_NoBackground;

        char label[32];
        ImFormatString(label, IM_ARRAYSIZE(label), "DockSpaceViewport_%08X", viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin(label, NULL, host_window_flags);
        ImGui::PopStyleVar(3);

        ImGuiID dockspace_id = ImGui::GetID("DockSpace");

        static bool _once = false;
        if (!_once) {
            // TODO : apparently should do something like this so we don't keep overwriting the user layout
            //        but, i have no idea how we'd then find the left/right/bottom ids?
            // if (!ImGui::DockBuilderGetNode(dockspace_id)) {
            _once = true;

            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id);

            ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.2f, &m_dockspace_left, &m_dockspace_right);
            ImGui::DockBuilderSplitNode(m_dockspace_right, ImGuiDir_Up, 0.7f, &m_dockspace_right,
                                        &m_dockspace_right_bottom);

            ImGui::DockBuilderFinish(dockspace_id);
        }

        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
        ImGui::End();
    }

    void draw_main_menu()
    {
        // main menu bar
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Tools")) {
                if (ImGui::MenuItem("Hash Generator")) {
                    m_show_hash_generator = !m_show_hash_generator;
                }

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
    }

    void draw_hash_generator_widget()
    {
        static constexpr float kWidgetTableLabelWidth = 0.2f;

        // hash generator widget
        if (m_show_hash_generator) {
            // ImGui::SetNextWindowSize({480, 344}, ImGuiCond_Always);
            ImGui::SetNextWindowDockID(m_dockspace_right_bottom, ImGuiCond_Appearing);

            if (ImGui::Begin("Hash Generator", &m_show_hash_generator, ImGuiWindowFlags_NoResize)) {
                static ImGuiInputTextFlags flags =
                    (ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_ReadOnly);

                // name hash
                if (ImGui::CollapsingHeader("Name Hash", (ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf))) {
                    ImGui::PushStyleColor(ImGuiCol_Text, {0.53f, 0.53f, 0.53f, 1.0f});
                    ImGui::TextWrapped(
                        "32 bit hash generator using Jenkins lookup3. Used for general string and path hashing.");
                    ImGui::PopStyleColor();

                    if (ImGuiEx::BeginWidgetTableLayout(kWidgetTableLabelWidth)) {
                        static char buf[1024] = {0};
                        static u32  namehash  = 0;

                        // input
                        ImGuiEx::WidgetTableLayoutColumn("String", [&] {
                            if (ImGui::InputText("##text_to_hash_value", buf, lengthOf(buf))) {
                                namehash = (strlen(buf) > 0 ? ava::hashlittle(buf) : 0);
                            }
                        });

                        // result
                        ImGuiEx::WidgetTableLayoutColumn(
                            "Hash", [&] { ImGui::InputInt("##text_to_hash_result", (i32*)&namehash, 0, 0, flags); });
                    }

                    ImGuiEx::EndWidgetTableLayout();
                }

                // object id
                if (ImGui::CollapsingHeader("Object ID", (ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf))) {
                    ImGui::PushStyleColor(ImGuiCol_Text, {0.53f, 0.53f, 0.53f, 1.0f});
                    ImGui::TextWrapped("64 bit hash generator using MurMur3. Used for object and event id hashing.");
                    ImGui::PopStyleColor();

                    if (ImGuiEx::BeginWidgetTableLayout(kWidgetTableLabelWidth)) {
                        static char           buf[1024] = {0};
                        static ava::SObjectID object_id{0, 0xFFFF};
                        static uint64_t       namehash = object_id.to_uint64();
                        static bool           global   = false;

                        static auto generate_hash = [] {
                            object_id = ava::SObjectID{ava::HashString64(buf), (u16)(global ? 0x0000 : 0xFFFF)};
                            namehash  = object_id.to_uint64();
                        };

                        // type
                        ImGuiEx::WidgetTableLayoutColumn("Global", [&] {
                            if (ImGui::Checkbox("##object_is_global", &global)) {
                                generate_hash();
                            }
                        });

                        // input
                        ImGuiEx::WidgetTableLayoutColumn("String", [&] {
                            if (ImGui::InputText("##text_to_hash_value", buf, lengthOf(buf))) {
                                generate_hash();
                            }
                        });

                        // result
                        ImGuiEx::WidgetTableLayoutColumn("Hash", [&] {
                            ImGui::InputScalar("Result", ImGuiDataType_U64, (void*)&namehash, 0, 0, "%016llX", flags);
                        });
                    }

                    ImGuiEx::EndWidgetTableLayout();
                }
            }

            ImGui::End();
        }
    }

    game::IFormat* context_menu_get_format_handler_for_file(const std::string& path)
    {
        if (m_context_format_handler) return m_context_format_handler;
        m_context_format_handler = m_app.get_format_handler_for_file(path);
        return m_context_format_handler;
    }

  private:
    App&                          m_app;
    Renderer&                     m_renderer;
    std::vector<RenderCallback_t> m_render_callbacks;
    bool                          m_show_hash_generator    = false;
    game::IFormat*                m_context_format_handler = nullptr;
    ImGuiID                       m_dockspace_left         = -1;
    ImGuiID                       m_dockspace_right        = -1;
    ImGuiID                       m_dockspace_right_bottom = -1;
};

UI* UI::create(App& app, Renderer& renderer)
{
    return new UIImpl(app, renderer);
}

void UI::destroy(UI* instance)
{
    delete instance;
}
} // namespace jcmr