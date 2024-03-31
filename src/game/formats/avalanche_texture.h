#ifndef JCMR_FORMATS_AVALANCHE_TEXTURE_H_HEADER_GUARD
#define JCMR_FORMATS_AVALANCHE_TEXTURE_H_HEADER_GUARD

#include "../format.h"
#include "render/fonts/icons.h"

namespace jcmr
{
struct App;

namespace game::format
{
    struct AvalancheTexture : IFormat {
        static AvalancheTexture* create(App& app);
        static void              destroy(AvalancheTexture* instance);

        const char* get_name() const override { return "Avalanche Texture"; }
        const char* get_filetype_icon() const override { return ICON_FA_FILE_IMAGE; }

        bool                     can_be_created() const override { return true; }
        std::vector<const char*> create_extensions() const override { return {"ddsc"}; }

        std::vector<const char*> get_extensions() override { return {"ddsc", "dds"}; }
        u32                      get_header_magic() const override { return ava::AvalancheTexture::AVTX_MAGIC; }
    };

} // namespace game::format
} // namespace jcmr

#endif // JCMR_FORMATS_AVALANCHE_TEXTURE_H_HEADER_GUARD
