#include <Window.h>
#include <Input.h>
#include <graphics/Renderer.h>
#include <examples/imgui_impl_win32.h>

#include <sstream>
#include <shlobj.h>
#include <shellapi.h>

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK Window::WndProc(HWND hwnd, uint32_t message, WPARAM wParam, LPARAM lParam)
{
    static bool m_IsResizing = false;
    static std::chrono::high_resolution_clock::time_point m_ResizeTime;

    // handle the resize
    if (m_IsResizing) {
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - m_ResizeTime);
        if (duration.count() > 50) {
            m_IsResizing = false;
            Window::Get()->Events().SizeChanged(Window::Get()->GetSize());
        }
    }

    // pass input to imgui first
    if (ImGui_ImplWin32_WndProcHandler(hwnd, message, wParam, lParam)) {
        return 1;
    }

    switch (message) {
        // resize handler
    case WM_SIZE:
        m_IsResizing = true;
        m_ResizeTime = std::chrono::high_resolution_clock::now();
        break;

        // focus gained handler
    case WM_SETFOCUS:
        Window::Get()->Events().FocusGained();
        break;

        // focus lost handler
    case WM_KILLFOCUS:
        Window::Get()->Events().FocusLost();
        break;

        // file drop handler
    case WM_DROPFILES: {
        const auto drop = reinterpret_cast<HDROP>(wParam);
        char file[MAX_PATH] = { 0 };

        if (DragQueryFile(drop, 0, file, MAX_PATH) != 0) {
            Window::Get()->Events().FileDropped(file);
            DragFinish(drop);
        }

        break;
    }

    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
   
    return 0;
}

bool Window::Initialise(const HINSTANCE& instance)
{
    static auto size = glm::vec2{ 1480, 870 };
    m_Instance = instance;

    WNDCLASSEX wc;

    wc.style = (CS_HREDRAW | CS_VREDRAW | CS_OWNDC);
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = instance;
    wc.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
    wc.hIconSm = wc.hIcon;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = g_WindowName;
    wc.cbSize = sizeof(WNDCLASSEX);

    RegisterClassEx(&wc);

    auto x = (GetSystemMetrics(SM_CXSCREEN) - static_cast<int32_t>(size.x)) / 2;
    auto y = (GetSystemMetrics(SM_CYSCREEN) - static_cast<int32_t>(size.y)) / 2;

    m_Hwnd = CreateWindowA(g_WindowName, g_WindowName, WS_OVERLAPPEDWINDOW, x, y, static_cast<int32_t>(size.x), static_cast<int32_t>(size.y), nullptr, nullptr, instance, this);

    ShowWindow(m_Hwnd, SW_SHOW);
    UpdateWindow(m_Hwnd);
    SetForegroundWindow(m_Hwnd);
    SetFocus(m_Hwnd);
    ShowCursor(true);

    DragAcceptFiles(m_Hwnd, true);

    Input::Get()->Initialise();

    return Renderer::Get()->Initialise(m_Hwnd);
}

void Window::Shutdown()
{
    m_Running = false;

    Renderer::Get()->Shutdown();

    ShowCursor(true);
    DestroyWindow(m_Hwnd);
    m_Hwnd = nullptr;

    UnregisterClass(g_WindowName, m_Instance);
    m_Instance = nullptr;
}

bool Window::Frame()
{
    return Renderer::Get()->Render();
}

void Window::Run()
{
    while (m_Running) {
        // handle messages
        MSG msg{};
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE) > 0) {
            if (msg.message == WM_QUIT) {
                goto shutdown;
            }

            // if the window has focus, pass input to the input handler
            if (HasFocus()) {
                Input::Get()->HandleMessage(&msg);
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // update the window
        Frame();

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

    return glm::vec2{ (rect.right - rect.left), (rect.bottom - rect.top) };
}

glm::vec2 Window::GetPosition() const
{
    RECT rect;
    GetWindowRect(m_Hwnd, &rect);

    return glm::vec2{ rect.left, rect.top };
}

glm::vec2 Window::GetCenterPoint() const
{
    auto size = GetSize();
    return glm::vec2{ size.x / 2, size.y / 2 };
}

void Window::CaptureMouse(bool capture)
{
    if (capture && !m_IsMouseCaptured) {
        SetCapture(m_Hwnd);
        m_IsMouseCaptured = true;
    }
    else if (!capture && m_IsMouseCaptured) {
        ReleaseCapture();
        m_IsMouseCaptured = false;
    }
}

int32_t Window::ShowMessageBox(const std::string& message, uint32_t type)
{
    return MessageBox(m_Hwnd, message.c_str(), g_WindowName, type);
}

void Window::ShowFolderSelection(const std::string& title, std::function<void(const std::string&)> fn_selected, std::function<void()> fn_cancelled)
{
    TCHAR path[MAX_PATH];

    BROWSEINFO browse_info;
    ZeroMemory(&browse_info, sizeof(BROWSEINFO));

    browse_info.hwndOwner = m_Hwnd;
    browse_info.pidlRoot = nullptr;
    browse_info.pszDisplayName = path;
    browse_info.lpszTitle = title.c_str();
    browse_info.ulFlags = BIF_RETURNONLYFSDIRS;
    browse_info.lpfn = nullptr;

    LPITEMIDLIST pidl = SHBrowseForFolder(&browse_info);
    if (pidl) {
        SHGetPathFromIDList(pidl, path);
        fn_selected(path);
    }
    else if (fn_cancelled) {
        fn_cancelled();
    }
}