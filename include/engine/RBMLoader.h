#ifndef RBMLOADER_H
#define RBMLOADER_H

#include <MainWindow.h>
#include <mutex>

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
#pragma pack(pop)

QT_FORWARD_DECLARE_CLASS(RenderBlockFactory)
QT_FORWARD_DECLARE_CLASS(IRenderBlock)

class RBMLoader : public Singleton<RBMLoader>
{
private:
    RenderBlockFactory* m_RenderBlockFactory = nullptr;
    QVector<IRenderBlock*> m_RenderBlocks;

public:
    RBMLoader();
    ~RBMLoader();

    std::recursive_mutex m_RenderBlockMutex;

    void ReadFile(const QString& filename);
    const QVector<IRenderBlock*>& GetRenderBlocks() { return m_RenderBlocks; }

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

    template <typename T>
    inline void ReadVertexBuffer(QDataStream& data, QVector<T>* output) noexcept
    {
        uint32_t verticesCount;
        Read(data, verticesCount);

        // read vertices data
        for (uint32_t i = 0; i < verticesCount; i++)
        {
            T vertexData;
            Read(data, vertexData);

            output->push_back(vertexData);
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
