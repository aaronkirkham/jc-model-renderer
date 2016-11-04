#ifndef RENDERBLOCKFACTORY_H
#define RENDERBLOCKFACTORY_H

#include <MainWindow.h>

QT_FORWARD_DECLARE_CLASS(IRenderBlock)

class RenderBlockFactory
{
public:
    RenderBlockFactory();
    virtual ~RenderBlockFactory() = default;

    IRenderBlock* GetRenderBlock(uint32_t typeHash);
};

#endif // RENDERBLOCKFACTORY_H
