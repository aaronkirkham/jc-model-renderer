#pragma once

#include <StdInc.h>
#include <singleton.h>

#include <chrono>

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

    bool m_Running = true;
    HINSTANCE m_Instance = nullptr;
    HWND m_Hwnd = nullptr;

    static LRESULT CALLBACK WndProc(HWND hwnd, uint32_t message, WPARAM wParam, LPARAM lParam);

public:
    Window() = default;
    virtual ~Window() = default;

    virtual WindowEvents& Events() { return m_WindowEvents; }

    bool Initialise(const HINSTANCE& hInstance);
    void Shutdown();

    bool Frame();
    void Run();

    glm::vec2 GetSize() const;
    glm::vec2 GetPosition() const;
    glm::vec2 GetCenterPoint() const;

    int32_t ShowMessageBox(const std::string& message, uint32_t type = MB_ICONERROR | MB_OK);
    void ShowFolderSelection(const std::string& title, std::function<void(const std::string&)> fn_selected, std::function<void()> fn_cancelled = {});

    const HWND& GetHwnd() const { return m_Hwnd; }
    bool HasFocus() const { return (GetForegroundWindow() == m_Hwnd); }
};