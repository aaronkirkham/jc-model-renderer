#include "pch.h"

#include "render_block_model.h"

#include "app/app.h"
#include "app/profile.h"

#include "game/game.h"
#include "game/games/justcause3/render_block.h"
#include "game/resource_manager.h"

#include "game/name_hash_lookup.h"

#include "render/imguiex.h"
#include "render/renderer.h"
#include "render/ui.h"

#include <AvaFormatLib/util/byte_vector_stream.h>
#include <imgui.h>

namespace jcmr::game::format
{
struct RenderBlockModelImpl final : RenderBlockModel {
    RenderBlockModelImpl(App& app)
        : m_app(app)
    {
        app.get_ui().on_render([this](RenderContext& context) {
            std::string _render_block_to_unload;
            for (auto& render_block : m_render_blocks) {
                bool open = true;
                ImGui::SetNextWindowSize({800, 600}, ImGuiCond_Appearing);
                if (ImGui::Begin(render_block.first.c_str(), &open,
                                 (ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoSavedSettings))) {
                    // menu bar
                    if (ImGui::BeginMenuBar()) {
                        if (ImGui::BeginMenu("File")) {
                            // save file
                            if (ImGui::Selectable(ICON_FA_SAVE " Save to...")) {
                                std::filesystem::path path;
                                if (os::get_open_folder(&path, os::FileDialogParams{})) {
                                    m_app.save_file(this, render_block.first, path);
                                }
                            }

                            // save to dropzone
                            if (ImGui::Selectable(ICON_FA_SAVE " Save to dropzone")) {
                                m_app.save_file(this, render_block.first, "dropzone");
                            }

                            // close
                            ImGui::Separator();
                            if (ImGui::Selectable(ICON_FA_WINDOW_CLOSE " Close")) {
                                open = false;
                            }

                            ImGui::EndMenu();
                        }

                        ImGui::EndMenuBar();
                    }

                    // draw render blocks
                    for (auto* block : render_block.second) {
                        ImGui::PushID(block);

                        if (ImGui::CollapsingHeader(block->get_type_name())) {
                            ImGui::Indent();

                            if (ImGuiEx::BeginWidgetTableLayout()) {
                                block->draw_ui();
                            }

                            ImGuiEx::EndWidgetTableLayout();
                            ImGui::Unindent();
                        }

                        ImGui::PopID();
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

        auto* resource_manager = m_app.get_game().get_resource_manager();

        ByteArray buffer;
        if (!resource_manager->read(filename, &buffer)) {
            LOG_ERROR("RenderBlockModel : failed to load file.");
            return false;
        }

        std::vector<ava::RenderBlockModel::RenderBlockData> blocks;
        AVA_FL_ENSURE(ava::RenderBlockModel::Parse(buffer, &blocks), false);

        // create render blocks
        {
            ProfileBlock _("RenderBlockModel : create render blocks");

            std::vector<game::IRenderBlock*> render_blocks;
            render_blocks.reserve(blocks.size());
            for (auto& block : blocks) {
                auto* render_block = (game::justcause3::RenderBlock*)m_app.get_game().create_render_block(block.first);
                if (render_block) {
                    render_block->set_resource_manager(resource_manager);
                    render_block->read(std::move(block.second));
                    render_blocks.emplace_back(std::move(render_block));
                } else {
                    LOG_WARNING("RenderBlockModel : failed to create render block with type {:x}", block.first);
                }
            }

            render_blocks.shrink_to_fit(); // because we might have failed to create some blocks.
            m_render_blocks.insert({filename, std::move(render_blocks)});
        }

        m_app.get_renderer().add_to_renderlist(m_render_blocks[filename]);
        return true;
    }

    void unload(const std::string& filename) override
    {
        auto iter = m_render_blocks.find(filename);
        if (iter == m_render_blocks.end()) return;

        auto& render_blocks = (*iter).second;
        m_app.get_renderer().remove_from_renderlist(render_blocks);

        for (auto* block : render_blocks) {
            delete block;
        }

        m_render_blocks.erase(iter);
    }

    bool save(const std::string& filename, ByteArray* out_buffer) override
    {
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
    }

    bool is_loaded(const std::string& filename) const override
    {
        return m_render_blocks.find(filename) != m_render_blocks.end();
    }

  private:
    App& m_app;

    std::unordered_map<std::string, std::vector<game::IRenderBlock*>> m_render_blocks;
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