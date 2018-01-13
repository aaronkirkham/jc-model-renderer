#pragma once

#include <StdInc.h>
#include <singleton.h>
#include <graphics/Types.h>

struct ImDrawList;
class DebugRenderer : public Singleton<DebugRenderer>
{
private:
    ImDrawList* m_DrawList = nullptr;

public:
    DebugRenderer() = default;
    virtual ~DebugRenderer() = default;

    void Begin(RenderContext_t* context);

    void DrawLine(const glm::vec3& from, const glm::vec3& to, const glm::vec4& colour);
    void DrawBBox(const glm::vec3& min, const glm::vec3& max, const glm::vec4& colour);

#ifdef DrawText
#undef DrawText
#endif

    void DrawText(const std::string& text, const glm::vec3& position, const glm::vec4& colour, bool center = false);
    void DrawText(const std::stringstream& text, const glm::vec3& position, const glm::vec4& colour, bool center = false);
};