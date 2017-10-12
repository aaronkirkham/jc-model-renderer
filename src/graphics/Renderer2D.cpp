#include <graphics/Renderer2D.h>
#include <graphics/Renderer.h>
#include <engine/Types.h>
#include <graphics/ShaderManager.h>

struct Vertex2D
{
    int16_t x;
    int16_t y;
    glm::vec4 colour;
};

Renderer2D::Renderer2D()
{
    m_VertexBuffer = Renderer::Get()->CreateVertexBuffer(30, sizeof(Vertex2D));
    m_VertexShader = ShaderManager::Get()->GetVertexShader("default2d_vs.vs");
    m_PixelShader = ShaderManager::Get()->GetPixelShader("default2d_ps.ps");

    // TODO: compress colour
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R16G16_SINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(layout, 2, m_VertexShader.get());
}

Renderer2D::~Renderer2D()
{
    Renderer::Get()->DestroyVertexDeclaration(m_VertexDeclaration);
    Renderer::Get()->DestroyBuffer(m_VertexBuffer);
}

void Renderer2D::Render(RenderContext_t* context)
{
}

void Renderer2D::DrawLine(const glm::vec2& from, const glm::vec2& to, const glm::vec4& colour)
{
    DrawLine(from, to, colour, colour);
}

void Renderer2D::DrawLine(const glm::vec2& from, const glm::vec2& to, const glm::vec4& from_colour, const glm::vec4& to_colour)
{
    auto context = Renderer::Get()->GetDeviceContext();

    uint32_t num = 1;
    D3D11_VIEWPORT vp;
    context->RSGetViewports(&num, &vp);

    auto x0 = JustCause3::Vertex::pack<int16_t>(2.0f * (from.x - 0.5f) / vp.Width - 1.0f);
    auto y0 = JustCause3::Vertex::pack<int16_t>(1.0f - 2.0f * (from.y - 0.5f) / vp.Height);
    auto x1 = JustCause3::Vertex::pack<int16_t>(2.0f * (to.x - 0.5f) / vp.Width - 1.0f);
    auto y1 = JustCause3::Vertex::pack<int16_t>(1.0f - 2.0f * (to.y - 0.5f) / vp.Height);

    Vertex2D vertices[] = {
        { x0, y0, from_colour },
        { x1, y1, to_colour },
    };

    Renderer::Get()->MapBuffer(m_VertexBuffer->m_Buffer, vertices);

    context->IASetInputLayout(m_VertexDeclaration->m_Layout);
    context->VSSetShader(m_VertexShader->m_Shader, nullptr, 0);
    context->PSSetShader(m_PixelShader->m_Shader, nullptr, 0);

    uint32_t offset = 0;
    context->IASetVertexBuffers(0, 1, &m_VertexBuffer->m_Buffer, &m_VertexBuffer->m_ElementStride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

    context->Draw(2, 0);
}

void Renderer2D::DrawRect(const glm::vec2& topleft, const glm::vec2& bottomright, const glm::vec4& colour)
{
    auto to = topleft + bottomright;
    auto topright = (topleft + bottomright.x);
    auto bottomleft = glm::vec2{ topleft.x, topleft.y + bottomright.y };

    // diameter
    //DrawLine({ topleft.x, topleft.y }, { to.x, to.y }, { 1, 0, 0, 1 });

    // top left to top right
    DrawLine({ topleft.x, topleft.y }, { topright.x, topleft.y }, colour);

    // top left to bottom left
    DrawLine({ topleft.x, topleft.y }, { topleft.x, bottomleft.y }, colour);

    // top right to bottom right
    DrawLine({ topright.x, topleft.y }, { to.x, to.y }, colour);

    // bottom left to bottom right
    DrawLine({ bottomleft.x, bottomleft.y }, { to.x, to.y }, colour);
}

void Renderer2D::DrawFilledRect(const glm::vec2& topleft, const glm::vec2& bottomright, const glm::vec4& colour)
{
    auto context = Renderer::Get()->GetDeviceContext();

    uint32_t num = 1;
    D3D11_VIEWPORT vp;
    context->RSGetViewports(&num, &vp);

    auto to = topleft + bottomright;

    auto x0 = JustCause3::Vertex::pack<int16_t>(2.0f * (topleft.x - 0.5f) / vp.Width - 1.0f);
    auto y0 = JustCause3::Vertex::pack<int16_t>(1.0f - 2.0f * (topleft.y - 0.5f) / vp.Height);
    auto x1 = JustCause3::Vertex::pack<int16_t>(2.0f * (to.x - 0.5f) / vp.Width - 1.0f);
    auto y1 = JustCause3::Vertex::pack<int16_t>(1.0f - 2.0f * (to.y - 0.5f) / vp.Height);

    Vertex2D vertices[] = {
        { x0, y0, colour },
        { x1, y0, colour },
        { x0, y1, colour },
        { x1, y1, colour },
    };

    Renderer::Get()->MapBuffer(m_VertexBuffer->m_Buffer, vertices);

    context->IASetInputLayout(m_VertexDeclaration->m_Layout);
    context->VSSetShader(m_VertexShader->m_Shader, nullptr, 0);
    context->PSSetShader(m_PixelShader->m_Shader, nullptr, 0);

    uint32_t offset = 0;
    context->IASetVertexBuffers(0, 1, &m_VertexBuffer->m_Buffer, &m_VertexBuffer->m_ElementStride, &offset);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    context->Draw(4, 0);
}