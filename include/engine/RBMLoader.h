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
};

#endif // RBMLOADER_H
