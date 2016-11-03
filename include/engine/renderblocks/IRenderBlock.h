#ifndef IRENDERBLOCK_H
#define IRENDERBLOCK_H

#include <MainWindow.h>
#include <QOpenGLTexture>

class IRenderBlock
{
protected:
    Buffer m_Buffer;
    Materials m_Materials;

public:
    virtual ~IRenderBlock() = default;

    virtual void Reset()
    {
        m_Materials.Reset();
        m_Buffer.Destroy();
    }

    virtual void Read(QDataStream& data) = 0;

    void ReadSkinBatch(QDataStream &data) {
        uint32_t newSize;
        data.readRawData((char*)&newSize, 4);
        uint32_t i = 0;
        do
        {
            uint32_t size;
            data.readRawData((char*)&size, 4);
            uint32_t offset;
            data.readRawData((char*)&offset, 4);
            uint32_t batchSize;
            data.readRawData((char*)&batchSize, 4);
            for (uint32_t n = 0; n < batchSize; ++n) {
                uint16_t unknown;
                data.readRawData((char*)&unknown, 2);
            }
            ++i;
        } while (i < newSize);
    }

    virtual Buffer* GetBuffer() { return &m_Buffer; }
    virtual Materials* GetMaterials() { return &m_Materials; }
};

#endif // IRENDERBLOCK_H
