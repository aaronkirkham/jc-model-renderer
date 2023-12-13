#ifndef JCMR_RENDERBLOCKS_RENDERBLOCKGENERALMKIII_H_HEADER_GUARD
#define JCMR_RENDERBLOCKS_RENDERBLOCKGENERALMKIII_H_HEADER_GUARD

#include "game/games/justcause3/render_block.h"

namespace jcmr
{
struct App;

namespace game::justcause3
{
    struct RenderBlockGeneralMkIII : RenderBlock {
        static RenderBlockGeneralMkIII* create(App& app);
        static void                     destroy(RenderBlockGeneralMkIII* instance);

        RenderBlockGeneralMkIII(App& app)
            : RenderBlock(app)
        {
        }

        const char* get_type_name() const override { return "RenderBlockGeneralMkIII"; }
        u32         get_type_hash() const override { return ava::RenderBlockModel::RB_GENERALMKIII; }
    };
} // namespace game::justcause3
} // namespace jcmr

#endif // JCMR_RENDERBLOCKS_RENDERBLOCKGENERALMKIII_H_HEADER_GUARD
