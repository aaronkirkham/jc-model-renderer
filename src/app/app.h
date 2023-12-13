#ifndef JCMR_APP_APP_H_HEADER_GUARD
#define JCMR_APP_APP_H_HEADER_GUARD

#include "platform.h"

namespace jcmr
{
namespace os
{
    struct Event;
}

namespace game
{
    struct IFormat;
}

struct IGame;
struct Renderer;
struct UI;

struct App {
    using FileHandler_t          = std::function<bool(const std::string& filename, ByteArray* out_buffer)>;
    using DirectoryTreeHandler_t = std::function<bool(const std::string& filename, const std::string& path)>;

    static App* create();
    static void destroy(App* app);

    App()          = default;
    virtual ~App() = default;

    virtual void on_init()                        = 0;
    virtual void on_shutdown()                    = 0;
    virtual void on_tick()                        = 0;
    virtual void on_event(const os::Event& event) = 0;

    virtual bool save_file(game::IFormat* format, const std::string& filename, const std::filesystem::path& path) = 0;
    virtual bool write_binary_file(const std::filesystem::path& path, ByteArray& buffer)                          = 0;

    virtual void           register_format_handler(game::IFormat* format)                       = 0;
    virtual game::IFormat* get_format_handler(u32 extension_hash) const                         = 0;
    virtual game::IFormat* get_format_handler_for_file(const std::filesystem::path& path) const = 0;

    virtual void                              register_file_read_handler(FileHandler_t callback) = 0;
    virtual const std::vector<FileHandler_t>& get_file_read_handlers() const                     = 0;

    virtual Renderer& get_renderer() = 0;
    virtual UI&       get_ui()       = 0;
    virtual IGame&    get_game()     = 0;

    static ByteArray load_internal_resource(i32 resource_id);
};
} // namespace jcmr

#endif // JCMR_APP_APP_H_HEADER_GUARD
