#include <graphics/DebugRenderer.h>
#include <graphics/Renderer.h>
#include <graphics/ShaderManager.h>
#include <jc3/Types.h>

struct Vertex2D
{
    glm::vec3 pos;
    glm::vec4 colour;
};

DebugRenderer::DebugRenderer()
{
    m_VertexBuffer = Renderer::Get()->CreateVertexBuffer(30, sizeof(Vertex2D));
    m_VertexShader = ShaderManager::Get()->GetVertexShader("default2d");
    m_PixelShader = ShaderManager::Get()->GetPixelShader("default2d");

    // TODO: compress colour
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    m_VertexDeclaration = Renderer::Get()->CreateVertexDeclaration(layout, 2, m_VertexShader.get());
}

DebugRenderer::~DebugRenderer()
{
    Renderer::Get()->DestroyVertexDeclaration(m_VertexDeclaration);
    Renderer::Get()->DestroyBuffer(m_VertexBuffer);
}

void DebugRenderer::Render(RenderContext_t* context)
{
    // DrawLine({ 200, 200 }, { 300, 300 }, { 1, 1, 1, 1 });
}

#if 0
void DebugRenderer::DrawLine(const glm::vec2& from, const glm::vec2& to, const glm::vec4& colour)
{
    DrawLine(from, to, colour, colour);
}

void DebugRenderer::DrawLine(const glm::vec2& from, const glm::vec2& to, const glm::vec4& from_colour, const glm::vec4& to_colour)
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

void DebugRenderer::DrawRect(const glm::vec2& topleft, const glm::vec2& bottomright, const glm::vec4& colour)
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

void DebugRenderer::DrawFilledRect(const glm::vec2& topleft, const glm::vec2& bottomright, const glm::vec4& colour)
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
#endif

void DebugRenderer::DrawLine(const glm::vec3& from, const glm::vec3& to, const glm::vec4& colour)
{
    auto context = Renderer::Get()->GetDeviceContext();

    Vertex2D vertices[] = {
        { from, colour },
        { to, colour },
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

void DebugRenderer::DrawBBox(const glm::vec3& min, const glm::vec3& max, const glm::vec4& colour)
{
    glm::vec3 bb[2] = { min, max };
    glm::vec3 points[8];

    // calculate all the points
    for (int i = 0; i < 8; i++)
    {
        points[i].x = bb[(i ^ (i >> 1)) & 1].x;
        points[i].y = bb[(i >> 1) & 1].y;
        points[i].z = bb[(i >> 2) & 1].z;
    }

    // draw the lines
    for (int i = 0; i < 4; ++i)
    {
        DrawLine(points[i], points[(i + 1) & 3], colour);
        DrawLine(points[4 + i], points[4 + ((i + 1) & 3)], colour);
        DrawLine(points[i], points[4 + i], colour);
    }
}