#include "irenderblock.h"

#include "graphics/renderer.h"
#include "graphics/texture.h"
#include "graphics/texture_manager.h"
#include "graphics/types.h"

namespace jc3
{
void IRenderBlock::DrawSkinBatches(RenderContext_t* context, const std::vector<jc3::CSkinBatch>& skin_batches)
{
    if (!m_Visible)
        return;

    for (const auto& batch : skin_batches) {
        context->m_Renderer->DrawIndexed(batch.m_Offset, batch.m_Size, m_IndexBuffer);
    }
}

IBuffer_t* IRenderBlock::ReadBuffer(std::istream& stream, BufferType type, uint32_t stride)
{
    assert(stride > 0);

    uint32_t count;
    stream.read((char*)&count, sizeof(count));

    auto buffer = std::make_unique<char[]>(count * stride);
    stream.read((char*)buffer.get(), (count * stride));

    return Renderer::Get()->CreateBuffer(buffer.get(), count, stride, type, "IRenderBlock Buffer");
}

void IRenderBlock::ReadMaterials(std::istream& stream)
{
    uint32_t count;
    stream.read((char*)&count, sizeof(count));

    m_Textures.resize(count);

    for (uint32_t i = 0; i < count; ++i) {
        uint32_t length;
        stream.read((char*)&length, sizeof(length));

        if (length == 0) {
            continue;
        }

        auto filename = std::make_unique<char[]>(length + 1);
        stream.read(filename.get(), length);
        filename[length] = '\0';

        auto texture = TextureManager::Get()->GetTexture(filename.get());
        if (texture) {
            m_Textures[i] = std::move(texture);
        }
    }

    stream.read((char*)&m_MaterialParams, sizeof(m_MaterialParams));
}

void IRenderBlock::WriteMaterials(std::ostream& stream)
{
    auto count = static_cast<uint32_t>(m_Textures.size());
    stream.write((char*)&count, sizeof(count));

    for (uint32_t i = 0; i < count; ++i) {
        const auto& texture = m_Textures[i];
        if (texture) {
            const auto& filename = m_Textures[i]->GetFileName().generic_string();
            const auto  length   = static_cast<uint32_t>(filename.length());

            stream.write((char*)&length, sizeof(length));
            stream.write(filename.c_str(), length);
        } else {
            static auto ZERO = 0;
            stream.write((char*)&ZERO, sizeof(ZERO));
        }
    }

    stream.write((char*)&m_MaterialParams, sizeof(m_MaterialParams));
}
}; // namespace jc3
