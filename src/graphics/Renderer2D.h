#pragma once

#include <StdInc.h>
#include <singleton.h>
#include <graphics/Types.h>

class Renderer2D : public Singleton<Renderer2D>
{
private:
    VertexBuffer_t* m_VertexBuffer = nullptr;
    VertexDeclaration_t* m_VertexDeclaration = nullptr;
    std::shared_ptr<VertexShader_t> m_VertexShader = nullptr;
    std::shared_ptr<PixelShader_t> m_PixelShader = nullptr;

public:
    Renderer2D();
    virtual ~Renderer2D();

    void Render(RenderContext_t* context);

    void DrawLine(const glm::vec2& from, const glm::vec2& to, const glm::vec4& colour);
    void DrawLine(const glm::vec2& from, const glm::vec2& to, const glm::vec4& from_colour, const glm::vec4& to_colour);

    void DrawRect(const glm::vec2& topleft, const glm::vec2& bottomright, const glm::vec4& colour);

    void DrawFilledRect(const glm::vec2& topleft, const glm::vec2& bottomright, const glm::vec4& colour);
};