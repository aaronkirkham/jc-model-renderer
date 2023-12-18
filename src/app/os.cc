#include "pch.h"

#include "os.h"

#include "app/app.h"

#include <Windows.h>
#include <combaseapi.h>
#include <shellapi.h>
#include <shlobj_core.h>

#include <backends/imgui_impl_win32.h>

#ifndef NO_RENDERDOC
#include "renderdoc_app.h"
static RENDERDOC_API_1_1_2* s_rdoc_api = nullptr;
#endif

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

namespace jcmr::os
{
static constexpr const char* WNDCLASS_NAME = "jcmr_win32";

struct AppContext {
    App*                 app  = nullptr;
    bool                 quit = false;
    WNDCLASS             wc{};
    std::vector<Window*> windows;

    Window* find_window(WindowHandle handle) const
    {
        for (const auto& window : windows) {
            if (window->handle == handle) {
                return window;
            }
        }

        return nullptr;
    }
};

static AppContext s_app_context;

void init(App& app)
{
    s_app_context.app = &app;

    static auto WndProc = [](HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) -> LRESULT {
        // pass input to imgui first
        if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) {
            return 1;
        }

        auto* app = s_app_context.app;
        if (app) {
            Event event{};

            auto* window = s_app_context.find_window(hWnd);
            event.window = window;

            switch (uMsg) {
                case WM_CLOSE: {
                    event.type = Event::Type::WINDOW_CLOSE;
                    app->on_event(event);
                    break;
                }

                case WM_MOVE: {
                    event.type          = Event::Type::WINDOW_MOVE;
                    event.window_move.x = (i16)LOWORD(lParam);
                    event.window_move.y = (i16)HIWORD(lParam);
                    app->on_event(event);
                    break;
                }

                case WM_SIZE: {
                    auto width  = (i32)LOWORD(lParam);
                    auto height = (i32)HIWORD(lParam);

                    if (window && !window->resizing && (window->width != width || window->height != height)) {
                        window->width  = width;
                        window->height = height;

                        event.type               = Event::Type::WINDOW_SIZE;
                        event.window_size.width  = width;
                        event.window_size.height = height;
                        app->on_event(event);
                    }

                    break;
                }

                case WM_ENTERSIZEMOVE: {
                    window->resizing = true;
                    return 0;
                }

                case WM_EXITSIZEMOVE: {
                    window->resizing = false;

                    // window dimensions changed
                    auto rect = get_window_rect(event.window);
                    if (window->width != rect.width || window->height != rect.height) {
                        window->width  = rect.width;
                        window->height = rect.height;

                        event.type               = Event::Type::WINDOW_SIZE;
                        event.window_size.width  = rect.width;
                        event.window_size.height = rect.height;
                        app->on_event(event);
                    }

                    return 0;
                }

                case WM_ACTIVATE: {
                    if (wParam == WA_INACTIVE) {
                        //
                    }

                    event.type                = Event::Type::WINDOW_FOCUS;
                    event.window_focus.gained = (wParam != WA_INACTIVE);
                    app->on_event(event);
                    break;
                }

                case WM_MENUCHAR: {
                    // prevent windows playing chime
                    return (MNC_CLOSE << 16);
                }
            }
        }

        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    };

    s_app_context.wc.style         = 0;
    s_app_context.wc.lpfnWndProc   = WndProc;
    s_app_context.wc.cbClsExtra    = 0;
    s_app_context.wc.cbWndExtra    = 0;
    s_app_context.wc.hInstance     = GetModuleHandle(nullptr);
    s_app_context.wc.hIcon         = LoadIcon(nullptr, IDI_APPLICATION);
    s_app_context.wc.hCursor       = nullptr;
    s_app_context.wc.hbrBackground = nullptr;
    s_app_context.wc.lpszClassName = WNDCLASS_NAME;

    auto result = RegisterClass(&s_app_context.wc);
    ASSERT(SUCCEEDED(result));

#ifndef NO_RENDERDOC
    if (auto* handle = GetModuleHandle("renderdoc.dll")) {
        auto RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(handle, "RENDERDOC_GetAPI");
        auto ret              = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**)&s_rdoc_api);
        ASSERT(ret == 1);

        LOG_INFO("renderdoc.dll is loaded.");
    }
#endif

    app.on_init();

    while (!s_app_context.quit) {
        MSG msg{};
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            bool  discard = false;
            Event event{};
            event.window = s_app_context.find_window(msg.hwnd);

            switch (msg.message) {
                case WM_QUIT: {
                    event.type = Event::Type::QUIT;
                    app.on_event(event);
                    break;
                }
            }

            if (!discard) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        app.on_tick();
    }

    app.on_shutdown();
    shutdown();
}

void shutdown()
{
    UnregisterClass(WNDCLASS_NAME, s_app_context.wc.hInstance);
}

Window* create_window(const InitWindowArgs& init_window_args)
{
    auto window    = new Window;
    u32  style     = init_window_args.flags & InitWindowArgs::NO_DECORATION ? WS_POPUP : WS_OVERLAPPEDWINDOW;
    u32  ext_style = 0;

    // center window if requested
    Point pos = init_window_args.position;
    if (init_window_args.flags & InitWindowArgs::CENTERED) {
        pos.x = ((GetSystemMetrics(SM_CXSCREEN) / 2) - (init_window_args.size.width / 2));
        pos.y = ((GetSystemMetrics(SM_CYSCREEN) / 2) - (init_window_args.size.height / 2));
    }

    auto hwnd = CreateWindowEx(ext_style, WNDCLASS_NAME, init_window_args.name, style, pos.x, pos.y,
                               init_window_args.size.width, init_window_args.size.height, (HWND)init_window_args.parent,
                               nullptr, s_app_context.wc.hInstance, nullptr);
    if (hwnd == INVALID_WINDOW_HANDLE) {
        auto err = GetLastError();
        delete window;
        return nullptr;
    }

    // accept drag/drop files
    if (init_window_args.flags & InitWindowArgs::ACCEPT_DROP_FILES) {
        DragAcceptFiles(hwnd, TRUE);
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    window->handle   = static_cast<WindowHandle>(hwnd);
    window->width    = init_window_args.size.width;
    window->height   = init_window_args.size.height;
    window->resizing = false;

    s_app_context.windows.push_back(window);
    return window;
}

void destroy_window(Window* window)
{
    ASSERT(window);
    DestroyWindow(static_cast<HWND>(window->handle));
    delete window;
}

void quit()
{
    s_app_context.quit = true;
}

void set_window_title(Window* window, const char* title)
{
    SetWindowText((HWND)window->handle, title);
}

void set_window_rect(Window* window, const Rect& rect)
{
    MoveWindow((HWND)window->handle, rect.left, rect.top, rect.width, rect.height, TRUE);
}

Rect get_window_rect(Window* window)
{
    RECT rect{};
    BOOL status = GetClientRect((HWND)window->handle, &rect);
    ASSERT(status != false);

    POINT point{rect.left, rect.top};
    ClientToScreen((HWND)window->handle, &point);
    return {point.x, point.y, (rect.right - rect.left), (rect.bottom - rect.top)};
}

i32 get_dpi()
{
    const HDC hdc = GetDC(nullptr);
    return GetDeviceCaps(hdc, LOGPIXELSX);
}

u64 get_tick_count()
{
    return GetTickCount64();
}

ByteArray map_view_of_file(const char* filename, u64 offset, u64 size)
{
    // ULARGE_INTEGER _offset;
    // _offset.QuadPart = offset;

    // TODO : if there's a benefit, we should only map between offset and size.
    //        because the mapping has to be aligned to the allocation granularity of the system
    //        we probably need to do some math to get the correct offset/sizes which fall in that page.

    auto  file_handle = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    auto  handle      = CreateFileMapping(file_handle, nullptr, PAGE_READONLY, 0, 0, "name");
    auto* ptr         = (char*)MapViewOfFile(handle, FILE_MAP_READ, 0, 0, 0);

    ByteArray result(size);
    std::memcpy(result.data(), ((char*)(ptr + offset)), size);

    UnmapViewOfFile(ptr);
    CloseHandle(handle);
    CloseHandle(file_handle);

    return result;
}

template <i32 N> static void toWChar(WCHAR (&out)[N], const char* in)
{
    const char* c    = in;
    WCHAR*      cout = out;
    while (*c && ((c - in) < (N - 1))) {
        *cout = *c;
        ++cout;
        ++c;
    }

    *cout = '\0';
}

template <i32 N> struct WCharString {
    WCharString(const char* rhs) { toWChar(data, rhs); }
    operator const WCHAR*() const { return data; }
    WCHAR data[N];
};

static bool create_file_dialog(std::filesystem::path* out_path, const CLSID& clsid, const std::string_view& title,
                               const FileDialogParams& params, u32 flags, Window* parent = nullptr)
{
    bool result = false;

    auto hr = CoInitializeEx(nullptr, (COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE));

    if (SUCCEEDED(hr)) {
        IFileDialog* dialog = nullptr;

        hr = CoCreateInstance(clsid, nullptr, CLSCTX_ALL, IID_IFileDialog, (void**)&dialog);
        if (SUCCEEDED(hr)) {
            // set title and window flags
            WCharString<MAX_PATH_LENGTH> _title(title.data());
            dialog->SetTitle(_title);
            dialog->SetOptions(flags);

            // set default filename
            if (params.filename[0] != '\0') {
                WCharString<MAX_PATH_LENGTH> tmp(params.filename);
                dialog->SetFileName(tmp);

                // WCharString<MAX_PATH_LENGTH> tmp2("hello");
                // dialog->SetFileNameLabel(tmp2);
            }

            // set default extension
            if (params.extension[0] != '\0') {
                WCharString<MAX_PATH_LENGTH> tmp(params.extension);
                dialog->SetDefaultExtension(tmp);
            }

            // set filters
            {
                std::vector<COMDLG_FILTERSPEC> specs;
                if (params.filters.empty()) {
                    specs.push_back({L"All Files (*.*)", L"*.*"});
                } else {
                    specs.reserve(params.filters.size());
                    for (const auto& filter : params.filters) {
                        COMDLG_FILTERSPEC spec{};

                        // copy filter name string
                        WCharString<256> wszName(filter.name);
                        spec.pszName = wszName;

                        // copy filter spec extension string
                        WCharString<256> wszSpec(filter.spec);
                        spec.pszSpec = wszSpec;

                        specs.emplace_back(std::move(spec));
                    }
                }

                dialog->SetFileTypes(specs.size(), specs.data());
            }

            // show dialog window
            hr = dialog->Show(parent ? (HWND)parent->handle : nullptr);
            if (SUCCEEDED(hr)) {
                IShellItem* item = nullptr;

                // get dialog selection
                hr = dialog->GetResult(&item);
                if (SUCCEEDED(hr)) {
                    PWSTR file_path;

                    // get filename of selected file
                    hr = item->GetDisplayName(SIGDN_FILESYSPATH, &file_path);
                    if (SUCCEEDED(hr)) {
                        *out_path = file_path;
                        CoTaskMemFree(file_path);
                        result = true;
                    }

                    item->Release();
                }
            }

            dialog->Release();
        }
    }

    CoUninitialize();
    return result;
}

bool get_open_file(FileDialogParams& params, std::filesystem::path* out_path)
{
    return get_open_file("Select file", params, out_path);
}

bool get_open_file(const char* title, FileDialogParams& params, std::filesystem::path* out_path)
{
    return create_file_dialog(out_path, CLSID_FileOpenDialog, title, params, FOS_FILEMUSTEXIST,
                              s_app_context.windows[0]);
}

bool get_open_folder(FileDialogParams& params, std::filesystem::path* out_path)
{
    return get_open_folder("Select folder", params, out_path);
}

bool get_open_folder(const char* title, FileDialogParams& params, std::filesystem::path* out_path)
{
    return create_file_dialog(out_path, CLSID_FileOpenDialog, title, params, (FOS_PICKFOLDERS | FOS_PATHMUSTEXIST),
                              s_app_context.windows[0]);
}
} // namespace jcmr::os