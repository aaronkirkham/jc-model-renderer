#include <Input.h>
#include <Window.h>
#include <graphics/Camera.h>
#include <graphics/Renderer.h>

#include <windowsx.h>

#include <graphics/DebugRenderer.h>

void Input::Initialise()
{
    // reset all keys if we lose window focus
    Window::Get()->Events().FocusLost.connect(
        [&] { std::fill(m_KeyboardState.begin(), m_KeyboardState.end(), false); });
}

bool Input::HandleMessage(MSG* event)
{
    auto&      imgIO   = ImGui::GetIO();
    const auto message = static_cast<uint32_t>(event->message);

    // don't capture mouse input if imgui wants it
    if (imgIO.WantCaptureMouse && (message == WM_LBUTTONUP || message == WM_LBUTTONDOWN || message == WM_MOUSEWHEEL)) {
        return false;
    }

    // don't capture mouse moves if imgui wants it
    if (imgIO.WantSetMousePos && message == WM_MOUSEMOVE) {
        return false;
    }

    // don't capture keyboard input if imgui wants it
    if (imgIO.WantCaptureKeyboard && (message == WM_KEYUP || message == WM_KEYDOWN)) {
        return false;
    }

    const auto key       = static_cast<uint32_t>(event->wParam);
    const auto modifiers = static_cast<uint32_t>(event->lParam);

    if (message == WM_KEYDOWN) {
        m_KeyboardState[key] = true;
        m_InputEvents.KeyDown(key);
    } else if (message == WM_KEYUP) {
        /*if (key == VK_ESCAPE) {
            TerminateProcess(GetCurrentProcess(), 0);
        }*/

        m_KeyboardState[key] = false;
        m_InputEvents.KeyUp(key);
    } else if (message == WM_LBUTTONUP || message == WM_LBUTTONDOWN || message == WM_RBUTTONUP
               || message == WM_RBUTTONDOWN) {
        auto position =
            glm::vec2{static_cast<float>(GET_X_LPARAM(event->lParam)), static_cast<float>(GET_Y_LPARAM(event->lParam))};
        m_InputEvents.MousePress(message, position);

        m_LastClickPosition = position;
        Camera::Get()->MouseToWorld(position, &m_LastClickWorldPosition);
    } else if (message == WM_MOUSEMOVE) {
        auto position =
            glm::vec2{static_cast<float>(GET_X_LPARAM(event->lParam)), static_cast<float>(GET_Y_LPARAM(event->lParam))};
        m_InputEvents.MouseMove((m_MousePosition - position));

        m_MousePosition = position;
        Camera::Get()->MouseToWorld(position, &m_MouseWorldPosition);
    } else if (message == WM_MOUSEWHEEL) {
        auto delta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(event->wParam));
        m_InputEvents.MouseScroll(delta);
    }

    return true;
}
