#pragma once

#include "../../irenderblock.h"

namespace jc3
{
class IRenderBlock : public ::IRenderBlock
{
  protected:
    float m_MaterialParams[4] = {0};

  public:
    virtual void Create()                    = 0;
    virtual void Read(std::istream& stream)  = 0;
    virtual void Write(std::ostream& stream) = 0;

    void DrawSkinBatches(RenderContext_t* context, const std::vector<jc::CSkinBatch>& skin_batches)
    {
        if (!m_Visible)
            return;

        for (const auto& batch : skin_batches) {
            context->m_Renderer->DrawIndexed(batch.m_Offset, batch.m_Size, m_IndexBuffer);
        }
    }

    IBuffer_t* ReadBuffer(std::istream& stream, BufferType type, uint32_t stride)
    {
        assert(stride > 0);

        uint32_t count;
        stream.read((char*)&count, sizeof(count));

        auto buffer = std::make_unique<char[]>(count * stride);
        stream.read((char*)buffer.get(), (count * stride));

        return Renderer::Get()->CreateBuffer(buffer.get(), count, stride, type, "IRenderBlock Buffer");
    }

    template <typename T> IBuffer_t* ReadVertexBuffer(std::istream& stream)
    {
        return ReadBuffer(stream, VERTEX_BUFFER, sizeof(T));
    }

    IBuffer_t* ReadIndexBuffer(std::istream& stream)
    {
        return ReadBuffer(stream, INDEX_BUFFER, sizeof(uint16_t));
    }

    void WriteBuffer(std::ostream& stream, IBuffer_t* buffer)
    {
        assert(buffer != nullptr);

        stream.write((char*)&buffer->m_ElementCount, sizeof(buffer->m_ElementCount));
        stream.write((char*)buffer->m_Data.data(), buffer->m_Data.size());
    }

    void ReadMaterials(std::istream& stream)
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

    void WriteMaterials(std::ostream& stream)
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

    void ReadSkinBatch(std::istream& stream, std::vector<jc::CSkinBatch>* skin_batches)
    {
        uint32_t count;
        stream.read((char*)&count, sizeof(count));
        skin_batches->reserve(count);

        for (uint32_t i = 0; i < count; ++i) {
            jc::CSkinBatch batch;

            stream.read((char*)&batch.m_Size, sizeof(batch.m_Size));
            stream.read((char*)&batch.m_Offset, sizeof(batch.m_Offset));
            stream.read((char*)&batch.m_BatchSize, sizeof(batch.m_BatchSize));

            // allocate the batch lookup
            if (batch.m_BatchSize != 0) {
                batch.m_BatchLookup = new int16_t[batch.m_BatchSize];
                stream.read((char*)batch.m_BatchLookup, sizeof(int16_t) * batch.m_BatchSize);
            }

            skin_batches->emplace_back(std::move(batch));
        }
    }

    void WriteSkinBatch(std::ostream& stream, std::vector<jc::CSkinBatch>* skin_batches)
    {
        uint32_t count = skin_batches->size();
        stream.write((char*)&count, sizeof(count));

        for (const auto& batch : *skin_batches) {
            stream.write((char*)&batch.m_Size, sizeof(batch.m_Size));
            stream.write((char*)&batch.m_Offset, sizeof(batch.m_Offset));
            stream.write((char*)&batch.m_BatchSize, sizeof(batch.m_BatchSize));

            if (batch.m_BatchSize != 0) {
                stream.write((char*)batch.m_BatchLookup, sizeof(int16_t) * batch.m_BatchSize);
            }
        }
    }

    void ReadDeformTable(std::istream& stream, jc::CDeformTable* deform_table)
    {
        uint32_t deformTable[256];
        stream.read((char*)&deformTable, sizeof(deformTable));

        for (uint32_t i = 0; i < 256; ++i) {
            const auto data = deformTable[i];

            deform_table->table[i] = data;
            if (data != -1 && deform_table->size < i) {
                deform_table->size = i;
            }
        }
    }

    void WriteDeformTable(std::ostream& stream, jc::CDeformTable* deform_table)
    {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t data = deform_table->table[i];
            stream.write((char*)&data, sizeof(data));
        }
    }
};
}; // namespace jc3
