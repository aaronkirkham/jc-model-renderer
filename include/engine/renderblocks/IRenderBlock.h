#ifndef IRENDERBLOCK_H
#define IRENDERBLOCK_H

#include <MainWindow.h>

QT_FORWARD_DECLARE_CLASS(RBMLoader)

class IRenderBlock
{
protected:
    QVector<QString> m_Materials;
    VertexBuffer m_VertexBuffer;
    IndexBuffer m_IndexBuffer;

public:
    virtual ~IRenderBlock() = default;

    virtual void Reset()
    {
        m_Materials.clear();
        m_VertexBuffer.Destroy();
        m_IndexBuffer.Destroy();
    }

    virtual void Read(QDataStream& data) = 0;

    virtual QVector<QString> GetMaterials() const { return m_Materials; }
    virtual VertexBuffer* GetVertexBuffer() { return &m_VertexBuffer; }
    virtual IndexBuffer* GetIndexBuffer() { return &m_IndexBuffer; }
};

#endif // IRENDERBLOCK_H
