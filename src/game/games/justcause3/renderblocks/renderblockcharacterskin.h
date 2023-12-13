#ifndef JCMR_RENDERBLOCKS_RENDERBLOCKCHARACTERSKIN_H_HEADER_GUARD
#define JCMR_RENDERBLOCKS_RENDERBLOCKCHARACTERSKIN_H_HEADER_GUARD

#include "game/games/justcause3/render_block.h"

namespace jcmr
{
struct App;

namespace game::justcause3
{
    struct RenderBlockCharacterSkin : RenderBlock {
        static RenderBlockCharacterSkin* create(App& app);
        static void                      destroy(RenderBlockCharacterSkin* instance);

        RenderBlockCharacterSkin(App& app)
            : RenderBlock(app)
        {
        }

        const char* get_type_name() const override { return "RenderBlockCharacterSkin"; }
        u32         get_type_hash() const override { return ava::RenderBlockModel::RB_CHARACTERSKIN; }
    };
} // namespace game::justcause3
} // namespace jcmr

#endif // JCMR_RENDERBLOCKS_RENDERBLOCKCHARACTERSKIN_H_HEADER_GUARD
