#ifndef RENDERBLOCKBUILDINGJC3_H
#define RENDERBLOCKBUILDINGJC3_H

#include <MainWindow.h>

#pragma pack(push, 1)
struct BuildingJC3Attributes
{
    char pad[0x68];
    SPackedAttribute packed;
    char pad2[0x1C];
};

struct BuildingJC3
{
    uint8_t version;
    BuildingJC3Attributes attributes;
};
#pragma pack(pop)

class RenderBlockBuildingJC3 : public IRenderBlock
{
public:
    RenderBlockBuildingJC3() = default;
    virtual ~RenderBlockBuildingJC3() = default;

    virtual void Read(QDataStream& data) override
    {
        Reset();

        // read general block info
        BuildingJC3 block;
        RBMLoader::instance()->Read(data, block);

        // read materials
        m_Materials.Read(data);

        // is the block compressed?
        if (block.attributes.packed.format == 1)
        {
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
                QVector<Vertex::GeneralShortPacked> result;
                RBMLoader::instance()->ReadVertexBuffer(data, &result);

                for (auto &vertexData : result)
                {
                    m_Buffer.m_TexCoords.push_back(Vertex::expand(vertexData.u0) * block.attributes.packed.uv0Extent.x());
                    m_Buffer.m_TexCoords.push_back(Vertex::expand(vertexData.v0) * block.attributes.packed.uv0Extent.y());
                    m_Buffer.m_TexCoords.push_back(Vertex::expand(vertexData.u1) * block.attributes.packed.uv1Extent.x());
                    m_Buffer.m_TexCoords.push_back(Vertex::expand(vertexData.v1) * block.attributes.packed.uv1Extent.y());
                }
            }
        }
        else
        {
            // read vertices
            {
                QVector<Vertex::Unpacked> result;
                RBMLoader::instance()->ReadVertexBuffer(data, &result);

                for (auto &vertexData : result)
                {
                    m_Buffer.m_Vertices.push_back(vertexData.x * block.attributes.packed.scale);
                    m_Buffer.m_Vertices.push_back(vertexData.y * block.attributes.packed.scale);
                    m_Buffer.m_Vertices.push_back(vertexData.z * block.attributes.packed.scale);

                    m_Buffer.m_TexCoords.push_back(vertexData.u0 * block.attributes.packed.uv0Extent.x());
                    m_Buffer.m_TexCoords.push_back(vertexData.v0 * block.attributes.packed.uv0Extent.y());
                    m_Buffer.m_TexCoords.push_back(vertexData.u1 * block.attributes.packed.uv1Extent.x());
                    m_Buffer.m_TexCoords.push_back(vertexData.v1 * block.attributes.packed.uv1Extent.y());
                }
            }
        }

        RBMLoader::instance()->ReadIndexBuffer(data, &m_Buffer.m_Indices);
    }
};

#endif // RENDERBLOCKBUILDINGJC3_H
