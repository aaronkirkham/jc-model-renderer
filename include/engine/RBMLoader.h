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
#pragma pack(pop)

QT_FORWARD_DECLARE_CLASS(RenderBlockFactory)
QT_FORWARD_DECLARE_CLASS(IRenderBlock)

class RBMLoader : public Singleton<RBMLoader>
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
};

#endif // RBMLOADER_H
