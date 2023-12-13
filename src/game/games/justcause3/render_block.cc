#include "pch.h"

#include "render_block.h"

#include "app/app.h"

#include "game/game.h"
#include "game/resource_manager.h"

#include "render/renderer.h"
#include "render/texture.h"

#include <AvaFormatLib/util/byte_array_buffer.h>
#include <AvaFormatLib/util/byte_vector_stream.h>

namespace jcmr::game::justcause3
{
void RenderBlock::read_textures(std::istream& stream)
{
    u32 num_textures;
    stream.read((char*)&num_textures, sizeof(u32));

    m_textures.resize(num_textures);

    for (u32 i = 0; i < num_textures; ++i) {
        u32 length;
        stream.read((char*)&length, sizeof(u32));

        if (length == 0) continue;

        auto filename = std::make_unique<char[]>(length + 1);
        stream.read(filename.get(), length);
        filename[length] = '\0';

        m_textures[i] = m_app.get_game().create_texture(filename.get());
    }

    // read material params
    stream.read((char*)&m_material_params, sizeof(m_material_params));
    LOG_INFO("RenderBlock : m_material_params={} {} {} {}", m_material_params[0], m_material_params[1],
             m_material_params[2], m_material_params[3]);
}

void RenderBlock::write_textures(ByteArray* buffer)
{
    ava::utils::ByteVectorStream stream(buffer);

    // write texture count
    u32 num_textures = (u32)m_textures.size();
    stream.write(num_textures);

    // write texture paths
    for (u32 i = 0; i < num_textures; ++i) {
        auto texture = m_textures[i];
        if (texture) {
            auto& filename = m_textures[i]->get_filename();
            u32   length   = (u32)filename.length();

            // write filename string
            stream.writeString(filename.c_str(), length);
        } else {
            // write an empty string (0 length)
            stream.writeString(nullptr, 0);
        }
    }

    // write material params
    stream.write((char*)&m_material_params, sizeof(m_material_params));
}

Buffer* RenderBlock::read_buffer(std::istream& stream, u32 type, u32 stride)
{
    ASSERT(stride > 0);

    u32 count;
    stream.read((char*)&count, sizeof(u32));

    u64  byte_size = (count * stride);
    auto buffer    = std::make_unique<char[]>(byte_size);
    stream.read((char*)buffer.get(), byte_size);

    return m_app.get_renderer().create_buffer(buffer.get(), count, stride, type, "RenderBlock buffer");
}

void RenderBlock::write_buffer(Buffer* buf, ByteArray* buffer)
{
    ava::utils::ByteVectorStream stream(buffer);

    stream.write(buf->count);
    stream.write(buf->data);
}
} // namespace jcmr::game::justcause3