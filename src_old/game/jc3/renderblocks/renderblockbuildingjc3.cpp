#include "renderblockbuildingjc3.h"

#include "graphics/renderer.h"

#include "game/render_block_factory.h"

namespace jc3
{
RenderBlockBuildingJC3::~RenderBlockBuildingJC3()
{
    Renderer::Get()->DestroyBuffer(m_VertexBufferData);
}

uint32_t RenderBlockBuildingJC3::GetTypeHash() const
{
    return RenderBlockFactory::RB_BUILDINGJC3;
}

void RenderBlockBuildingJC3::Setup(RenderContext_t* context)
{
    if (!m_Visible) {
        return;
    }

    IRenderBlock::Setup(context);

    // set the constant buffers
    // context->m_Renderer->SetVertexShaderConstants(m_ConstantBuffer, 2, m_Constants);
    // context->m_Renderer->SetPixelShaderConstants(m_ConstantBuffer, 2, m_Constants);

    // context->m_Renderer->SetCullMode((!(m_Block.attributes.flags & 1)) ? D3D11_CULL_BACK : D3D11_CULL_NONE);

    // set the 2nd vertex buffers
    context->m_Renderer->SetVertexStream(m_VertexBufferData, 1);
}
}; // namespace jc3
