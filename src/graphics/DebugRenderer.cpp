#include <graphics/DebugRenderer.h>
#include <graphics/Renderer.h>
#include <graphics/ShaderManager.h>
#include <jc3/Types.h>

struct Vertex3D
{
    glm::vec3 pos;
    glm::vec4 colour;
};

DebugRenderer::DebugRenderer()
{
    m_VertexBuffer = Renderer::Get()->CreateVertexBuffer(30, sizeof(Vertex3D));
    m_VertexShader = GET_VERTEX_SHADER(debug_renderer);
    m_PixelShader = GET_PIXEL_SHADER(debug_renderer);

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
}

void DebugRenderer::DrawLine(const glm::vec3& from, const glm::vec3& to, const glm::vec4& colour)
{
    auto context = Renderer::Get()->GetDeviceContext();

    Vertex3D vertices[] = {
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