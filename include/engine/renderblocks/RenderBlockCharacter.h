#ifndef RENDERBLOCKCHARACTER_H
#define RENDERBLOCKCHARACTER_H

#include <MainWindow.h>

#pragma pack(push, 1)
struct CharacterAttributes
{
    uint32_t flags;
    float scale;
    char pad[0x64];
};

struct Character
{
    uint8_t version;
    CharacterAttributes attributes;
};
#pragma pack(pop)

class RenderBlockCharacter : public IRenderBlock
{
public:
    RenderBlockCharacter() = default;
    virtual ~RenderBlockCharacter() = default;

    int64_t GetStride(uint32_t flags) const {
        int strides[] = {
            0x18,
            0x1C,
            0x20,
            0x20,
            0x24,
            0x28
        };

        return strides[3 * ((flags >> 1) & 1) + ((flags >> 5) & 1) + ((flags >> 4) & 1)];
    }

    virtual void Read(QDataStream& data) override
    {
        Reset();

        // read general block info
        Character block;
        RBMLoader::instance()->Read(data, block);

        // read materials
        m_Materials.Read(data);

        RBMLoader::instance()->ReadVertexBuffer(data, &m_Buffer.m_Vertices, true, GetStride(block.attributes.flags));
        auto ReadSkinBatch = [](QDataStream &data) {
            uint32_t newSize;
            data.readRawData((char*)&newSize, 4);
            uint32_t i = 0;
            do 
            {
                uint32_t size;
                data.readRawData((char*)&size, 4);
                uint32_t offset;
                data.readRawData((char*)&offset, 4);
                uint32_t batchSize;
                data.readRawData((char*)&batchSize, 4);
                for (uint32_t n = 0; n < batchSize; ++n) {
                    uint16_t unknown;
                    data.readRawData((char*)&unknown, 2);
                }
                ++i;
            } while (i < newSize);
        };
        ReadSkinBatch(data);

        RBMLoader::instance()->ReadIndexBuffer(data, &m_Buffer.m_Indices);
    }
};

#endif // RENDERBLOCKCHARACTER_H
