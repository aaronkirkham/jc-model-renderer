#ifndef IRENDERBLOCK_H
#define IRENDERBLOCK_H

#include <MainWindow.h>

QT_FORWARD_DECLARE_CLASS(RBMLoader)

class IRenderBlock
{
public:
    virtual ~IRenderBlock() = default;

    virtual void Read(RBMLoader* loader) = 0;

    virtual QVector<QString> GetMaterials() const = 0;
    virtual VertexBuffer* GetVertexBuffer() = 0;
    virtual IndexBuffer* GetIndexBuffer() = 0;
};

#endif // IRENDERBLOCK_H
