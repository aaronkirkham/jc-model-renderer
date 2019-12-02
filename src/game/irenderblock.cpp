#include "irenderblock.h"

#include "graphics/renderer.h"
#include "graphics/texture.h"
#include "graphics/ui.h"

IRenderBlock::~IRenderBlock()
{
    Renderer::Get()->DestroyBuffer(m_VertexBuffer);
    Renderer::Get()->DestroyBuffer(m_IndexBuffer);
    Renderer::Get()->DestroyVertexDeclaration(m_VertexDeclaration);
    Renderer::Get()->DestroySamplerState(m_SamplerState);
}

void IRenderBlock::Setup(RenderContext_t* context)
{
    if (!m_Visible || !m_VertexShader || !m_PixelShader || !m_VertexDeclaration || !m_VertexBuffer) {
        return;
    }

    // enable the vertex and pixel shaders
    context->m_DeviceContext->VSSetShader(m_VertexShader->m_Shader, nullptr, 0);
    context->m_DeviceContext->PSSetShader(m_PixelShader->m_Shader, nullptr, 0);

    // set the input layout
    context->m_DeviceContext->IASetInputLayout(m_VertexDeclaration->m_Layout);

    // set the vertex buffer
    context->m_Renderer->SetVertexStream(m_VertexBuffer, 0);

    // set the topology
    context->m_DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void IRenderBlock::Draw(RenderContext_t* context)
{
    if (!m_Visible) {
        return;
    }

    if (m_IndexBuffer) {
        context->m_Renderer->DrawIndexed(0, m_IndexBuffer->m_ElementCount, m_IndexBuffer);
    } else if (m_VertexBuffer) {
        context->m_Renderer->Draw(0, (m_VertexBuffer->m_ElementCount / 3));
    }
}

void IRenderBlock::BindTexture(int32_t texture_index, int32_t slot, SamplerState_t* sampler)
{
    assert(texture_index < m_Textures.size());

    const auto& texture = m_Textures[texture_index];
    if (texture && texture->IsLoaded()) {
        texture->Use(slot, sampler);
    }
}

void IRenderBlock::DrawUI_Texture(const std::string& title, uint32_t texture_index)
{
    assert(texture_index < m_Textures.size());

    UI::Get()->RenderBlockTexture(this, title, m_Textures[texture_index]);
}
