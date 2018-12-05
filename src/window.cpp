#include <ShlObj.h>
#include <Windows.h>
#include <commdlg.h>
#include <shellapi.h>

#include "graphics/renderer.h"
#include "settings.h"
#include "version.h"
#include "window.h"

#include "game/formats/avalanche_archive.h"
#include "game/formats/render_block_model.h"
#include "game/formats/runtime_container.h"

#include <examples/imgui_impl_win32.h>
#include <imgui.h>

#include <httplib.h>

#ifdef DEBUG
#include <spdlog/sinks/stdout_color_sinks.h>
#endif

extern bool g_IsJC4Mode;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK Window::WndProc(HWND hwnd, uint32_t message, WPARAM wParam, LPARAM lParam)
{
    // pass input to imgui first
    if (ImGui_ImplWin32_WndProcHandler(hwnd, message, wParam, lParam)) {
        return 1;
    }

    switch (message) {
        case WM_SIZE: {
            Window::Get()->StartResize();
            break;
        }

        case WM_SETFOCUS: {
            Window::Get()->Events().FocusGained();
            break;
        }

        case WM_KILLFOCUS: {
            Window::Get()->Events().FocusLost();
            break;
        }

        case WM_CLOSE: {
            DestroyWindow(hwnd);
            break;
        }

        case WM_DESTROY: {
            PostQuitMessage(0);
            break;
        }

        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0;
}

bool Window::Initialise(const HINSTANCE& instance)
{
#ifdef DEBUG
    if (AllocConsole()) {
        freopen("CONOUT$", "w", stdout);
        SetConsoleTitle("Debug Console");
    }

    // spdlog::set_level(spdlog::level::trace);
    spdlog::set_pattern("[%H:%M:%S] [%^%l%$] %v");

    m_Log = spdlog::stdout_color_mt("console");
#endif

    static auto size = glm::vec2{1480, 870};
    m_Instance       = instance;

    WNDCLASSEX wc{};
    wc.style         = (CS_HREDRAW | CS_VREDRAW | CS_OWNDC);
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = instance;
    wc.hIcon         = LoadIcon(nullptr, IDI_WINLOGO);
    wc.hIconSm       = wc.hIcon;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wc.lpszMenuName  = nullptr;
    wc.lpszClassName = g_WindowName;
    wc.cbSize        = sizeof(WNDCLASSEX);

    RegisterClassEx(&wc);

    auto x = (GetSystemMetrics(SM_CXSCREEN) - static_cast<int32_t>(size.x)) / 2;
    auto y = (GetSystemMetrics(SM_CYSCREEN) - static_cast<int32_t>(size.y)) / 2;

    m_Hwnd = CreateWindowA(g_WindowName, g_WindowName, WS_OVERLAPPEDWINDOW, x, y, static_cast<int32_t>(size.x),
                           static_cast<int32_t>(size.y), nullptr, nullptr, instance, this);

    ShowWindow(m_Hwnd, SW_SHOW);
    UpdateWindow(m_Hwnd);
    SetForegroundWindow(m_Hwnd);
    SetFocus(m_Hwnd);
    ShowCursor(true);

    m_DragDrop = std::make_unique<DropTarget>(m_Hwnd);

    return Renderer::Get()->Initialise(m_Hwnd);
}

void Window::Shutdown()
{
    m_Running = false;

    // clear factories
    // TODO: move me!
    RuntimeContainer::Instances.clear();
    RenderBlockModel::Instances.clear();
    AvalancheArchive::Instances.clear();

    Renderer::Get()->Shutdown();

    ShowCursor(true);
    DestroyWindow(m_Hwnd);
    m_Hwnd = nullptr;

    UnregisterClass(g_WindowName, m_Instance);
    m_Instance = nullptr;

#ifdef DEBUG
    FreeConsole();
#endif
}

void Window::Run()
{
    using clock = std::chrono::high_resolution_clock;
    auto start  = clock::now();

    while (m_Running) {
        const auto diff        = clock::now() - start;
        start                  = clock::now();
        const float delta_time = std::chrono::duration_cast<std::chrono::milliseconds>(diff).count() / 1000.0f;

        // handle messages
        MSG msg{};
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE) > 0) {
            if (msg.message == WM_QUIT) {
                goto shutdown;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // handle window resize
        if (m_IsResizing) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - m_TimeSinceResize);
            if (duration.count() > 50) {
                m_IsResizing = false;
                Window::Get()->Events().SizeChanged(Window::Get()->GetSize());
            }
        }

        // render
        Renderer::Get()->Render(delta_time);

        // if the window is minimized, sleep for a bit
        if (IsIconic(m_Hwnd)) {
            // TODO: check if we're doing any work, importing/exporting anything and then sleep for less time
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

shutdown:
    Shutdown();
}

glm::vec2 Window::GetSize() const
{
    RECT rect{};
    GetClientRect(m_Hwnd, &rect);

    return glm::vec2{(rect.right - rect.left), (rect.bottom - rect.top)};
}

glm::vec2 Window::GetPosition() const
{
    RECT rect;
    GetWindowRect(m_Hwnd, &rect);

    return glm::vec2{rect.left, rect.top};
}

glm::vec2 Window::GetCenterPoint() const
{
    auto size = GetSize();
    return glm::vec2{size.x / 2, size.y / 2};
}

void Window::CaptureMouse(bool capture)
{
    if (capture && !m_IsMouseCaptured) {
        SetCapture(m_Hwnd);
        m_IsMouseCaptured = true;
    } else if (!capture && m_IsMouseCaptured) {
        ReleaseCapture();
        m_IsMouseCaptured = false;
    }
}

int32_t Window::ShowMessageBox(const std::string& message, uint32_t type)
{
    auto result = MessageBox(m_Hwnd, message.c_str(), g_WindowName, type);
    if (type == MB_ICONERROR) {
        TerminateProcess(GetCurrentProcess(), -1);
    }

    return result;
}

void Window::ShowFileSelection(const std::string& title, const std::string& filter,
                               std::function<void(const std::filesystem::path&)> fn_selected)
{
    char         filename[MAX_PATH] = {0};
    OPENFILENAME ofn                = {0};

    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner   = m_Hwnd;
    ofn.lpstrFilter = filter.c_str();
    ofn.lpstrFile   = filename;
    ofn.nMaxFile    = MAX_PATH;
    ofn.lpstrTitle  = title.c_str();
    ofn.Flags       = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn)) {
        fn_selected(filename);
    }
}

void Window::ShowFolderSelection(const std::string&                                title,
                                 std::function<void(const std::filesystem::path&)> fn_selected,
                                 std::function<void()>                             fn_cancelled)
{
    TCHAR      path[MAX_PATH] = {0};
    BROWSEINFO browse_info    = {0};

    browse_info.hwndOwner      = m_Hwnd;
    browse_info.pidlRoot       = nullptr;
    browse_info.pszDisplayName = path;
    browse_info.lpszTitle      = title.c_str();
    browse_info.ulFlags        = BIF_RETURNONLYFSDIRS;
    browse_info.lpfn           = nullptr;

    LPITEMIDLIST pidl = SHBrowseForFolder(&browse_info);
    if (pidl) {
        SHGetPathFromIDList(pidl, path);
        fn_selected(path);
    } else if (fn_cancelled) {
        fn_cancelled();
    }
}

void Window::SelectJustCauseDirectory()
{
    std::string title = g_IsJC4Mode ? "Please select your Just Cause 4 install folder"
                                    : "Please select your Just Cause 3 install folder";

    // ask the user to select their jc directory, we can't find it!
    ShowFolderSelection(
        title,
        [&](const std::filesystem::path& selected) {
            Settings::Get()->SetValue(g_IsJC4Mode ? "jc4_directory" : "jc3_directory", selected.string());
            LOG_INFO("Selected directory: \"{}\"", selected.string());
        },
        [] {
            const auto& jc_directory =
                Settings::Get()->GetValue<std::string>(g_IsJC4Mode ? "jc4_directory" : "jc3_directory");
            if (jc_directory.empty()) {
                Window::Get()->ShowMessageBox(
                    "Unable to find Just Cause root directory.\n\nSome features will be disabled.");
            }
        });
}

std::filesystem::path Window::GetJustCauseDirectory()
{
    return Settings::Get()->GetValue<std::string>(g_IsJC4Mode ? "jc4_directory" : "jc3_directory");
}

void Window::CheckForUpdates(bool show_no_update_messagebox)
{
    static int32_t current_version[3] = {VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION};

    std::thread([show_no_update_messagebox] {
        try {
            httplib::Client client("kirkh.am");
            const auto      res = client.get("/jc3-rbm-renderer/latest.json");
            if (res && res->status == 200) {
                const auto& data        = nlohmann::json::parse(res->body);
                const auto& version_str = data["version"].get<std::string>();

                int32_t latest_version[3] = {0};
                std::sscanf(version_str.c_str(), "%d.%d.%d", &latest_version[0], &latest_version[1],
                            &latest_version[2]);

                if (std::lexicographical_compare(current_version, current_version + 3, latest_version,
                                                 latest_version + 3)) {
                    std::string msg = "A new version (" + version_str + ") is available for download.\n\n";
                    msg.append("Do you want to go to the release page on GitHub?");

                    if (Window::Get()->ShowMessageBox(msg, MB_ICONQUESTION | MB_YESNO) == IDYES) {
                        ShellExecuteA(nullptr, "open",
                                      "https://github.com/aaronkirkham/jc-rbm-renderer/releases/latest", nullptr,
                                      nullptr, SW_SHOWNORMAL);
                    }
                } else if (show_no_update_messagebox) {
                    Window::Get()->ShowMessageBox("You have the latest version!", MB_ICONINFORMATION);
                }
            }
        } catch (...) {
            LOG_ERROR("Failed to check for updates");
        }
    }).detach();
}
