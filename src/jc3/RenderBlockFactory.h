#pragma once

#include <cstdint>

class IRenderBlock;
class RenderBlockFactory
{
public:
    RenderBlockFactory() = default;
    virtual ~RenderBlockFactory() = default;

    static IRenderBlock* CreateRenderBlock(const uint32_t type);
};