#include <graphics/DebugRenderer.h>
#include <graphics/Renderer.h>
#include <graphics/ShaderManager.h>
#include <jc3/Types.h>

#include <graphics/Camera.h>
#include <Window.h>
#include <Input.h>

static ImVec2 worldToPos(const glm::vec3& worldPos)
{
    glm::vec3 screen;
    Camera::Get()->WorldToScreen(worldPos, &screen);

    return { screen.x, screen.y };
}

void DebugRenderer::Begin(RenderContext_t* context)
{
    ImGuiIO& io = ImGui::GetIO();

    const ImU32 flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::SetNextWindowSize(io.DisplaySize);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, 0);
    ImGui::Begin("Debug Renderer", NULL, flags);

    m_DrawList = ImGui::GetWindowDrawList();

    ImGui::End();
    ImGui::PopStyleColor();
}

void DebugRenderer::DrawLine(const glm::vec3& from, const glm::vec3& to, const glm::vec4& colour)
{
    m_DrawList->AddLine(worldToPos(from), worldToPos(to), ImColor(colour.x, colour.y, colour.z, colour.a));
}

void DebugRenderer::DrawBBox(const glm::vec3& min, const glm::vec3& max, const glm::vec4& colour)
{
    glm::vec3 bb[2] = { min, max };
    ImVec2 points[8];

    // calculate all the points
    for (int i = 0; i < 8; i++)
    {
        points[i] = worldToPos({ bb[(i ^ (i >> 1)) & 1].x, bb[(i >> 1) & 1].y, bb[(i >> 2) & 1].z });
    }

    const auto im_color = ImColor(colour.x, colour.y, colour.z, colour.a);

    // draw the lines
    for (int i = 0; i < 4; ++i)
    {
        m_DrawList->AddLine(points[i], points[(i + 1) & 3], im_color);
        m_DrawList->AddLine(points[4 + i], points[4 + ((i + 1) & 3)], im_color);
        m_DrawList->AddLine(points[i], points[4 + i], im_color);
    }
}

void DebugRenderer::DrawText(const std::string& text, const glm::vec3& position, const glm::vec4& colour, bool center)
{
    if (center) {
        const auto text_size = ImGui::CalcTextSize(text.c_str());

        auto screen_pos = worldToPos(position);
        screen_pos.x -= (text_size.x / 2);
        screen_pos.y -= (text_size.y / 2);

        m_DrawList->AddText(screen_pos, ImColor(colour.x, colour.y, colour.z, colour.a), text.c_str());
    }
    else {
        m_DrawList->AddText(worldToPos(position), ImColor(colour.x, colour.y, colour.z, colour.a), text.c_str());
    }
}

void DebugRenderer::DrawText(const std::stringstream& text, const glm::vec3& position, const glm::vec4& colour, bool center)
{
    DrawText(text.str(), position, colour, center);
}