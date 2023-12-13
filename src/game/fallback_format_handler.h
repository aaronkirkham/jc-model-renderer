#ifndef JCMR_GAME_FALLBACK_FORMAT_HANDLER_H_HEADER_GUARD
#define JCMR_GAME_FALLBACK_FORMAT_HANDLER_H_HEADER_GUARD

#include "format.h"

namespace jcmr
{
struct App;

static constexpr u32 FALLBACK_FORMAT_MAGIC = 0x11223344;

namespace game::format
{
    struct FallbackFormatHandler : IFormat {
        static FallbackFormatHandler* create(App& app);
        static void                   destroy(FallbackFormatHandler* instance);

        virtual const char* get_filetype_icon() const { return ICON_FA_FILE; }

        bool can_be_exported() const override { return true; }

        std::vector<const char*> get_extensions() override { return {}; }
        u32                      get_header_magic() const override { return FALLBACK_FORMAT_MAGIC; }
    };

} // namespace game::format
} // namespace jcmr

#endif // JCMR_GAME_FALLBACK_FORMAT_HANDLER_H_HEADER_GUARD
