#include "pch.h"

#include "avalanche_texture.h"

#include "app/app.h"
#include "app/profile.h"
#include "app/utils.h"

#include "game/game.h"
#include "game/render_block.h"
#include "game/resource_manager.h"

#include "render/imguiex.h"
#include "render/renderer.h"
#include "render/texture.h"
#include "render/ui.h"

#include <imgui.h>

namespace jcmr::game::format
{
struct AvalancheTextureImpl final : AvalancheTexture {
    AvalancheTextureImpl(App& app)
        : m_app(app)
    {
        app.get_ui().on_render([this](RenderContext& context) {
            std::string _texture_to_unload;
            for (auto& texture : m_textures) {
                // ImGui::SetNextWindowSizeConstraints({128, 128}, {FLT_MAX, FLT_MAX}, [](ImGuiSizeCallbackData* data) {
                //     data->DesiredSize = {std::max(data->DesiredSize.x, data->DesiredSize.y),
                //                          std::max(data->DesiredSize.x, data->DesiredSize.y)};
                // });

                auto dock_id = m_app.get_ui().get_dockspace_id(UI::E_DOCKSPACE_RIGHT);
                ImGui::SetNextWindowDockID(dock_id, ImGuiCond_Appearing); // TODO: ImGuiCond_FirstUseEver

                auto title = fmt::format("{} {}##texture_view", get_filetype_icon(), texture.first);

                bool open = true;
                if (ImGui::Begin(title.c_str(), &open, (ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoSavedSettings))) {
                    // menu bar
                    if (!m_app.get_ui().draw_format_menu_bar(this, texture.first)) {
                        open = false;
                    }

                    // draw all texture streams
                    if (ImGui::BeginTable("##texture_stream_grid", 4,
                                          ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_HighlightHoveredColumn)) {
                        for (size_t i = 0; i < texture.second.size(); ++i) {
                            const auto& stream = texture.second[i];

                            ImGui::TableNextColumn();

                            ImGui::Text("Stream %d", i);
                            ImGui::Text("%d x %d", stream.entry.first.m_Width, stream.entry.first.m_Height);

                            if (stream.texture) {
                                ImGui::Image(stream.texture->get_srv(), {256, 256});
                            } else {
                                ImGui::Dummy({256, 256});
                            }
                        }

                        ImGui::EndTable();
                    }
                }

                if (!open) {
                    _texture_to_unload = texture.first;
                }

                ImGui::End();
            }

            // raw textures
            for (auto& texture : m_raw_textures) {
                auto dock_id = m_app.get_ui().get_dockspace_id(UI::E_DOCKSPACE_RIGHT);
                ImGui::SetNextWindowDockID(dock_id, ImGuiCond_Appearing); // TODO: ImGuiCond_FirstUseEver

                auto title = fmt::format("{} {}##texture_view", get_filetype_icon(), texture.first);

                bool open = true;
                if (ImGui::Begin(title.c_str(), &open, (ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoSavedSettings))) {
                    // menu bar
                    if (!m_app.get_ui().draw_format_menu_bar(this, texture.first)) {
                        open = false;
                    }

                    auto min = ImGui::GetWindowContentRegionMin();
                    auto max = ImGui::GetWindowContentRegionMax();

                    auto width  = static_cast<i32>(max.x - min.x);
                    auto height = static_cast<i32>(max.y - min.y);

                    width -= (width % 2 != 0) ? 1 : 0;
                    height -= (height % 2 != 0) ? 1 : 0;

                    if (texture.second) {
                        ImGui::Image(texture.second->get_srv(), {(float)width, (float)height});
                    } else {
                        ImGui::Dummy({(float)width, (float)height});
                    }
                }

                if (!open) {
                    _texture_to_unload = texture.first;
                }

                ImGui::End();
            }

            if (!_texture_to_unload.empty()) {
                unload(_texture_to_unload);
                _texture_to_unload.clear();
            }
        });
    }

    bool load(const std::string& filename) override
    {
        if (is_loaded(filename)) {
            return true;
        }

        LOG_INFO("AvalancheTexture : loading \"{}\"...", filename);

        // raw texture
        auto extension = utils::get_extension(filename);
        if (extension == "dds") {
            auto texture = m_app.get_game()->create_raw_texture(filename);
            m_raw_textures.insert({filename, std::move(texture)});
            return true;
        }

        ava::AvalancheTexture::AvtxHeader header{};
        const auto                        textures = m_app.get_game()->load_textures(filename, &header);
        // textures[0].secon

        std::vector<TextureStream> streams;
        streams.reserve(textures.size());

        for (const auto& texture : textures) {
            TextureStream stream{};
            stream.entry   = std::move(texture);
            stream.texture = m_app.get_game()->create_texture(stream.entry.first, stream.entry.second);

            streams.push_back(std::move(stream));
        }

        m_textures.insert({filename, std::move(streams)});

        return true;
    }

    void unload(const std::string& filename) override
    {
        auto iter = m_textures.find(filename);
        if (iter != m_textures.end()) m_textures.erase(iter);

        auto iter2 = m_raw_textures.find(filename);
        if (iter2 != m_raw_textures.end()) m_raw_textures.erase(iter2);
    }

    bool save(const std::string& filename, ByteArray* out_buffer) override
    {
        //
        return false;
    }

    bool is_loaded(const std::string& filename) const override
    {
        return m_textures.find(filename) != m_textures.end() || m_raw_textures.find(filename) != m_raw_textures.end();
    }

  private:
    App& m_app;

    struct TextureStream {
        IGame::TextureEntryBufferPair entry;
        std::shared_ptr<Texture>      texture;
    };

    std::unordered_map<std::string, std::vector<TextureStream>> m_textures;

    std::unordered_map<std::string, std::shared_ptr<Texture>> m_raw_textures;
};

AvalancheTexture* AvalancheTexture::create(App& app)
{
    return new AvalancheTextureImpl(app);
}

void AvalancheTexture::destroy(AvalancheTexture* instance)
{
    delete instance;
}
} // namespace jcmr::game::format