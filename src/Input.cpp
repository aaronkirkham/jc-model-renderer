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
        } 
        else {
            m_InputEvents.MouseDown(position);
        }

        m_LastClickPosition = position;
    }

    return true;
}

void Input::Update()
{
    auto window = Window::Get();
    if (window->IsClipEnabled()) {
        auto center = window->GetCenterPoint();

        POINT pointCursor;
        GetCursorPos(&pointCursor);

        auto cursorPos = glm::vec2{ pointCursor.x, pointCursor.y };
        if (cursorPos != center) {
            // reset to center of the window
            SetCursorPos(static_cast<int32_t>(center.x), static_cast<int32_t>(center.y));

            // calculate the delta
            auto cursorDelta = (cursorPos - center);
            if ((std::abs(cursorDelta.x) + std::abs(cursorDelta.y)) > 2) {
                m_InputEvents.MouseMove(cursorDelta);
            }
        }
    }
}

void Input::ResetCursor()
{
    auto center = Window::Get()->GetCenterPoint();
    SetCursorPos(static_cast<int32_t>(center.x), static_cast<int32_t>(center.y));
}