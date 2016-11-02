#ifndef IRENDERBLOCK_H
#define IRENDERBLOCK_H

#include <MainWindow.h>

class IRenderBlock
{
protected:
    QVector<QString> m_Materials;
    Buffer m_Buffer;

public:
    virtual ~IRenderBlock() = default;

    virtual void Reset()
    {
        m_Materials.clear();
        m_Buffer.Destroy();
    }

    virtual void Read(QDataStream& data) = 0;

    virtual QVector<QString> GetMaterials() const { return m_Materials; }
    virtual Buffer* GetBuffer() { return &m_Buffer; }
};

#endif // IRENDERBLOCK_H
