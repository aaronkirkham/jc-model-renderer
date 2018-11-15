#include "Widget_Viewport.h"

#include <Window.h>
#include <graphics/Camera.h>
#include <graphics/Renderer.h>
#include <graphics/UI.h>

Widget_Viewport::Widget_Viewport()
{
    m_Title = "Viewport";
}

bool Widget_Viewport::Begin()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin(m_Title.c_str());
    return true;
}

void Widget_Viewport::End()
{
    ImGui::End();
    ImGui::PopStyleVar();
}

void Widget_Viewport::Render(RenderContext_t* context)
{
    auto width  = static_cast<int32_t>(ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x);
    auto height = static_cast<int32_t>(ImGui::GetWindowContentRegionMax().y - ImGui::GetWindowContentRegionMin().y);

    width -= (width % 2 != 0) ? 1 : 0;
    height -= (height % 2 != 0) ? 1 : 0;

    const auto texture = context->m_Renderer->GetGBufferSRV(UI::Get()->GetCurrentActiveGBuffer());
    if (texture) {
        ImGui::Image(texture, ImVec2((float)width, (float)height));
    }

    Camera::Get()->UpdateWindowSize({width, height});

    // drag drop target
    if (const auto payload = UI::Get()->GetDropPayload(DROPPAYLOAD_UNKNOWN)) {
        DEBUG_LOG("Widget_Viewport DropPayload: " << payload->data);
    }
}
