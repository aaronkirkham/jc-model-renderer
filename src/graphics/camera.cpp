#include "../jc3/formats/render_block_model.h"
#include "../window.h"
#include "camera.h"
#include "types.h"

#include <imgui.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

static constexpr auto g_MouseSensitivity       = 0.0025f;
static constexpr auto g_MovementSensitivity    = 0.05f;
static constexpr auto g_MouseScrollSensitivity = 0.1f;

static auto SpeedMultiplier = [](float value) {
    const auto& io = ImGui::GetIO();

    if (io.KeyShift) {
        return value * 0.1f;
    }

    if (io.KeyCtrl) {
        return value * 5.0f;
    }

    return value * 1.0f;
};

Camera::Camera()
{
    const auto  window      = Window::Get();
    const auto& window_size = window->GetSize();

    m_Position   = glm::vec3(0, 3, -10);
    m_Rotation   = glm::vec3(0, 0, 0);
    m_Projection = glm::perspectiveFovLH(glm::radians(m_FOV), window_size.x, window_size.y, m_NearClip, m_FarClip);
    m_Viewport   = window_size;

    // handle window losing focus
    window->Events().FocusLost.connect([&] {
        if (m_IsTranslatingView || m_IsRotatingView) {
            Window::Get()->CaptureMouse(false);
        }

        m_IsTranslatingView = false;
        m_IsRotatingView    = false;
    });
}

void Camera::Update(RenderContext_t* context)
{
    // calculate the view matrix
    m_View = glm::translate(glm::mat4(1), -m_Position);
    m_View = glm::rotate(m_View, m_Rotation.x, {0, 0, 1});
    m_View = glm::rotate(m_View, m_Rotation.y, {1, 0, 0});
    m_View = glm::rotate(m_View, m_Rotation.z, {0, 1, 0});

    // update view projection
    m_ViewProjection = (m_Projection * m_View);

    // update the render context
    context->m_viewProjectionMatrix = m_ViewProjection;
}

void Camera::UpdateViewport(const glm::vec2& size)
{
    if (size.x > 0 && size.y > 0 && m_Viewport != size) {
        m_Projection = glm::perspectiveFovLH(glm::radians(m_FOV), size.x, size.y, m_NearClip, m_FarClip);
        m_Viewport   = size;
    }
}

glm::vec2 Camera::WorldToScreen(const glm::vec3& world)
{
    const auto& screen = glm::project(world, m_View, m_Projection, glm::vec4(0, 0, m_Viewport));
    return {screen.x, (m_Viewport.y - screen.y)};
}

glm::vec3 Camera::ScreenToWorld(const glm::vec2& screen)
{
    const auto& world =
        glm::unProject({screen.x, (m_Viewport.y - screen.y), 1}, m_View, m_Projection, glm::vec4(0, 0, m_Viewport));
    return world;
}

void Camera::FocusOn(RenderBlockModel* model)
{
    // TODO: make better, doesn't work well in every situation
    // and some times doesn't zoom in far enough!

    // We should look at the current camera rotation and zoom to the
    // side of the bounding box we are facing.

    assert(model);

    // calculate how far we need to translate to get the model in view
    const auto& dimensions = model->GetBoundingBox()->GetSize();
    const float distance =
        ((glm::max(glm::max(dimensions.x, dimensions.y), dimensions.z) / 2) / glm::sin(glm::radians(m_FOV) / 2));

    m_Position = glm::vec3{0, (dimensions.y / 2), -(distance + 0.5f)};
    m_Rotation = glm::vec3(0);
}

std::shared_ptr<RenderBlockModel> Camera::Pick(const glm::vec2& mouse)
{
    Ray                                                ray(m_Position, ScreenToWorld(mouse));
    std::map<float, std::shared_ptr<RenderBlockModel>> hits;

    for (const auto& Instance : RenderBlockModel::Instances) {
        const auto render_block = Instance.second;

        const auto dist = ray.Hit(*render_block->GetBoundingBox());
        if (dist == 0.0f || dist == INFINITY) {
            continue;
        }

        hits[dist] = std::move(render_block);
    }

    return !hits.empty() ? hits.begin()->second : nullptr;
}

void Camera::OnMousePress(int32_t button, bool is_button_down, const glm::vec2& position)
{
    // left button
    if (button == 0) {
        m_IsRotatingView = is_button_down;
    }
    // right button
    else if (button == 1) {
        m_IsTranslatingView = is_button_down;
    }

    // capture the mouse to stop it escaping the window
    Window::Get()->CaptureMouse(is_button_down);
}

void Camera::OnMouseMove(const glm::vec2& delta)
{
    // handle view translation
    if (m_IsTranslatingView) {
        m_Position.x += delta.x * SpeedMultiplier(g_MouseSensitivity);
        m_Position.y -= delta.y * SpeedMultiplier(g_MouseSensitivity);
    }

    // handle view rotation
    if (m_IsRotatingView) {
        m_Rotation.z += delta.x * SpeedMultiplier(g_MouseSensitivity);
        m_Rotation.y += delta.y * SpeedMultiplier(g_MouseSensitivity);
    }
}

void Camera::OnMouseScroll(const float delta)
{
    m_Position.z += (delta * SpeedMultiplier(g_MouseScrollSensitivity));
}
