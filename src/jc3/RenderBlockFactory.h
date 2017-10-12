#pragma once

#include <cstdint>
#include <singleton.h>

class IRenderBlock;
class RenderBlockFactory
{
public:
    RenderBlockFactory() = default;
    virtual ~RenderBlockFactory() = default;

    static IRenderBlock* CreateRenderBlock(const uint32_t type);
};