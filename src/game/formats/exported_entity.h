#ifndef JCMR_FORMATS_EXPORTED_ENTITY_H_HEADER_GUARD
#define JCMR_FORMATS_EXPORTED_ENTITY_H_HEADER_GUARD

#include "../format.h"
#include "render/fonts/icons.h"

namespace jcmr
{
struct App;

namespace game::format
{
    struct ExportedEntity : IFormat {
        static ExportedEntity* create(App& app);
        static void            destroy(ExportedEntity* instance);

        bool        wants_to_override_directory_tree() const override { return true; }
        const char* get_filetype_icon() const override { return ICON_FA_FILE_ARCHIVE; }

        bool can_be_exported() const override { return true; }

        std::vector<const char*> get_extensions() override { return {"ee", "bl", "nl", "fl"}; }
        u32                      get_header_magic() const override { return ava::StreamArchive::SARC_MAGIC; }
    };

} // namespace game::format
} // namespace jcmr

#endif // JCMR_FORMATS_EXPORTED_ENTITY_H_HEADER_GUARD
