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

    virtual Buffer* GetBuffer() { return &m_Buffer; }
    virtual Materials* GetMaterials() { return &m_Materials; }
};

#endif // IRENDERBLOCK_H
