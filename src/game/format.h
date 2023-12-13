#ifndef JCMR_GAME_FORMAT_H_HEADER_GUARD
#define JCMR_GAME_FORMAT_H_HEADER_GUARD

#include "platform.h"
#include "render/fonts/icons.h"

namespace jcmr::game
{
struct IFormat {
    virtual ~IFormat() = default;

    virtual void update() {}

    virtual bool load(const std::string& filename)                        = 0;
    virtual void unload(const std::string& filename)                      = 0;
    virtual bool save(const std::string& filename, ByteArray* out_buffer) = 0;
    virtual bool is_loaded(const std::string& filename) const             = 0;

    // directory_list specifics
    virtual bool        wants_to_override_directory_tree() const { return false; }
    virtual const char* get_filetype_icon() const { return ICON_FA_FILE; }
    virtual void        directory_tree_handler(const std::string& filename, const std::string& path) {}
    virtual void        context_menu_handler(const std::string& filename) {}

    // import/export specifics
    virtual bool can_be_imported() const { return false; }
    virtual bool can_be_exported() const { return false; }
    virtual bool has_import_settings() const { return false; }
    virtual bool has_export_settings() const { return false; }
    virtual bool import_from() { return false; }
    virtual bool export_to(const std::string& filename, const std::filesystem::path& path) { return false; }

    virtual std::vector<const char*> get_extensions() = 0;
    virtual u32                      get_header_magic() const { return 0xFFffFFff; }
};
} // namespace jcmr::game

#endif // JCMR_GAME_FORMAT_H_HEADER_GUARD
