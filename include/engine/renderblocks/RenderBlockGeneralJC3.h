#ifndef RENDERBLOCKGENERALJC3_H
#define RENDERBLOCKGENERALJC3_H

#include <MainWindow.h>

#pragma pack(push, 1)
struct SPackedAttribute
{
    int32_t format;
    float scale;
    char pad[0x18];
};

struct GeneralJC3Attributes
{
    char pad[0x34];
    SPackedAttribute packed;
    char pad2[0x4];
};

struct GeneralJC3
{
    uint8_t version;
    GeneralJC3Attributes attributes;
};
#pragma pack(pop)

class RenderBlockGeneralJC3 : public IRenderBlock
{
public:
    RenderBlockGeneralJC3() = default;
    virtual ~RenderBlockGeneralJC3() = default;

    virtual void Read(QDataStream& data) override
    {
        Reset();

        // read general block info
        GeneralJC3 block;
        RBMLoader::instance()->Read(data, block);

        // read materials
        m_Materials.Read(data);

        // read vertex & index buffers
        RBMLoader::instance()->ReadVertexBuffer(data, &m_Buffer.m_Vertices, block.attributes.packed.format == 1);
        RBMLoader::instance()->ReadIndexBuffer(data, &m_Buffer.m_Indices);
    }
};

#endif // RENDERBLOCKGENERALJC3_H
