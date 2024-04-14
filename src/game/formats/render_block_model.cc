#include "pch.h"

#include "render_block_model.h"

#include "app/app.h"
#include "app/profile.h"

#include "game/game.h"
#include "game/games/justcause3/render_block.h"
#include "game/resource_manager.h"

#include "game/name_hash_lookup.h"

#include "render/camera.h"
#include "render/imguiex.h"
#include "render/render_pipeline.h"
#include "render/renderer.h"
#include "render/ui.h"

#include <AvaFormatLib/util/byte_vector_stream.h>
#include <imgui.h>

namespace jcmr::game::format
{
template <typename T> inline T signum(T a)
{
    return a > 0 ? (T)1 : (a < 0 ? (T)-1 : 0);
}

struct RenderBlockRenderer {
    RenderBlockRenderer(App& app, const std::vector<IRenderBlock*>& render_blocks)
        : m_app(app)
        , m_render_blocks(std::move(render_blocks))
    {
        m_pipeline = RenderPipeline::create(app);

        m_pipeline->set_render_callback([&](const RenderContext& context) {
            for (auto* render_block : m_render_blocks) {
                if (render_block->setup(context)) {
                    render_block->draw(context);
                    // set_default_render_state();
                }
            }
        });
    }

    ~RenderBlockRenderer()
    {
        if (m_pipeline) {
            RenderPipeline::destroy(m_pipeline);
            m_pipeline = nullptr;
        }

        for (auto* block : m_render_blocks) {
            delete block;
        }
    }

    void resize(u32 width, u32 height)
    {
        if (!m_initialized) {
            m_pipeline->init(width, height);
            m_initialized = true;
            m_width       = width;
            m_height      = height;
        }

        if (m_width != width || m_height != height) {
            m_pipeline->resize(width, height);
            m_width  = width;
            m_height = height;
        }
    }

    void render_preview()
    {
        auto avail_size = ImGui::GetContentRegionAvail();
        resize((u32)avail_size.x, (u32)avail_size.y);

        // draw framebuffer
        auto* framebuffer = const_cast<ID3D11ShaderResourceView*>(m_pipeline->get_framebuffer(m_preview_buffer));
        ImGui::Image(framebuffer, avail_size);

        // viewport camera control
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            m_mouse_down     = true;
            m_mouse_down_pos = ImGui::GetIO().MousePos;
        } else if (m_mouse_down && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            m_mouse_down     = false;
            m_mouse_down_pos = {};
        } else if (m_mouse_down) {
            const auto&  pos = ImGui::GetIO().MousePos;
            const ImVec2 delta{(m_mouse_down_pos.x - pos.x), (m_mouse_down_pos.y - pos.y)};
            m_mouse_down_pos = pos;

            const float yaw   = signum(delta.x) * (powf(fabsf(delta.x / 220), 1.2f));
            const float pitch = signum(delta.y) * (powf(fabsf(delta.y / 220), 1.2f));

            auto& camera = m_pipeline->get_camera();
            camera.rotate(yaw, pitch);
        }
    }

    RenderPipeline*                   get_pipeline() const { return m_pipeline; }
    const std::vector<IRenderBlock*>& get_render_blocks() const { return m_render_blocks; }

  public:
    App&                       m_app;
    std::vector<IRenderBlock*> m_render_blocks;
    bool                       m_initialized = false;
    RenderPipeline*            m_pipeline;
    u32                        m_width          = 0;
    u32                        m_height         = 0;
    int32_t                    m_preview_buffer = 0;

  private:
    bool   m_mouse_down = false;
    ImVec2 m_mouse_down_pos{};
};

struct RenderBlockModelImpl final : RenderBlockModel {
    RenderBlockModelImpl(App& app)
        : m_app(app)
    {
        app.get_ui().on_render([this](RenderContext& context) {
            std::string _render_block_to_unload;
            for (auto& render_block : m_render_blocks) {
                auto dock_id = m_app.get_ui().get_dockspace_id(UI::E_DOCKSPACE_RIGHT);
                ImGui::SetNextWindowDockID(dock_id, ImGuiCond_Appearing); // TODO: ImGuiCond_FirstUseEver

                bool open = true;
                if (ImGui::Begin(render_block.first.c_str(), &open,
                                 (ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoSavedSettings))) {
                    // menu bar
                    if (!m_app.get_ui().draw_format_menu_bar(this, render_block.first)) {
                        open = false;
                    }

                    auto* render_block_renderer = render_block.second;

                    if (ImGuiEx::BeginWidgetTableLayout(0.5f)) {
                        // left
                        {
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn();

                            // G-Buffer texture preview
                            // TODO : Move this to the menu bar instead.
                            if (ImGuiEx::BeginWidgetTableLayout()) {
                                static const char* S_GBUFFER_NAMES[] = {"Diffuse", "Normal", "Properties",
                                                                        "PropertiesEx"};
                                IMGUIEX_TABLE_COL("G-Buffer",
                                                  ImGui::Combo("##preview_buffer",
                                                               &render_block_renderer->m_preview_buffer,
                                                               S_GBUFFER_NAMES, IM_ARRAYSIZE(S_GBUFFER_NAMES)));
                                ImGuiEx::EndWidgetTableLayout();
                            }

                            render_block_renderer->render_preview();
                        }

                        // right
                        {
                            ImGui::TableNextColumn();
                            ImGui::SetNextItemWidth(-1);
                            // ImGui::PushID(label);
                            for (auto* render_block : render_block.second->get_render_blocks()) {
                                ImGui::PushID(render_block);

                                if (ImGui::CollapsingHeader(render_block->get_type_name())) {
                                    ImGui::Indent();

                                    if (ImGuiEx::BeginWidgetTableLayout()) {
                                        render_block->draw_ui();
                                    }

                                    ImGuiEx::EndWidgetTableLayout();
                                    ImGui::Unindent();
                                }

                                ImGui::PopID();
                            }
                            // ImGui::PopID();
                        }

                        ImGuiEx::EndWidgetTableLayout();
                    }
                }

                if (!open) {
                    _render_block_to_unload = render_block.first;
                }

                ImGui::End();
            }

            if (!_render_block_to_unload.empty()) {
                unload(_render_block_to_unload);
                _render_block_to_unload.clear();
            }
        });
    }

    bool load(const std::string& filename) override
    {
        if (is_loaded(filename)) {
            return true;
        }

        LOG_INFO("RenderBlockModel : loading \"{}\"...", filename);

        // TODO : handle .LOD differently!
        if (filename.find(".lod") != std::string::npos) {
            LOG_ERROR("RenderBlockModel : can't parse .LOD yet..");
            return false;
        }

        auto* resource_manager = m_app.get_game()->get_resource_manager();

        ByteArray buffer;
        if (!resource_manager->read(filename, &buffer)) {
            LOG_ERROR("RenderBlockModel : failed to load file.");
            return false;
        }

        std::vector<ava::RenderBlockModel::RenderBlockData> blocks;
        AVA_FL_ENSURE(ava::RenderBlockModel::Parse(buffer, &blocks), false);

        // create render blocks
        std::vector<game::IRenderBlock*> render_blocks;
        render_blocks.reserve(blocks.size());
        {
            ProfileBlock _("RenderBlockModel : create render blocks");

            for (auto& block : blocks) {
                auto* render_block = (game::justcause3::RenderBlock*)m_app.get_game()->create_render_block(block.first);
                if (render_block) {
                    render_block->set_resource_manager(resource_manager);
                    render_block->read(std::move(block.second));
                    render_blocks.emplace_back(std::move(render_block));
                } else {
                    LOG_WARNING("RenderBlockModel : failed to create render block with type {:x}", block.first);
                }
            }

            render_blocks.shrink_to_fit(); // because we might have failed to create some blocks with unknown types.
        }

        auto* renderer = new RenderBlockRenderer(m_app, render_blocks);
        m_render_blocks.insert({filename, renderer});
        return true;
    }

    void unload(const std::string& filename) override
    {
        auto iter = m_render_blocks.find(filename);
        if (iter == m_render_blocks.end()) return;

        auto* render_block_renderer = (*iter).second;
        delete render_block_renderer;

        m_render_blocks.erase(iter);
    }

    bool save(const std::string& filename, ByteArray* out_buffer) override
    {
        return false;

#if 0
        auto iter = m_render_blocks.find(filename);
        if (iter == m_render_blocks.end()) return false;

        auto& render_block = (*iter);

        ava::utils::ByteVectorStream stream(out_buffer);

        // write header
        ava::RenderBlockModel::RbmHeader header{};
        // header.m_BoundingBoxMin;
        // header.m_BoundingBoxMax;
        header.m_NumberOfBlocks = render_block.second.size();
        stream.write(header);

        // write blocks
        for (auto* block : render_block.second) {
            // write the type hash
            u32 type_hash = block->get_type_hash();
            stream.write(type_hash);

            // write the block data
            reinterpret_cast<game::justcause3::RenderBlock*>(block)->write(out_buffer);

            // write the end of block checksum
            stream.write(ava::RenderBlockModel::RBM_BLOCK_CHECKSUM);
        }

        return true;
#endif
    }

    bool is_loaded(const std::string& filename) const override
    {
        return m_render_blocks.find(filename) != m_render_blocks.end();
    }

  private:
    App&                                                  m_app;
    std::unordered_map<std::string, RenderBlockRenderer*> m_render_blocks;
};

RenderBlockModel* RenderBlockModel::create(App& app)
{
    return new RenderBlockModelImpl(app);
}

void RenderBlockModel::destroy(RenderBlockModel* instance)
{
    delete instance;
}
} // namespace jcmr::game::format