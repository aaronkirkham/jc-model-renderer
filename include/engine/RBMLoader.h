#ifndef RBMLOADER_H
#define RBMLOADER_H

#include <QString>
#include <QFile>
#include <QVector3D>
#include <QVector4D>
#include "RenderBlockFactory.h"

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

class RBMLoader
{
private:
    RenderBlockFactory* m_RenderBlockFactory = nullptr;
    IRenderBlock* m_CurrentRenderBlock = nullptr;
    QFile* m_File = nullptr;
    qint64 m_FileReadOffset = 0;

public:
    RBMLoader();
    ~RBMLoader();

    void OpenFile(const QString& filename);

    void SetReadOffset(qint64 offset) { m_FileReadOffset = offset; }
    qint64 GetReadOffset() const { return m_FileReadOffset; }

    IRenderBlock* GetCurrentRenderBlock() { return m_CurrentRenderBlock; }

    template <typename T>
    inline void Read(T& value) noexcept
    {
        if (!m_File)
            return;

        m_File->seek(m_FileReadOffset);
        m_File->read((char *)&value, sizeof(T));
        m_FileReadOffset += sizeof(T);
    }

    inline void Read(QString& value) noexcept
    {
        if (!m_File)
            return;

        uint32_t length;
        Read(length);

        m_File->seek(m_FileReadOffset);
        value = m_File->read(length);
        m_FileReadOffset += length;
    }

    inline void ReadLength(QByteArray& value, uint32_t length) noexcept
    {
        m_File->seek(m_FileReadOffset);
        value = m_File->read(length);
        m_FileReadOffset += length;
    }

    inline void ReadVertexBuffer(uint32_t stride, VertexBuffer* vertexBuffer) noexcept
    {
        uint32_t vertices;
        Read(vertices);
        qDebug() << "Vertices:" << vertices;

        // read vertices
        QByteArray bytes;
        ReadLength(bytes, (stride * vertices));

        //vertexBuffer->Create<GLfloat>(0);
    }

    struct PackedVertex
    {
        uint16_t m_Position[4];
    };

    inline void ReadPackedVertexBuffer(uint32_t stride, VertexBuffer* vertexBuffer) noexcept
    {
        static auto unpack = [](uint16_t value) -> GLfloat {
            if (value == -1)
                return -1.0f;

            return (value * (1.0f / 32767));
        };

        uint32_t vertices;
        Read(vertices);
        qDebug() << "Vertices:" << vertices << "" << (vertices * stride);

        QVector<GLfloat> buffer;
        for (int i = 0; i < vertices; i++) {
            PackedVertex vertex;
            Read(vertex);

            buffer.push_back(unpack(vertex.m_Position[0]));
            buffer.push_back(unpack(vertex.m_Position[1]));
            buffer.push_back(unpack(vertex.m_Position[2]));
        }

        qDebug() << buffer;
        vertexBuffer->Create(buffer);
    }

    inline void ReadIndexBuffer(IndexBuffer* indexBuffer) noexcept
    {
        uint32_t indices;
        Read(indices);
        qDebug() << "Indices:" << indices;

        // read indices
        QByteArray bytes;
        ReadLength(bytes, (2 * indices));

        //indexBuffer->Create<uint16_t>(0);
    }
};

#endif // RBMLOADER_H
