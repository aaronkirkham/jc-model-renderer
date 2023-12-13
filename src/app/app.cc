#include "pch.h"

#include "app.h"

#include "app/allocator.h"
#include "app/directory_list.h"
#include "app/os.h"

#include "game/fallback_format_handler.h"
#include "game/format.h"
#include "game/game.h"
#include "game/name_hash_lookup.h"
#include "game/resource_manager.h"

#include "game/formats/avalanche_data_format.h"
#include "game/formats/avalanche_model_format.h"
#include "game/formats/avalanche_texture.h"
#include "game/formats/exported_entity.h"
#include "game/formats/render_block_model.h"
#include "game/formats/runtime_container.h"
#include "game/formats/xvmc_script.h"

#include "render/renderer.h"
#include "render/ui.h"

#include <Windows.h>
#include <fstream>
#include <imgui.h>

namespace jcmr
{
struct AppImpl final : App {
  public:
    AppImpl(const AppImpl&)        = delete;
    void operator=(const AppImpl&) = delete;

    AppImpl()
        : m_allocator(m_main_allocator)
    {
        m_renderer = Renderer::create(*this);

        m_fallback_format_handler = game::format::FallbackFormatHandler::create(*this);
        register_format_handler(m_fallback_format_handler);

        register_format_handler(game::format::ExportedEntity::create(*this));
        register_format_handler(game::format::RuntimeContainer::create(*this));
        register_format_handler(game::format::RenderBlockModel::create(*this));
        register_format_handler(game::format::AvalancheTexture::create(*this));
        register_format_handler(game::format::AvalancheModelFormat::create(*this));
        register_format_handler(game::format::AvalancheDataFormat::create(*this));
        register_format_handler(game::format::XVMCScript::create(*this));

        // render dictionary
        m_renderer->get_ui().on_render([this](RenderContext&) {
            auto dock_id = m_renderer->get_ui().get_dockspace_id(UI::E_DOCKSPACE_LEFT);
            ImGui::SetNextWindowDockID(dock_id, ImGuiCond_Appearing);

            if (ImGui::Begin("Files")) {
                if (!m_current_game) return;

                auto& tree = m_current_game->get_resource_manager()->get_dictionary_tree();
                tree.render_search_input();

                if (ImGui::BeginChild("##scroll", ImVec2(0, 0), 0, ImGuiWindowFlags_HorizontalScrollbar)) {
                    tree.render(*this);
                }

                ImGui::EndChild();
            }

            ImGui::End();
        });
    }

    ~AppImpl()
    {
        // game::format::FallbackFormatHandler::destroy(m_fallback_format_handler);
        Renderer::destroy(m_renderer);
    }

    void on_init() override
    {
        os::InitWindowArgs args{};
        args.name  = "Avalanche Game Tools";
        args.size  = {1920, 1080};
        args.flags = os::InitWindowArgs::CENTERED;

        m_window = os::create_window(args);
        ASSERT(m_window);

        auto rect = os::get_window_rect(m_window);

        Renderer::InitRendererArgs renderer_args{};
        renderer_args.window_handle = m_window->handle;
        renderer_args.width         = rect.width;
        renderer_args.height        = rect.height;

        // init renderer
        m_renderer->init(renderer_args);

        // game related
        load_namehash_lookup_table();
        m_current_game = IGame::create(EGame::EGAME_JUSTCAUSE3, *this);
        // m_current_game = IGame::create(EGame::EGAME_JUSTCAUSE4, *this);

        // DEBUGGING, REMOVE ME LATER
        // auto handler = get_format_handler_for_file(
        //     "editor/entities/jc_weapons/01_one_handed/w011_pistol_u_pozhar_98/w011_pistol_u_pozhar_98.wtunec");
        // ASSERT(handler != nullptr);
        // handler->load(
        //     "editor/entities/jc_weapons/01_one_handed/w011_pistol_u_pozhar_98/w011_pistol_u_pozhar_98.wtunec");
        // LOG_INFO("app::on_init - exiting..");
        // exit(1);
    }

    void on_shutdown() override
    {
        m_renderer->shutdown();

        os::destroy_window(m_window);
        m_window = nullptr;
    }

    void on_tick() override
    {
        if (m_renderer) {
            m_renderer->frame();
        }

        // update format handlers
        for (auto& handler : m_format_handlers) {
            handler.second->update();
        }
    }

    void on_event(const os::Event& event) override
    {
        switch (event.type) {
            case os::Event::Type::WINDOW_SIZE: {
                m_renderer->resize(event.window_size.width, event.window_size.height);
                break;
            }

            case os::Event::Type::QUIT:
            case os::Event::Type::WINDOW_CLOSE: {
                os::quit();
                break;
            }
        }
    }

    bool save_file(game::IFormat* format, const std::string& filename, const std::filesystem::path& path) override
    {
        LOG_INFO("App : save file \"{}\" to \"{}\"...", filename, path.generic_string());

        // TODO : because we support saving folders (with contents), this won't work for folders inside ExportedEntity
        //        archives, we need a nice way of dealing with that. Maybe that's a job for format->save() above?

        std::filesystem::path filepath;

        // are we saving to dropzone?
        if (path == "dropzone") {
            filepath = m_current_game->get_directory() / path / filename;
            std::filesystem::create_directories(filepath.parent_path());
        } else {
            std::filesystem::path _name(filename);
            filepath = path / _name.filename();
        }

        // format handler couldn't save the current file, try read from the resource manager
        ByteArray buffer;
        if (!format || !format->is_loaded(filename) || !format->save(filename, &buffer)) {
            auto* resource_manager = m_current_game->get_resource_manager();
            if (!resource_manager->read(filename, &buffer)) {
                return false;
            }
        }

        // write the file
        return write_binary_file(filepath, buffer);
    }

    bool write_binary_file(const std::filesystem::path& path, ByteArray& buffer) override
    {
        std::ofstream stream(path, std::ios::binary);
        if (stream.fail()) return false;
        stream.write((char*)buffer.data(), buffer.size());
        stream.close();
        return true;
    }

    void register_format_handler(game::IFormat* format) override
    {
        // special case for fallback handler
        const auto header_magic = format->get_header_magic();
        if (header_magic == FALLBACK_FORMAT_MAGIC) {
            m_format_handlers.insert({FALLBACK_FORMAT_MAGIC, format});
            return;
        }

        auto extensions = format->get_extensions();

        if (extensions.empty()) {
            m_format_handlers.insert({header_magic, format});
            return;
        }

        for (auto& extension : extensions) {
            auto extension_hash = ava::hashlittle(extension);
            ASSERT(m_format_handlers.find(extension_hash) == m_format_handlers.end());
            m_format_handlers.insert({extension_hash, format});
        }
    }

    game::IFormat* get_format_handler(u32 extension_hash) const override
    {
        auto iter = m_format_handlers.find(extension_hash);
        if (iter == m_format_handlers.end()) return nullptr;
        return (*iter).second;
    }

    game::IFormat* get_format_handler_for_file(const std::filesystem::path& path) const override
    {
        auto* resource_manager = m_current_game->get_resource_manager();

        // TODO : maybe look into some optimizations to only read the first 4-bytes from the file instead of it all
        // TODO : could probably pass the buffer into the format handler too somehow - so we don't have to load twice.
        ByteArray buffer;
        if (!resource_manager->read(path.generic_string(), &buffer)) {
            return nullptr;
        }

        auto header_magic = *(u32*)buffer.data();
        auto iter =
            std::find_if(m_format_handlers.begin(), m_format_handlers.end(), [header_magic](const auto& handler) {
                return handler.second->get_header_magic() == header_magic;
            });

        // if we don't have a handler for this format, return the fallback
        if (iter == m_format_handlers.end()) {
            return m_fallback_format_handler;
        }

        return (*iter).second;
    }

    void register_file_read_handler(FileHandler_t callback) override { m_file_read_handlers.emplace_back(callback); }
    const std::vector<FileHandler_t>& get_file_read_handlers() const override { return m_file_read_handlers; }

    Renderer& get_renderer() override { return *m_renderer; }
    UI&       get_ui() override { return m_renderer->get_ui(); }
    IGame&    get_game() override { return *m_current_game; }

  private:
    DefaultAllocator m_main_allocator;
    IAllocator&      m_allocator;

    os::Window*    m_window;
    Renderer*      m_renderer                = nullptr;
    IGame*         m_current_game            = nullptr;
    game::IFormat* m_fallback_format_handler = nullptr;

    std::unordered_map<u32, game::IFormat*> m_format_handlers;
    std::vector<FileHandler_t>              m_file_read_handlers;
};

App* App::create()
{
    return new AppImpl;
}

void App::destroy(App* app)
{
    delete app;
}

ByteArray App::load_internal_resource(i32 resource_id)
{
    const auto handle = GetModuleHandle(nullptr);

    const auto resource_info = FindResource(handle, MAKEINTRESOURCE(resource_id), RT_RCDATA);
    if (!resource_info) {
        DEBUG_BREAK();
        return {};
    }

    const auto resource_data = LoadResource(handle, resource_info);
    if (!resource_data) {
        DEBUG_BREAK();
        return {};
    }

    const void* ptr  = LockResource(resource_data);
    auto        size = SizeofResource(handle, resource_info);

    ByteArray result(size);
    std::memcpy(result.data(), ptr, size);
    return result;
}
} // namespace jcmr