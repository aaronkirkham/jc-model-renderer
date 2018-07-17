#include <graphics/Camera.h>
#include <graphics/Renderer.h>
#include <Window.h>
#include <Input.h>

static constexpr auto g_MouseSensitivity = 0.0025f;
static constexpr auto g_MovementSensitivity = 0.05f;
static constexpr auto g_MouseScrollSensitivity = 0.01f;

static auto SpeedMultiplier = [](float value) {
    return value * (Input::Get()->IsKeyPressed(VK_SHIFT) ? 0.05f : 1.0f);
};

Camera::Camera()
{
    const auto window = Window::Get();
    const auto window_size = window->GetSize();

    m_Projection = glm::perspectiveFovLH(glm::radians(m_FOV), window_size.x, window_size.y, m_NearClip, m_FarClip);
    m_Viewport = glm::vec4{ 0, 0, window_size.x, window_size.y };

    // create the frame constant buffers
    FrameConstants constants;
    m_FrameConstants = Renderer::Get()->CreateConstantBuffer(constants, "Camera Frame Buffer");

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
    events.MouseDown.connect([&](const glm::vec2& position) {
        m_IsRotatingView = true;
        return true;
    });

    events.MouseUp.connect([&](const glm::vec2& position) {
        m_IsRotatingView = false;
        return true;
    });

    events.MouseMove.connect([&](const glm::vec2& position) {
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
    Renderer::Get()->DestroyBuffer(m_FrameConstants);
}

void Camera::Update()
{
    // calculate the view matrix
    {
        m_View = glm::translate(glm::mat4(1.0f), -m_Position);
        m_View = glm::rotate(m_View, m_Rotation.x, glm::vec3{ 0.0f, 0.0f, 1.0f });
        m_View = glm::rotate(m_View, m_Rotation.y, glm::vec3{ 1.0f, 0.0f, 0.0f });
        m_View = glm::rotate(m_View, m_Rotation.z, glm::vec3{ 0.0f, 1.0f, 0.0f });
    }

    // update frame constants
    {
        FrameConstants constants;
        constants.viewProjection = (m_Projection * m_View);

        Renderer::Get()->SetVertexShaderConstants(m_FrameConstants, 0, constants);
    }
}

void Camera::ResetToDefault()
{
    m_Position = glm::vec3(0, 3, -10);
    m_Rotation = glm::vec3(0, 0, 0);
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