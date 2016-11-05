#ifndef RENDERBLOCKLANDMARK_H
#define RENDERBLOCKLANDMARK_H

#include <MainWindow.h>

#pragma pack(push, 1)
struct LandmarkAttributes
{
    char pad[0x34];
    SPackedAttribute packed;
    char pad2[0x4];
};

struct Landmark
{
    uint8_t version;
    LandmarkAttributes attributes;
};
#pragma pack(pop)

class RenderBlockLandmark : public IRenderBlock
{
public:
    RenderBlockLandmark() = default;
    virtual ~RenderBlockLandmark() = default;

    virtual uint8_t GetTexCoordSize() const override { return 4; }

    virtual void Read(QDataStream& data) override
    {
        Reset();

        // read general block info
        Landmark block;
        RBMLoader::instance()->Read(data, block);

        // read materials
        m_Materials.Read(data);

        // read vertices
        {
            QVector<Vertex::Packed> result;
            RBMLoader::instance()->ReadVertexBuffer(data, &result);

            for (auto &vertexData : result)
            {
                m_Buffer.m_Vertices.push_back(Vertex::expand(vertexData.x) * block.attributes.packed.scale);
                m_Buffer.m_Vertices.push_back(Vertex::expand(vertexData.y) * block.attributes.packed.scale);
                m_Buffer.m_Vertices.push_back(Vertex::expand(vertexData.z) * block.attributes.packed.scale);
            }
        }

        // read uvs
        {
            QVector<Vertex::PackedTex2UV> result;
            RBMLoader::instance()->ReadVertexBuffer(data, &result);

            for (auto &vertexData : result)
            {
                m_Buffer.m_TexCoords.push_back(Vertex::expand(vertexData.u0) * block.attributes.packed.uv0Extent.x());
                m_Buffer.m_TexCoords.push_back(Vertex::expand(vertexData.v0) * block.attributes.packed.uv0Extent.y());
                m_Buffer.m_TexCoords.push_back(Vertex::expand(vertexData.u1) * block.attributes.packed.uv1Extent.x());
                m_Buffer.m_TexCoords.push_back(Vertex::expand(vertexData.v1) * block.attributes.packed.uv1Extent.y());
            }
        }

        RBMLoader::instance()->ReadIndexBuffer(data, &m_Buffer.m_Indices);
    }
};

#endif // RENDERBLOCKLANDMARK_H
