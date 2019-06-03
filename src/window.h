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

using FileSelectionFilter = std::pair<std::string, std::string>;
struct FileSelectionParams {
    std::string                      Filename;
    std::string                      Extension;
    std::vector<FileSelectionFilter> Filters;
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

    std::filesystem::path CreateFileDialog(const CLSID& clsid, const std::string& title,
                                           const FileSelectionParams& params = {}, uint32_t flags = 0);

    void ConvertFileSelectionFilters(std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>& converter,
                                     const std::vector<FileSelectionFilter>& filters, std::vector<std::wstring>* buffer,
                                     std::vector<COMDLG_FILTERSPEC>* filterspec);

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

    int32_t               ShowMessageBox(const std::string& message, uint32_t type = MB_ICONWARNING | MB_OK);
    std::filesystem::path ShowFileSelecton(const std::string& title, const FileSelectionParams& params = {},
                                           uint32_t flags = 0);
    std::filesystem::path ShowFolderSelection(const std::string& title, const FileSelectionParams& params = {},
                                              uint32_t flags = 0);
    std::filesystem::path ShowSaveDialog(const std::string& title, const FileSelectionParams& params = {},
                                         uint32_t flags = 0);

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
