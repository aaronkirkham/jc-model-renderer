#ifndef RBMLOADER_H
#define RBMLOADER_H

#include <MainWindow.h>

struct CAABox
{
    QVector3D m_Min;
    QVector3D m_Max;
};

#pragma pack(push, 1)
struct RenderBlockModel
{
    // Header
    uint32_t magicLength;
    uint8_t magic[5];
    uint32_t versionMajor;
    uint32_t versionMinor;
    uint32_t versionRevision;

    // Block information
    CAABox boundingBox;
    uint32_t numberOfBlocks;
    uint32_t flags;
};

struct PackedVertex
{
    int16_t m_Position[4];
};

struct UnknownStruct
{
    int16_t coords[3];
    int16_t normals[3];
    float uv[2];
};
#pragma pack(pop)

QT_FORWARD_DECLARE_CLASS(RenderBlockFactory)
QT_FORWARD_DECLARE_CLASS(IRenderBlock)

class RBMLoader : public Singleton<RBMLoader>
{
private:
    RenderBlockFactory* m_RenderBlockFactory = nullptr;
    IRenderBlock* m_CurrentRenderBlock = nullptr;

public:
    RBMLoader();
    ~RBMLoader();

    void ReadFile(const QString& filename);
    IRenderBlock* GetCurrentRenderBlock() { return m_CurrentRenderBlock; }

    template <typename T>
    inline void Read(QDataStream& data, T& value) noexcept
    {
        data.readRawData((char *)&value, sizeof(T));
    }

    inline void Read(QDataStream& data, QString& value) noexcept
    {
        uint32_t length;
        Read(data, length);

        // ugly..
        auto str = new char[length + 1];
        data.readRawData(str, length);
        str[length] = '\0';

        value = QString(str);
        delete[] str;
    }

    inline void ReadVertexBuffer(QDataStream& data, QVector<GLfloat>* vertices, bool compressed = false) noexcept
    {
        if (compressed)
        {
            static auto expand = [](int16_t value) -> GLfloat {
                if (value == -1)
                    return -1.0f;

                return (value * (1.0f / 32767));
            };

            // read vertices
            {
                uint32_t verticesCount;
                Read(data, verticesCount);

                for (uint32_t i = 0; i < verticesCount; i++)
                {
                    PackedVertex vertex;
                    Read(data, vertex);

                    vertices->push_back(expand(vertex.m_Position[0]));
                    vertices->push_back(expand(vertex.m_Position[1]));
                    vertices->push_back(expand(vertex.m_Position[2]));
                }
            }

            // read uvs?
            {
                uint32_t uvs;
                Read(data, uvs);

                for (uint32_t i = 0; i < uvs; i++)
                {
                    UnknownStruct unk;
                    Read(data, unk);
                }
            }
        }
        else
        {
            qDebug() << "TODO: Implement reading of unpacked vertices.";
            Q_ASSERT(false);
        }
    }

    inline void ReadIndexBuffer(QDataStream& data, QVector<uint16_t>* indices) noexcept
    {
        uint32_t indicesCount;
        Read(data, indicesCount);

        // read indices
        for (uint32_t i = 0; i < indicesCount; i++)
        {
            uint16_t index;
            Read(data, index);

            indices->push_back(index);
        }
    }
};

#endif // RBMLOADER_H
