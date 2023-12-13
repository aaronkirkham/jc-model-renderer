#ifndef JCMR_JUSTCAUSE4_RENDER_BLOCK_H_HEADER_GUARD
#define JCMR_JUSTCAUSE4_RENDER_BLOCK_H_HEADER_GUARD

#include "game/render_block.h"

namespace jcmr::game::justcause4
{
struct RenderBlock : IRenderBlock {
    RenderBlock(App& app)
        : IRenderBlock(app)
    {
    }

    virtual void read(const ava::SAdfDeferredPtr& attributes) = 0;

  protected:
    //
};
} // namespace jcmr::game::justcause4

#endif // JCMR_JUSTCAUSE4_RENDER_BLOCK_H_HEADER_GUARD
