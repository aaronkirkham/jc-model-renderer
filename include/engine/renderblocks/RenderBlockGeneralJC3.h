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

struct PackedVertex
{
    int16_t m_Position[4];
};

struct Vec4Buffer
{
    int16_t coords[3];
    int16_t normals[3];
    float uv[2];
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

        static auto unpack = [](int16_t value) -> GLfloat {
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

        QVector<GLfloat> vertices;
        QVector<uint16_t> indices;

        // read buffers
        if (block.attributes.packed.format == 1)
        {
            // vertices
            {
                uint32_t verticesCount;
                RBMLoader::instance()->Read(data, verticesCount);

                for (uint32_t i = 0; i < verticesCount; i++)
                {
                    PackedVertex vertex;
                    RBMLoader::instance()->Read(data, vertex);

                    vertices.push_back(unpack(vertex.m_Position[0]) * block.attributes.packed.scale);
                    vertices.push_back(unpack(vertex.m_Position[1]) * block.attributes.packed.scale);
                    vertices.push_back(unpack(vertex.m_Position[2]) * block.attributes.packed.scale);
                    //vertices.push_back(unpack(vertex.m_Position[3]) * block.attributes.packed.scale);

                    //qDebug() << unpack(vertex.m_Position[0]) << unpack(vertex.m_Position[1]) << unpack(vertex.m_Position[2]);
                }
            }

            // read uvs
            {
                uint32_t uvs;
                RBMLoader::instance()->Read(data, uvs);

                for (uint32_t i = 0; i < uvs; i++)
                {
                    Vec4Buffer buffer;
                    RBMLoader::instance()->Read(data, buffer);

                    //qDebug() << unpack(buffer.coords[0]) << unpack(buffer.coords[1]) << unpack(buffer.coords[2]);
                    //qDebug() << unpack(buffer.normals[0]) << unpack(buffer.normals[1]) << unpack(buffer.normals[2]);
                    //qDebug() << buffer.uv[0] << buffer.uv[1];
                    //qDebug() << "";
                }
            }
        }

        // read indices
        {
            uint32_t indicesCount;
            RBMLoader::instance()->Read(data, indicesCount);

            // read indices
            for (uint32_t i = 0; i < indicesCount; i++)
            {
                uint16_t val;
                RBMLoader::instance()->Read(data, val);

                indices.push_back(val);
            }

            //qDebug() << indices;
        }

        m_Buffer.Setup(vertices, indices);
    }
};

#endif // RENDERBLOCKGENERALJC3_H
