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

        // read vertices data
        // TODO: need to implement the different vertex types depending on the flags above (GetStride).
        // The stride should be the size of the struct we read from.
        {
            QVector<Vertex::PackedCharacterPos4> result;
            RBMLoader::instance()->ReadVertexBuffer(data, &result);

            for (Vertex::PackedCharacterPos4 &vertexData : result)
            {
                m_Buffer.m_Vertices.push_back(Vertex::expand(vertexData.x) * block.attributes.scale);
                m_Buffer.m_Vertices.push_back(Vertex::expand(vertexData.y) * block.attributes.scale);
                m_Buffer.m_Vertices.push_back(Vertex::expand(vertexData.z) * block.attributes.scale);

                m_Buffer.m_TexCoords.push_back(Vertex::expand(vertexData.u0));
                m_Buffer.m_TexCoords.push_back(Vertex::expand(vertexData.v0));
            }
        }

        ReadSkinBatch(data);

        RBMLoader::instance()->ReadIndexBuffer(data, &m_Buffer.m_Indices);
    }
};

#endif // RENDERBLOCKCHARACTER_H
