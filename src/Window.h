#pragma once

#include <StdInc.h>
#include "singleton.h"

static constexpr auto g_WindowName = "Engine";

#define DEBUG_LOG(s)                                    \
{                                                       \
    std::stringstream ss_;                              \
    ss_ << s << std::endl;                              \
    OutputDebugStringA(ss_.str().c_str());              \
}

struct WindowEvents
{
    ksignals::Event<void(const glm::vec2& size)> WindowResized;
};

class Window : public Singleton<Window>
{
private:
    WindowEvents m_WindowEvents;

    HINSTANCE m_Instance = nullptr;
    HWND m_Hwnd = nullptr;
    bool m_ClipCursor = false;

    static LRESULT CALLBACK WndProc(HWND hwnd, uint32_t message, WPARAM wParam, LPARAM lParam);

public:
    Window() = default;
    virtual ~Window() = default;

    virtual WindowEvents& Events() { return m_WindowEvents; }

    bool Initialise(const HINSTANCE& hInstance);
    void Shutdown();

    bool Frame();
    void Run();

    void UpdateClip();

    void SetClipEnabled(bool state) { m_ClipCursor = state; }
    bool IsClipEnabled() const { return m_ClipCursor; }

    glm::vec2 GetSize() const;
    glm::vec2 GetPosition() const;
    glm::vec2 GetCenterPoint() const;

    bool HasFocus() { return (GetForegroundWindow() == m_Hwnd); }
};