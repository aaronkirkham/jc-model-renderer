#include <graphics/Camera.h>
#include <graphics/Renderer.h>
#include <Window.h>
#include <Input.h>

constexpr float g_MouseSensitivity = 0.0025f;
constexpr float g_MovementSensitivity = 0.05f;

Camera::Camera()
{
    auto window = Window::Get();
    auto windowSize = window->GetSize();

    m_Projection = glm::perspectiveFovLH(glm::radians(m_FOV), windowSize.x, windowSize.y, m_NearClip, m_FarClip);

    // create the frame constant buffers
    FrameConstants constants;
    m_FrameConstants = Renderer::Get()->CreateConstantBuffer(constants);

    m_Position = glm::vec3(0, 3, -10);
    m_Rotation = glm::vec3(0, 0, 0);

    // rebuild projection matrix if the window is resized
    window->Events().WindowResized.connect([&](const glm::vec2& size) {
        if (size.x != 0 && size.y != 0) {
            m_Projection = glm::perspectiveFovLH(glm::radians(m_FOV), size.x, size.y, m_NearClip, m_FarClip);
        }
    });

    auto& events = Input::Get()->Events();
    events.MouseDown.connect([&](const glm::vec2& position) {
        /*m_IsRotatingView = true;
        Window::Get()->SetClipEnabled(true);
        Input::Get()->ResetCursor();*/
        return true;
    });

    events.MouseUp.connect([&](const glm::vec2& position) {
        /*m_IsRotatingView = false;
        Window::Get()->SetClipEnabled(false);*/
        return true;
    });

    events.MouseMove.connect([&](const glm::vec2& position) {
        if (m_IsRotatingView) {
            m_Rotation.z -= position.x * g_MouseSensitivity;
            m_Rotation.y -= position.y * g_MouseSensitivity;
        }

        return true;
    });

    events.KeyDown.connect([&](uint32_t key) {
        if (key == VK_RIGHT) {
            m_Position.x += g_MovementSensitivity;
        }
        else if (key == VK_LEFT) {
            m_Position.x -= g_MovementSensitivity;
        }
        else if (key == VK_UP) {
            m_Position.z += g_MovementSensitivity;
        }
        else if (key == VK_DOWN) {
            m_Position.z -= g_MovementSensitivity;
        }
        else if (key == VK_PRIOR) {
            m_Position.y += g_MovementSensitivity;
        }
        else if (key == VK_NEXT) {
            m_Position.y -= g_MovementSensitivity;
        }

        return true;
    });
}

Camera::~Camera()
{
    Renderer::Get()->DestroyBuffer(m_FrameConstants);
}

void Camera::Update()
{
    // calculate the view matrix
    {
        auto roll = glm::rotate({}, m_Rotation.x, glm::vec3{ 0.0f, 0.0f, 1.0f });
        auto pitch = glm::rotate({}, m_Rotation.y, glm::vec3{ 1.0f, 0.0f, 0.0f });
        auto yaw = glm::rotate({}, m_Rotation.z, glm::vec3{ 0.0f, 1.0f, 0.0f });

        m_View = (roll * pitch * yaw) * glm::translate({}, -m_Position);
    }

    // update frame constants
    {
        FrameConstants constants;
        constants.viewProjection = (m_Projection * m_View);

        Renderer::Get()->SetVertexShaderConstants(m_FrameConstants, 0, constants);
    }
}

bool Camera::WorldToScreen(const glm::vec3& world, glm::vec3* screen)
{
    auto viewProjection = (m_Projection * m_View);
    float z = viewProjection[0][3] * world.x + viewProjection[1][3] * world.y + viewProjection[2][3] * world.z + viewProjection[3][3];

    if (z < 0.1f) {
        *screen = glm::vec3{ 0, 0, z };
        return false;
    }

    float x = viewProjection[0][0] * world.x + viewProjection[1][0] * world.y + viewProjection[2][0] * world.z + viewProjection[3][0];
    float y = viewProjection[0][1] * world.x + viewProjection[1][1] * world.y + viewProjection[2][1] * world.z + viewProjection[3][1];

    auto size = Window::Get()->GetSize();

    float halfWidth = (size.x * 0.5f);
    float halfHeight = (size.y * 0.5f);

    *screen = glm::vec3{ (halfWidth + halfWidth * x / z), (halfHeight - halfHeight * y / z), z };
    return true;
}