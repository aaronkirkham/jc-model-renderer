#ifndef RENDERBLOCKFACTORY_H
#define RENDERBLOCKFACTORY_H

#include <MainWindow.h>

QT_FORWARD_DECLARE_CLASS(IRenderBlock)

class RenderBlockFactory
{
private:
    QMap<uint32_t, IRenderBlock*> m_RenderBlockTypes;

public:
    RenderBlockFactory();

    IRenderBlock* GetRenderBlock(uint32_t typeHash);
   

    bool IsValidRenderBlock(uint32_t typeHash) { return m_RenderBlockTypes.find(typeHash) != m_RenderBlockTypes.end(); }
};

#endif // RENDERBLOCKFACTORY_H
