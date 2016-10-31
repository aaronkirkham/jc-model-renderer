#ifndef RENDERBLOCKGENERALJC3_H
#define RENDERBLOCKGENERALJC3_H

#include "irenderblock.h"
#include "../RBMLoader.h"
#include <QDebug>

#pragma pack(push, 1)
struct SPackedAttribute
{
    int format;
    char pad[0x1C];
};

struct GeneralJC3Attribute
{
    char pad[0x34];
    SPackedAttribute packed;
    char pad2[0x4];
};

struct GeneralJC3
{
    uint8_t version;
    GeneralJC3Attribute attributes;
    int32_t textureCount;
};
#pragma pack(pop)

class RenderBlockGeneralJC3 : public IRenderBlock
{
private:
    VertexBuffer m_VertexBuffer;
    VertexBuffer m_VertexBuffer2;
    IndexBuffer m_IndexBuffer;

public:
    RenderBlockGeneralJC3() = default;

    virtual void Read(RBMLoader* loader) override
    {
        qDebug() << "RenderBlockGeneralJC3";

        // read general block info
        GeneralJC3 block;
        loader->Read(block);

        // read textures
        for (int i = 0; i < block.textureCount; i++) {
            QString t;
            loader->Read(t);

            qDebug() << t;
        }

        // unknown stuff
        uint32_t unk[4];
        loader->Read(unk);

        // read vertices
        if (block.attributes.packed.format == 1) {
            loader->ReadPackedVertexBuffer(8, &m_VertexBuffer);
            loader->ReadPackedVertexBuffer(20, &m_VertexBuffer2);
        } else {
            loader->ReadVertexBuffer(40, &m_VertexBuffer);
        }

        // read indices
        loader->ReadIndexBuffer(&m_IndexBuffer);
    }

    virtual VertexBuffer* GetVertexBuffer() override { return &m_VertexBuffer2; }
    virtual IndexBuffer* GetIndexBuffer() override { return &m_IndexBuffer; }
};

#endif // RENDERBLOCKGENERALJC3_H
