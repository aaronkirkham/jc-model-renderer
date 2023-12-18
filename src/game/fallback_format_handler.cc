#include "pch.h"

#include "fallback_format_handler.h"

#include "app/app.h"

#include "game/game.h"
#include "game/resource_manager.h"

#include "render/imguiex.h"
#include "render/renderer.h"
#include "render/ui.h"

namespace jcmr::game::format
{
struct FallbackFormatHandlerImpl final : FallbackFormatHandler {
    FallbackFormatHandlerImpl(App& app)
        : m_app(app)
    {
        app.get_ui().on_render([this](RenderContext& context) {
            std::string _file_to_unload;
            for (auto& file : m_loaded_files) {
                bool open = true;
                ImGui::SetNextWindowSize({850, 500}, ImGuiCond_Appearing);

                auto title = fmt::format("{} {}", ICON_FA_FILE, file.first.c_str());

                if (ImGui::Begin(title.c_str(), &open, ImGuiWindowFlags_NoSavedSettings)) {
                    auto size = ImGui::GetContentRegionAvail();

                    // ImGui::PushItemWidth(-1);
                    // TODO : hex editor view
                    // https://github.com/ocornut/imgui_club/blob/main/imgui_memory_editor/imgui_memory_editor.h
                    ImGui::InputTextMultiline("##text_editor", (char*)file.second.data(), file.second.size(), size,
                                              ImGuiInputTextFlags_ReadOnly);
                }

                if (!open) {
                    _file_to_unload = file.first;
                }

                ImGui::End();
            }

            if (!_file_to_unload.empty()) {
                unload(_file_to_unload);
                _file_to_unload.clear();
            }
        });
    }

    bool load(const std::string& filename) override
    {
        if (is_loaded(filename)) {
            return true;
        }

        auto* resource_manager = m_app.get_game()->get_resource_manager();

        ByteArray buffer;
        if (!resource_manager->read(filename, &buffer)) {
            return false;
        }

        m_loaded_files.insert({filename, std::move(buffer)});
        return true;
    }

    void unload(const std::string& filename) override
    {
        auto iter = m_loaded_files.find(filename);
        if (iter == m_loaded_files.end()) return;
        m_loaded_files.erase(iter);
    }

    bool save(const std::string& filename, ByteArray* out_buffer) override
    {
        //
        return false;
    }

    bool is_loaded(const std::string& filename) const override
    {
        return m_loaded_files.find(filename) != m_loaded_files.end();
    }

  private:
    App&                                       m_app;
    std::unordered_map<std::string, ByteArray> m_loaded_files;
};

FallbackFormatHandler* FallbackFormatHandler::create(App& app)
{
    return new FallbackFormatHandlerImpl(app);
}

void FallbackFormatHandler::destroy(FallbackFormatHandler* instance)
{
    delete instance;
}
} // namespace jcmr::game::format