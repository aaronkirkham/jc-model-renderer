#ifndef JCMR_FORMATS_RESOURCE_BUNDLE_H_HEADER_GUARD
#define JCMR_FORMATS_RESOURCE_BUNDLE_H_HEADER_GUARD

#include "../format.h"

namespace jcmr
{
struct App;

namespace game::format
{
    struct ResourceBundle : IFormat {
        static ResourceBundle* create(App& app);
        static void            destroy(ResourceBundle* instance);

        bool                wants_to_override_directory_tree() const override { return true; }
        virtual const char* get_name() const { return "Resource Bundle"; }
        const char*         get_filetype_icon() const override { return ICON_FA_FILE_ARCHIVE; }

        bool can_be_exported() const override { return true; }

        std::vector<const char*> get_extensions() override { return {"resourcebundle"}; }
    };
} // namespace game::format
} // namespace jcmr

#endif // JCMR_FORMATS_RESOURCE_BUNDLE_H_HEADER_GUARD
