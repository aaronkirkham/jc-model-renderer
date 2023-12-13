#include "pch.h"

#include "avalanche_texture.h"

#include "app/app.h"
#include "app/profile.h"

#include "game/game.h"
#include "game/render_block.h"
#include "game/resource_manager.h"

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

                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});

                auto title = fmt::format("{} {}##texture_view", get_filetype_icon(), texture.first);

                bool open = true;
                if (ImGui::Begin(title.c_str(), &open, ImGuiWindowFlags_NoSavedSettings)) {
                    auto min = ImGui::GetWindowContentRegionMin();
                    auto max = ImGui::GetWindowContentRegionMax();

                    auto width  = static_cast<i32>(max.x - min.x);
                    auto height = static_cast<i32>(max.y - min.y);

                    width -= (width % 2 != 0) ? 1 : 0;
                    height -= (height % 2 != 0) ? 1 : 0;

                    if (texture.second) {
                        ImGui::Image(texture.second->get_srv(), {(float)width, (float)height});
                    }
                }

                if (!open) {
                    _texture_to_unload = texture.first;
                }

                ImGui::End();
                ImGui::PopStyleVar();
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

        auto texture = m_app.get_game().create_texture(filename);
        m_textures.insert({filename, std::move(texture)});
        return true;
    }

    void unload(const std::string& filename) override
    {
        auto iter = m_textures.find(filename);
        if (iter == m_textures.end()) return;
        m_textures.erase(iter);
    }

    bool save(const std::string& filename, ByteArray* out_buffer) override
    {
        //
        return false;
    }

    bool is_loaded(const std::string& filename) const override { return m_textures.find(filename) != m_textures.end(); }

  private:
    App& m_app;

    std::unordered_map<std::string, std::shared_ptr<Texture>> m_textures;
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