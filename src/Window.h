#pragma once

#include <StdInc.h>
#include <singleton.h>
#include <DropTarget.h>
#include <chrono>

static constexpr auto g_WindowName = "JC3 Render Block Model Renderer";

//#ifdef DEBUG
#define DEBUG_LOG(s)                                                                                                   \
    {                                                                                                                  \
        std::stringstream ss_;                                                                                         \
        ss_ << s << std::endl;                                                                                         \
        OutputDebugStringA(ss_.str().c_str());                                                                         \
    }
//#else
//#define DEBUG_LOG(s)
//#endif

struct WindowEvents {
    ksignals::Event<void(const glm::vec2& size)>    SizeChanged;
    ksignals::Event<void()>                         FocusLost;
    ksignals::Event<void()>                         FocusGained;
    ksignals::Event<void(const fs::path& filename)> DragEnter;
    ksignals::Event<void()>                         DragLeave;
    ksignals::Event<void()>                         DragDropped;
    ksignals::Event<void(const fs::path&)>          UnhandledDragDropped;
};

class Window : public Singleton<Window>
{
  private:
    WindowEvents m_WindowEvents;

    bool                                           m_Running    = true;
    HINSTANCE                                      m_Instance   = nullptr;
    HWND                                           m_Hwnd       = nullptr;
    bool                                           m_IsResizing = false;
    std::chrono::high_resolution_clock::time_point m_TimeSinceResize;
    bool                                           m_IsMouseCaptured = false;
    std::unique_ptr<DropTarget>                    m_DragDrop        = nullptr;

    static LRESULT CALLBACK WndProc(HWND hwnd, uint32_t message, WPARAM wParam, LPARAM lParam);

  public:
    Window()          = default;
    virtual ~Window() = default;

    virtual WindowEvents& Events()
    {
        return m_WindowEvents;
    }

    bool Initialise(const HINSTANCE& hInstance);
    void Shutdown();
    void BeginShutdown()
    {
        m_Running = false;
    }

    bool Frame();
    void Run();

    void StartResize()
    {
        m_IsResizing      = true;
        m_TimeSinceResize = std::chrono::high_resolution_clock::now();
    }

    glm::vec2 GetSize() const;
    glm::vec2 GetPosition() const;
    glm::vec2 GetCenterPoint() const;

    void CaptureMouse(bool capture);
    bool IsMouseCpatured() const
    {
        return m_IsMouseCaptured;
    }

    int32_t ShowMessageBox(const std::string& message, uint32_t type = MB_ICONERROR | MB_OK);
    void    ShowFolderSelection(const std::string& title, std::function<void(const fs::path&)> fn_selected,
                                std::function<void()> fn_cancelled = {});

    const HWND& GetHwnd() const
    {
        return m_Hwnd;
    }
    bool HasFocus() const
    {
        return (GetForegroundWindow() == m_Hwnd);
    }
};
