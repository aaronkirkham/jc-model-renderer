#ifndef RENDERBLOCKFACTORY_H
#define RENDERBLOCKFACTORY_H

#include <QMap>
#include <QDebug>
#include "renderblocks/IRenderBlock.h"

class RenderBlockFactory
{
private:
    QMap<uint32_t, IRenderBlock*> m_RenderBlockTypes;

public:
    RenderBlockFactory();

    IRenderBlock* GetRenderBlock(uint32_t typeHash)
    {
        if (!IsValidRenderBlock(typeHash)) {
            Q_ASSERT(false);
            return false;
        }

        return m_RenderBlockTypes[typeHash];
    }

    bool IsValidRenderBlock(uint32_t typeHash) { return m_RenderBlockTypes.find(typeHash) != m_RenderBlockTypes.end(); }
};

#endif // RENDERBLOCKFACTORY_H
