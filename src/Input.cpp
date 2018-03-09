#include <Input.h>
#include <Window.h>
#include <graphics/Renderer.h>
#include <graphics/Camera.h>

#include <windowsx.h>

#include <graphics/DebugRenderer.h>

Input::~Input()
{
}

void Input::Initialise()
{
#if 0
    Renderer::Get()->Events().RenderFrame.connect([&](RenderContext_t* context) {
        glm::vec3 window_center = glm::vec3(Window::Get()->GetCenterPoint(), 1);
        glm::vec3 window_center_world;
        Camera::Get()->ScreenToWorld(window_center, &window_center_world);

        DebugRenderer::Get()->DrawLine(window_center_world, m_MouseWorldPosition, { 1, 0, 0, 1 });
    });
#endif
}

bool Input::HandleMessage(MSG* event)
{
    auto& imgIO = ImGui::GetIO();

    // don't capture mouse input if imgui wants it
    if (imgIO.WantCaptureMouse && (event->message == WM_LBUTTONUP || event->message == WM_LBUTTONDOWN || event->message == WM_MOUSEWHEEL)) {
        return false;
    }

    // don't capture mouse moves if imgui wants it
    if (imgIO.WantMoveMouse && event->message == WM_MOUSEMOVE) {
        return false;
    }

    // don't capture keyboard input if imgui wants it
    if (imgIO.WantCaptureKeyboard && (event->message == WM_KEYUP || event->message == WM_KEYDOWN)) {
        return false;
    }

    const auto key = static_cast<uint32_t>(event->wParam);

    if (event->message == WM_KEYDOWN)
    {
        m_KeyboardState[key] = true;
        m_InputEvents.KeyDown(key);
    }
    else if (event->message == WM_KEYUP)
    {
        /*if (key == VK_ESCAPE) {
            TerminateProcess(GetCurrentProcess(), 0);
        }*/

        m_KeyboardState[key] = false;
        m_InputEvents.KeyUp(key);
    }
    else if (event->message == WM_LBUTTONUP || event->message == WM_LBUTTONDOWN)
    {
        auto position = glm::vec2{ static_cast<float>(GET_X_LPARAM(event->lParam)), static_cast<float>(GET_Y_LPARAM(event->lParam)) };

        if (event->message == WM_LBUTTONUP) {
            m_InputEvents.MouseUp(position);

            ReleaseCapture();
            m_IsMouseDown = false;
        } 
        else {
            m_InputEvents.MouseDown(position);

            SetCapture(event->hwnd);
            m_IsMouseDown = true;
        }

        m_LastClickPosition = position;
        Camera::Get()->ScreenToWorld({ position.x, position.y, 1 }, &m_LastClickWorldPosition);
    }
    else if (event->message == WM_MOUSEMOVE)
    {
        auto position = glm::vec2{ static_cast<float>(GET_X_LPARAM(event->lParam)), static_cast<float>(GET_Y_LPARAM(event->lParam)) };
        m_InputEvents.MouseMove((m_MousePosition - position));

        m_MousePosition = position;
        Camera::Get()->ScreenToWorld(glm::vec3{ position.x, position.y, 1 }, &m_MouseWorldPosition);
    }
    else if (event->message == WM_MOUSEWHEEL)
    {
        // auto keys = GET_KEYSTATE_WPARAM(event->wParam);

        auto delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(event->wParam));
        m_InputEvents.MouseScroll(delta);
    }

    return true;
}

void Input::Update()
{
}