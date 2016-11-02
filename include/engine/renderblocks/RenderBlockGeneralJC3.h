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
public:
    RenderBlockGeneralJC3() = default;
    virtual ~RenderBlockGeneralJC3() = default;

    virtual void Read(QDataStream& data) override
    {
        Reset();

        static auto unpack = [](uint16_t value) -> GLfloat {
            if (value == -1)
                return -1.0f;

            return (value * (1.0f / 32767));
        };

        // read general block info
        GeneralJC3 block;
        RBMLoader::instance()->Read(data, block);

        // read textures
        {
            for (uint32_t i = 0; i < block.textureCount; i++)
            {
                QString material;
                RBMLoader::instance()->Read(data, material);
                m_Materials.push_back(material);
            }
        }

        // unknown stuff
        uint32_t unk[4];
        RBMLoader::instance()->Read(data, unk);

        // read buffers
        if (block.attributes.packed.format == 1)
        {
            QVector<GLfloat> buffer;

            // vertices
            {
                uint32_t vertices;
                RBMLoader::instance()->Read(data, vertices);

                for (uint32_t i = 0; i < vertices; i++) {
                    PackedVertex vertex;
                    RBMLoader::instance()->Read(data, vertex);

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
                RBMLoader::instance()->Read(data, uvs);

                for (uint32_t i = 0; i < uvs; i++) {
                    Vec4Buffer buffer;
                    RBMLoader::instance()->Read(data, buffer);
                }
            }

            m_VertexBuffer.Setup(buffer);
        }

        // read indices
        {
            uint32_t indices;
            RBMLoader::instance()->Read(data, indices);

            // read indices
            QVector<uint16_t> buffer;
            for (uint32_t i = 0; i < indices; i++) {
                uint16_t val;
                RBMLoader::instance()->Read(data, val);

                buffer.push_back(val);
            }

            m_IndexBuffer.Setup(buffer);
        }
    }
};

#endif // RENDERBLOCKGENERALJC3_H
