#ifndef RENDERBLOCKCARPAINTMM_H
#define RENDERBLOCKCARPAINTMM_H

#include <MainWindow.h>

#pragma pack(push, 1)
struct SCarpaintMMAttribute
{
    unsigned int flags;
    float unknown;
};

struct CarPaintMM
{
    uint8_t version;
    SCarpaintMMAttribute attributes;
    char pad[0x13C];
};

#pragma pack(pop)

class RenderBlockCarPaintMM : public IRenderBlock
{
public:
    RenderBlockCarPaintMM() = default;
    virtual ~RenderBlockCarPaintMM() = default;

    virtual uint8_t GetTexCoordSize() const override { return 4; }

    virtual void Read(QDataStream& data) override
    {
        Reset();

        // read general block info
        CarPaintMM block;
        RBMLoader::instance()->Read(data, block);


        // Read some unkown stuff
        {char unknown[0x4C];
        data.readRawData(unknown, 0x4C); }

        // Read the deform table
        auto ReadDeformTable = [](QDataStream& data) {
            uint32_t unknown;
            for(auto i = 0; i < 0x100; ++i) {
                data.readRawData((char*)&unknown, 4);
            }
        };

        ReadDeformTable(data);

        // read materials
        m_Materials.Read(data);

        // read vertex & index buffers
        if(_bittest((const long*)&block.attributes.flags, 0xD)) {
            QVector<Vertex::UnpackedVertexWithNormal1> result;
            RBMLoader::instance()->ReadVertexBuffer(data, &result);
            for (auto &vertexData : result)
            {
                m_Buffer.m_Vertices.push_back(vertexData.x);
                m_Buffer.m_Vertices.push_back(vertexData.y);
                m_Buffer.m_Vertices.push_back(vertexData.z);
            }
            QVector<Vertex::VertexNormals> normals;
            RBMLoader::instance()->ReadVertexBuffer(data, &normals);
            for (auto &vertexData : normals)
            {
                m_Buffer.m_TexCoords.push_back(vertexData.u);
                m_Buffer.m_TexCoords.push_back(vertexData.v);
                m_Buffer.m_TexCoords.push_back(vertexData.u2);
                m_Buffer.m_TexCoords.push_back(vertexData.v2);
            }
            ReadSkinBatch(data);
        } else if(_bittest((const long*)&block.attributes.flags, 0xC)) {
            // Has deform information
            QVector<Vertex::VertexDeformPos> result;
            RBMLoader::instance()->ReadVertexBuffer(data, &result);
            for (auto &vertexData : result)
            {
                m_Buffer.m_Vertices.push_back(vertexData.x);
                m_Buffer.m_Vertices.push_back(vertexData.y);
                m_Buffer.m_Vertices.push_back(vertexData.z);
            }
            QVector<Vertex::VertexDeformNormal2> normals;
            RBMLoader::instance()->ReadVertexBuffer(data, &normals);
            for (auto &vertexData : normals)
            {
                m_Buffer.m_TexCoords.push_back(vertexData.u);
                m_Buffer.m_TexCoords.push_back(vertexData.v);
                m_Buffer.m_TexCoords.push_back(vertexData.u2);
                m_Buffer.m_TexCoords.push_back(vertexData.v2);
            }
        } else {
            QVector<Vertex::UnpackedVertexPosition> result;
            RBMLoader::instance()->ReadVertexBuffer(data, &result);
            for (auto &vertexData : result)
            {
                m_Buffer.m_Vertices.push_back(vertexData.x);
                m_Buffer.m_Vertices.push_back(vertexData.y);
                m_Buffer.m_Vertices.push_back(vertexData.z);
            }
            QVector<Vertex::VertexNormals> normals;
            RBMLoader::instance()->ReadVertexBuffer(data, &normals);
            for (auto &vertexData : normals)
            {
                m_Buffer.m_TexCoords.push_back(vertexData.u);
                m_Buffer.m_TexCoords.push_back(vertexData.v);
                m_Buffer.m_TexCoords.push_back(vertexData.u2);
                m_Buffer.m_TexCoords.push_back(vertexData.v2);
            }
        }

        auto v12 = (block.attributes.flags & 0x60) == 0;
        if (!v12) {
            QVector<Vertex::PackedTex2UV> uvs;
            RBMLoader::instance()->ReadVertexBuffer(data, &uvs);
         
        }

        RBMLoader::instance()->ReadIndexBuffer(data, &m_Buffer.m_Indices);
    }
};

#endif // RENDERBLOCKCARPAINTMM_H
 
