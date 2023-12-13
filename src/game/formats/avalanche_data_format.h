#ifndef JCMR_FORMATS_AVALANCHE_DATA_FORMAT_H_HEADER_GUARD
#define JCMR_FORMATS_AVALANCHE_DATA_FORMAT_H_HEADER_GUARD

#include "../format.h"

namespace jcmr
{
struct App;

namespace game::format
{
    struct AvalancheDataFormat : IFormat {
        static AvalancheDataFormat* create(App& app);
        static void                 destroy(AvalancheDataFormat* instance);

        virtual const char* get_filetype_icon() const { return ICON_FA_CODE; }

        bool can_be_exported() const override { return true; }

        std::vector<const char*> get_extensions() override { return {}; }
        u32                      get_header_magic() const override { return ava::ADF_MAGIC; }
    };

} // namespace game::format
} // namespace jcmr

#endif // JCMR_FORMATS_AVALANCHE_DATA_FORMAT_H_HEADER_GUARD
