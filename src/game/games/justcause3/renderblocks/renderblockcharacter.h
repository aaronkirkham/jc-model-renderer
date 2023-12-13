#ifndef JCMR_RENDERBLOCKS_RENDERBLOCKCHARACTER_H_HEADER_GUARD
#define JCMR_RENDERBLOCKS_RENDERBLOCKCHARACTER_H_HEADER_GUARD

#include "game/games/justcause3/render_block.h"

namespace jcmr
{
struct App;

namespace game::justcause3
{
    struct RenderBlockCharacter : RenderBlock {
        static RenderBlockCharacter* create(App& app);
        static void                  destroy(RenderBlockCharacter* instance);

        RenderBlockCharacter(App& app)
            : RenderBlock(app)
        {
        }

        const char* get_type_name() const override { return "RenderBlockCharacter"; }
        u32         get_type_hash() const override { return ava::RenderBlockModel::RB_CHARACTER; }
    };
} // namespace game::justcause3
} // namespace jcmr

#endif // JCMR_RENDERBLOCKS_RENDERBLOCKCHARACTER_H_HEADER_GUARD
