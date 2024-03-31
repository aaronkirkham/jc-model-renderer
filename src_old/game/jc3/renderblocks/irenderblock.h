#pragma once

#include "../../irenderblock.h"

#include <fstream>

struct RenderContext_t;
namespace jc3
{
#pragma pack(push, 1)
struct CSkinBatch {
    int16_t* m_BatchLookup;
    int32_t  m_BatchSize;
    int32_t  m_Size;
    int32_t  m_Offset;
    char     alignment[4];
};
static_assert(sizeof(CSkinBatch) == 0x18, "CSkinBatch alignment is wrong!");

struct CDeformTable {
    uint16_t table[256];
    uint8_t  size = 0;
};
static_assert(sizeof(CDeformTable) == 0x201, "CDeformTable alignment is wrong!");
#pragma pack(pop)

class IRenderBlock : public ::IRenderBlock
{
  protected:
    float m_MaterialParams[4] = {0};

  public:
    virtual void Create()                    = 0;
    virtual void Read(std::istream& stream)  = 0;
    virtual void Write(std::ostream& stream) = 0;

    void DrawSkinBatches(RenderContext_t* context, const std::vector<jc3::CSkinBatch>& skin_batches);

    IBuffer_t* ReadBuffer(std::istream& stream, BufferType type, uint32_t stride);

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

    void ReadMaterials(std::istream& stream);
    void WriteMaterials(std::ostream& stream);

    void MakeEmptyMaterials(uint8_t count);

    void ReadSkinBatch(std::istream& stream, std::vector<jc3::CSkinBatch>* skin_batches)
    {
        uint32_t count;
        stream.read((char*)&count, sizeof(count));
        skin_batches->reserve(count);

        for (uint32_t i = 0; i < count; ++i) {
            jc3::CSkinBatch batch;

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

    void WriteSkinBatch(std::ostream& stream, std::vector<jc3::CSkinBatch>* skin_batches)
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

    void ReadDeformTable(std::istream& stream, jc3::CDeformTable* deform_table)
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

    void WriteDeformTable(std::ostream& stream, jc3::CDeformTable* deform_table)
    {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t data = deform_table->table[i];
            stream.write((char*)&data, sizeof(data));
        }
    }
};
}; // namespace jc3
