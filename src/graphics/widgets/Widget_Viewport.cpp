#include "Widget_Viewport.h"

#include <Window.h>
#include <graphics/Camera.h>
#include <graphics/Renderer.h>
#include <graphics/UI.h>

#include <jc3/FileLoader.h>
#include <jc3/formats/RenderBlockModel.h>

#include <Input.h>

Widget_Viewport::Widget_Viewport()
{
    m_Title = "Viewport";
}

bool Widget_Viewport::Begin()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin(m_Title.c_str(), nullptr, m_Flags);
    ImGui::PopStyleVar();
    return true;
}

void Widget_Viewport::Render(RenderContext_t* context)
{
    auto width  = static_cast<int32_t>(ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x);
    auto height = static_cast<int32_t>(ImGui::GetWindowContentRegionMax().y - ImGui::GetWindowContentRegionMin().y);

    width -= (width % 2 != 0) ? 1 : 0;
    height -= (height % 2 != 0) ? 1 : 0;

    const auto texture = context->m_Renderer->GetGBufferSRV(m_RenderTarget);
    if (texture) {
        ImGui::Image(texture, ImVec2((float)width, (float)height));
    }

    // update camera projection
    Camera::Get()->UpdateViewport({width, height});

    // TODO:
    //  - context menu, allow saving of the current gbuffer srv to a file (so you can quickly greenscreen things)
    //  - handle all user input, move most of the Input.cpp stuff here for camera rotating
    //  - update drag drop so we can drop RBM things into this window and then they will be loaded

    // drag drop target
    if (const auto payload = UI::Get()->GetDropPayload(DROPPAYLOAD_UNKNOWN)) {
        FileLoader::Get()->ReadFileFromDisk(payload->data);
    }

    // handle user input
    HandleInput(context->m_DeltaTime);

    // TEMP TEMP TEMP
    UI::Get()->SceneDrawList = ImGui::GetWindowDrawList();

    // gbuffer render target selection
    {
        static const char* gbuffer_items[] = {"Diffuse", "Normal", "Properties", "PropertiesEx"};

        ImGui::SetCursorPos(ImVec2(5, 25));

        ImGui::PushItemWidth(110);
        ImGui::Combo("##render_target", &m_RenderTarget, gbuffer_items, IM_ARRAYSIZE(gbuffer_items));
        ImGui::PopItemWidth();
    }
}

void Widget_Viewport::HandleInput(const float delta_time)
{
    const auto& io        = ImGui::GetIO();
    const auto& mouse_pos = glm::vec2(io.MousePos.x, io.MousePos.y);

    // FIXME: make sure the initial click was inside this window!

    // handle mouse press
    if (ImGui::IsMouseDown(0)) {
        if (!m_MouseState[0] && ImGui::IsItemHovered()) {
            m_MouseState[0] = true;
            Input::Get()->Events().MousePress(WM_LBUTTONDOWN, mouse_pos);
        }
        // handle mouse move
        else if (m_MouseState[0]) {
            Input::Get()->Events().MouseMove({-io.MouseDelta.x, -io.MouseDelta.y});
        }
    }
    // handle mouse release
    else if (m_MouseState[0]) {
        m_MouseState[0] = false;
        Input::Get()->Events().MousePress(WM_LBUTTONUP, mouse_pos);
    }

#if 0
    const auto render_block = Camera::Get()->Pick({io.MousePos.x, io.MousePos.y});
    if (render_block) {
        ImGui::SetTooltip(render_block->GetFileName().c_str());
    }
#endif
}
