#include <Window.h>
#include <Input.h>
#include <graphics/Renderer.h>
#include <examples/directx11_example/imgui_impl_dx11.h>

#include <sstream>

extern LRESULT ImGui_ImplDX11_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK Window::WndProc(HWND hwnd, uint32_t message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplDX11_WndProcHandler(hwnd, message, wParam, lParam))
    {
        return 1;
    }

    if (message == WM_SIZE)
    {
        Window::Get()->Events().WindowResized(glm::vec2{ static_cast<float>(LOWORD(lParam)), static_cast<float>(HIWORD(lParam)) });
        return 0;
    }
    else if (message == WM_SYSCOMMAND)
    {
        // Disable ALT application menu
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
    }
    else if (message == WM_DESTROY || message == WM_CLOSE)
    {
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

bool Window::Initialise(const HINSTANCE& instance)
{
    static auto size = glm::vec2{ 1280, 960 };
    m_Instance = instance;

    WNDCLASSEX wc;

    wc.style = (CS_HREDRAW | CS_VREDRAW | CS_OWNDC);
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = instance;
    wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    wc.hIconSm = wc.hIcon;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wc.lpszMenuName = NULL;
    wc.lpszClassName = g_WindowName;
    wc.cbSize = sizeof(WNDCLASSEX);

    RegisterClassEx(&wc);

    auto x = (GetSystemMetrics(SM_CXSCREEN) - static_cast<int32_t>(size.x)) / 2;
    auto y = (GetSystemMetrics(SM_CYSCREEN) - static_cast<int32_t>(size.y)) / 2;

    m_Hwnd = CreateWindowA(g_WindowName, g_WindowName, WS_OVERLAPPEDWINDOW, x, y, static_cast<int32_t>(size.x), static_cast<int32_t>(size.y), nullptr, nullptr, instance, nullptr);

    ShowWindow(m_Hwnd, SW_SHOW);
    UpdateWindow(m_Hwnd);
    SetForegroundWindow(m_Hwnd);
    SetFocus(m_Hwnd);
    ShowCursor(true);

    Input::Get()->Initialise();
    
    return Renderer::Get()->Initialise(m_Hwnd);
}

void Window::Shutdown()
{
    Renderer::Get()->Shutdown();

    ShowCursor(true);

    DestroyWindow(m_Hwnd);
    m_Hwnd = nullptr;

    UnregisterClass(g_WindowName, m_Instance);
    m_Instance = nullptr;
}

bool Window::Frame()
{
    UpdateClip();
    Input::Get()->Update();

    return Renderer::Get()->Render();
}

void Window::Run()
{
    bool running = true;
    while (running)
    {
        // handle messages
        MSG msg{};
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE) > 0)
        {
            if (msg.message == WM_QUIT) {
                running = false;
            }

            Input::Get()->HandleMessage(&msg);

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // update the window
        Frame();
    }
}

void Window::UpdateClip()
{
    static bool m_LastClip = !m_ClipCursor;

    if (m_ClipCursor == m_LastClip) {
        return;
    }

    if (m_ClipCursor) {
        RECT rect;
        GetClientRect(m_Hwnd, &rect);

        ClientToScreen(m_Hwnd, reinterpret_cast<POINT*>(&rect.left));
        ClientToScreen(m_Hwnd, reinterpret_cast<POINT*>(&rect.right));

        ClipCursor(&rect);
        ShowCursor(false);
    }
    else {
        ClipCursor(nullptr);
        ShowCursor(true);
    }

    m_LastClip = m_ClipCursor;
}

void Window::DebugString(const char* format, ...)
{
    va_list args;
    char buffer[2048];
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    std::stringstream str;
    str << buffer << std::endl;

    OutputDebugStringA(str.str().c_str());
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
    return GetPosition() + glm::vec2{ size.x / 2, size.y / 2 };
}