#ifndef JCMR_JUSTCAUSE3_RENDER_BLOCK_H_HEADER_GUARD
#define JCMR_JUSTCAUSE3_RENDER_BLOCK_H_HEADER_GUARD

#include "game/render_block.h"

namespace jcmr::game::justcause3
{
struct RenderBlock : IRenderBlock {
    RenderBlock(App& app)
        : IRenderBlock(app)
    {
    }

    virtual void read(const ByteArray& buffer) = 0;
    virtual void write(ByteArray* buffer)      = 0;

    virtual void    read_textures(std::istream& stream);
    virtual void    write_textures(ByteArray* buffer);
    virtual Buffer* read_buffer(std::istream& stream, u32 type, u32 stride);
    virtual void    write_buffer(Buffer* buf, ByteArray* buffer);

  protected:
};
} // namespace jcmr::game::justcause3

#endif // JCMR_JUSTCAUSE3_RENDER_BLOCK_H_HEADER_GUARD
