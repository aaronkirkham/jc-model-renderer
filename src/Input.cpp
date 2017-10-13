#include <Input.h>
#include <Window.h>
#include <graphics/Renderer.h>

#include <windowsx.h>

Input::~Input()
{
}

void Input::Initialise()
{
}

bool Input::HandleMessage(MSG* event)
{
    auto key = static_cast<uint32_t>(event->wParam);

    if (event->message == WM_KEYDOWN)
    {
        m_KeyboardState[key] = true;
        m_InputEvents.KeyDown(key);
    }
    else if (event->message == WM_KEYUP)
    {
        if (key == VK_ESCAPE) {
            TerminateProcess(GetCurrentProcess(), 0);
        }

        m_KeyboardState[key] = false;
        m_InputEvents.KeyUp(key);
    }
    else if (event->message == WM_LBUTTONUP || event->message == WM_LBUTTONDOWN)
    {
        auto position = glm::vec2{ static_cast<float>(GET_X_LPARAM(event->lParam)), static_cast<float>(GET_Y_LPARAM(event->lParam)) };

        if (event->message == WM_LBUTTONUP) {
            m_InputEvents.MouseUp(position);

            ReleaseCapture();
        } 
        else {
            m_InputEvents.MouseDown(position);

            SetCapture(event->hwnd);
        }

        m_LastClickPosition = position;
    }
    else if (event->message == WM_MOUSEMOVE)
    {
        auto position = glm::vec2{ static_cast<float>(GET_X_LPARAM(event->lParam)), static_cast<float>(GET_Y_LPARAM(event->lParam)) };
        m_InputEvents.MouseMove((m_MousePosition - position));

        m_MousePosition = position;
    }

    return true;
}

void Input::Update()
{
}