#ifndef RENDERBLOCKGENERALJC3_H
#define RENDERBLOCKGENERALJC3_H

#include <MainWindow.h>

#pragma pack(push, 1)
struct SPackedAttribute
{
    int format;
    float scale;
    char pad[0x18];
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
    uint32_t textureCount;
};
#pragma pack(pop)

struct PackedVertex
{
    uint16_t m_Position[4];
};

struct Vec4Buffer
{
    float m_Position[5];
};

class RenderBlockGeneralJC3 : public IRenderBlock
{
private:
    QVector<QString> m_Materials;
    VertexBuffer m_VertexBuffer;
    IndexBuffer m_IndexBuffer;

public:
    RenderBlockGeneralJC3() = default;

    virtual void Read(RBMLoader* loader) override
    {
        static auto unpack = [](uint16_t value) -> GLfloat {
            if (value == -1)
                return -1.0f;

            return (value * (1.0f / 32767));
        };

        // read general block info
        GeneralJC3 block;
        loader->Read(block);

        // read textures
        {
            for (uint32_t i = 0; i < block.textureCount; i++)
            {
                QString material;
                loader->Read(material);
                m_Materials.push_back(material);
            }
        }

        qDebug() << m_Materials.at(0);

        // unknown stuff
        uint32_t unk[4];
        loader->Read(unk);

        // read buffers
        if (block.attributes.packed.format == 1)
        {
            QVector<GLfloat> buffer;

            // vertices
            {
                uint32_t vertices;
                loader->Read(vertices);

                for (uint32_t i = 0; i < vertices; i++) {
                    PackedVertex vertex;
                    loader->Read(vertex);

                    buffer.push_back(unpack(vertex.m_Position[0]) * block.attributes.packed.scale);
                    buffer.push_back(unpack(vertex.m_Position[1]) * block.attributes.packed.scale);
                    buffer.push_back(unpack(vertex.m_Position[2]) * block.attributes.packed.scale);
                    //buffer.push_back(unpack(vertex.m_Position[3]) * block.attributes.packed.scale);

                    //qDebug() << unpack(vertex.m_Position[0]) << unpack(vertex.m_Position[1]) << unpack(vertex.m_Position[2]);
                }
            }

            // read uvs
            {
                uint32_t uvs;
                loader->Read(uvs);

                for (uint32_t i = 0; i < uvs; i++) {
                    Vec4Buffer buffer;
                    loader->Read(buffer);
                }
            }

            m_VertexBuffer.Create(buffer);
            qDebug() << "vertex buffer count:" << m_VertexBuffer.m_Count;
        }

        // read indices
        {
            uint32_t indices;
            loader->Read(indices);
            qDebug() << "Indices:" << indices;

            // read indices
            QVector<uint16_t> buffer;
            for (uint32_t i = 0; i < indices; i++) {
                uint16_t val;
                loader->Read(val);

                buffer.push_back(val);
            }

            m_IndexBuffer.Create(buffer);
        }
    }

    virtual QVector<QString> GetMaterials() const override { return m_Materials; }
    virtual VertexBuffer* GetVertexBuffer() override { return &m_VertexBuffer; }
    virtual IndexBuffer* GetIndexBuffer() override { return &m_IndexBuffer; }
};

#endif // RENDERBLOCKGENERALJC3_H
