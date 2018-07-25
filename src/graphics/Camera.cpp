#include <graphics/Camera.h>
#include <graphics/Renderer.h>
#include <Window.h>
#include <Input.h>

static constexpr auto g_MouseSensitivity = 0.0025f;
static constexpr auto g_MovementSensitivity = 0.05f;
static constexpr auto g_MouseScrollSensitivity = 0.01f;

static auto SpeedMultiplier = [](float value) {
    const auto& input = Input::Get();

    return value * (input->IsKeyPressed(VK_SHIFT) ? 0.05f : input->IsKeyPressed(VK_CONTROL) ? 5.0f : 1.0f);
};

Camera::Camera()
{
    const auto window = Window::Get();
    const auto window_size = window->GetSize();

    m_Projection = glm::perspectiveFovLH(glm::radians(m_FOV), window_size.x, window_size.y, m_NearClip, m_FarClip);
    m_Viewport = glm::vec4{ 0, 0, window_size.x, window_size.y };

    m_Position = glm::vec3(0, 3, -10);
    m_Rotation = glm::vec3(0, 0, 0);

    // rebuild projection matrix if the window is resized
    window->Events().WindowResized.connect([&](const glm::vec2& size) {
        if (size.x != 0 && size.y != 0) {
            m_Projection = glm::perspectiveFovLH(glm::radians(m_FOV), size.x, size.y, m_NearClip, m_FarClip);
            m_Viewport = glm::vec4{ 0, 0, size.x, size.y };
        }
    });

    auto& events = Input::Get()->Events();
    events.MousePress.connect([&](uint32_t message, const glm::vec2& position) {
        bool capture_mouse = false;

        switch (message) {
            // left button down
        case WM_LBUTTONDOWN:
            m_IsRotatingView = true;
            capture_mouse = true;
            break;

            // left button up
        case WM_LBUTTONUP:
            m_IsRotatingView = false;
            break;

            // right button down
        case WM_RBUTTONDOWN:
            m_IsTranslatingView = true;
            capture_mouse = true;
            break;

        case WM_RBUTTONUP:
            m_IsTranslatingView = false;
            break;
        }

        // capture the mouse to stop it escaping the window
        Window::Get()->CaptureMouse(capture_mouse);

        return true;
    });

    events.MouseMove.connect([&](const glm::vec2& position) {
        // handle view translation
        if (m_IsTranslatingView) {
            m_Position.x += position.x * SpeedMultiplier(g_MouseSensitivity);
            m_Position.y -= position.y * SpeedMultiplier(g_MouseSensitivity);
        }

        // handle view rotation
        if (m_IsRotatingView) {
            m_Rotation.z += position.x * SpeedMultiplier(g_MouseSensitivity);
            m_Rotation.y += position.y * SpeedMultiplier(g_MouseSensitivity);
        }

        return true;
    });

    events.MouseScroll.connect([&](float delta) {
        m_Position.z += (delta * SpeedMultiplier(g_MouseScrollSensitivity));
    });

    events.KeyDown.connect([&](uint32_t key) {
        if (key == VK_RIGHT) {
            m_Position.x += SpeedMultiplier(g_MovementSensitivity);
        }
        else if (key == VK_LEFT) {
            m_Position.x -= SpeedMultiplier(g_MovementSensitivity);
        }
        else if (key == VK_UP) {
            m_Position.z += SpeedMultiplier(g_MovementSensitivity);
        }
        else if (key == VK_DOWN) {
            m_Position.z -= SpeedMultiplier(g_MovementSensitivity);
        }
        else if (key == VK_PRIOR) {
            m_Position.y += SpeedMultiplier(g_MovementSensitivity);
        }
        else if (key == VK_NEXT) {
            m_Position.y -= SpeedMultiplier(g_MovementSensitivity);
        }

        return true;
    });
}

Camera::~Camera()
{
    Shutdown();
}

void Camera::Shutdown()
{
}

void Camera::Update(RenderContext_t* context)
{
    // calculate the view matrix
    {
        m_View = glm::translate(glm::mat4(1.0f), -m_Position);
        m_View = glm::rotate(m_View, m_Rotation.x, { 0, 0, 1 });
        m_View = glm::rotate(m_View, m_Rotation.y, { 1, 0, 0 });
        m_View = glm::rotate(m_View, m_Rotation.z, { 0, 1, 0 });
    }

    // update view projection
    m_ViewProjection = (m_Projection * m_View);

    // update the render context
    context->m_viewProjectionMatrix = m_ViewProjection;
}

void Camera::ResetToDefault()
{
    m_Position = glm::vec3(0, 0, -10);
    m_Rotation = glm::vec3(0);
}

void Camera::WorldToScreen(const glm::vec3& world, glm::vec3* screen)
{
    *screen = glm::project(world, m_View, m_Projection, m_Viewport);

    // glm uses the bottom of the window, so we need to take that into account
    const auto window_size = Window::Get()->GetSize();
    screen->y = (window_size.y - screen->y);
}

void Camera::ScreenToWorld(const glm::vec3& screen, glm::vec3* world)
{
    // glm uses the bottom of the window, so we need to take that into account
    const auto window_size = Window::Get()->GetSize();
    *world = glm::unProject({ screen.x, window_size.y - screen.y, screen.z }, m_View, m_Projection, m_Viewport);
}

void Camera::FocusOnBoundingBox(const glm::vec3& bb_min, const glm::vec3& bb_max)
{
    const auto bb_width = (bb_max.x - bb_min.x);
    const auto bb_height = (bb_max.y - bb_min.y);
    const auto bb_length = (bb_max.z - bb_min.z);

    // calculate how far we need to translate to get the model in view
    const float distance = ((glm::max(glm::max(bb_width, bb_height), bb_length) / 2) / glm::sin(glm::radians(m_FOV) / 2));

    m_Position = glm::vec3{ 0, (bb_height / 2), -(distance + 0.5f) };
    m_Rotation = glm::vec3(0);
}