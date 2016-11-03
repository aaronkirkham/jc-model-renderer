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
            RBMLoader::instance()->ReadVertexBuffer(data, &m_Buffer.m_Vertices, false, 0x10);
            QVector<GLfloat> unknownVerts;
            RBMLoader::instance()->ReadVertexBuffer(data, &unknownVerts, true, 0x18);
            ReadSkinBatch(data);
        } else if(_bittest((const long*)&block.attributes.flags, 0xC)) {
            RBMLoader::instance()->ReadVertexBuffer(data, &m_Buffer.m_Vertices, false, 0x18);
            QVector<GLfloat> unknownVerts;
            RBMLoader::instance()->ReadVertexBuffer(data, &unknownVerts, true, 0x20);
        } else {
            RBMLoader::instance()->ReadVertexBuffer(data, &m_Buffer.m_Vertices, true, 0xC);
            QVector<GLfloat> unknownVerts;
            RBMLoader::instance()->ReadVertexBuffer(data, &unknownVerts, true, 0x18);

            qDebug() << "This format is currently not supported";
        }

        auto v12 = (block.attributes.flags & 0x60) == 0;
        if (!v12) {
            QVector<GLfloat> unknownVerts;
            RBMLoader::instance()->ReadVertexBuffer(data, &unknownVerts, true, 8);
        }

        RBMLoader::instance()->ReadIndexBuffer(data, &m_Buffer.m_Indices);
    }
};

#endif // RENDERBLOCKCARPAINTMM_H
 