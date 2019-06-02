#pragma once

#include <filesystem>
#include <sstream>

#include "drop_target.h"
#include "singleton.h"

#include <glm/vec2.hpp>
#include <ksignals.h>

#ifdef DEBUG
#define SPDLOG_ACTIVE_LEVEL 1
#endif

#include <spdlog/spdlog.h>

#ifdef DEBUG
static constexpr auto g_WindowName = "Just Cause Model Renderer (DEBUG)";
#else
static constexpr auto g_WindowName = "Just Cause Model Renderer";
#endif

struct WindowEvents {
    ksignals::Event<void(const glm::vec2& size)>                           SizeChanged;
    ksignals::Event<void()>                                                FocusLost;
    ksignals::Event<void()>                                                FocusGained;
    ksignals::Event<void(const std::vector<std::filesystem::path>& files)> DragEnter;
    ksignals::Event<void()>                                                DragLeave;
    ksignals::Event<void()>                                                DragDropped;
};

class Window : public Singleton<Window>
{
  private:
    WindowEvents m_WindowEvents;

#ifdef DEBUG
    std::shared_ptr<spdlog::logger> m_Log = nullptr;
#endif

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

    int32_t ShowMessageBox(const std::string& message, uint32_t type = MB_ICONWARNING | MB_OK);
    void    ShowFileSelection(const std::string& title, const char* default_filename, const char* filter,
                              const char* default_extension, std::function<void(const std::filesystem::path&)> fn_selected,
                              std::function<void()> fn_cancelled = {});
    void    ShowFileFolderSelection(const std::string& title, const char* default_filename, const char* filter,
                                    const char*                                       default_extension,
                                    std::function<void(const std::filesystem::path&)> fn_selected,
                                    std::function<void()>                             fn_cancelled = {});
    void    ShowFolderSelection(const std::string& title, uint32_t flags, std::function<void(const std::filesystem::path&)> fn_selected,
                                std::function<void()> fn_cancelled = {});

    void                  SwitchMode(bool jc4_mode);
    void                  SelectJustCauseDirectory(bool override_mode = false, bool jc3_mode = true);
    std::filesystem::path GetJustCauseDirectory();
    void                  CheckForUpdates(bool show_no_update_messagebox = false);

    bool LoadInternalResource(int32_t resource_id, std::vector<uint8_t>* out_buffer);

    const HWND& GetHwnd() const
    {
        return m_Hwnd;
    }

    bool HasFocus() const
    {
        return (GetForegroundWindow() == m_Hwnd);
    }

#ifdef DEBUG
    spdlog::logger* GetLog()
    {
        return m_Log.get();
    }
#endif
};
