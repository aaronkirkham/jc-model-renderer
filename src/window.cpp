#include <Windows.h>
#include <shellapi.h>

#include <rapidjson/document.h>

#include <httplib.h>
#ifdef DEBUG
#include <spdlog/sinks/stdout_color_sinks.h>
#endif

#include "settings.h"
#include "version.h"
#include "window.h"

#include "graphics/renderer.h"
#include "graphics/shader_manager.h"
#include "graphics/texture_manager.h"
#include "graphics/ui.h"

#include "game/file_loader.h"
#include "game/formats/avalanche_archive.h"
#include "game/formats/avalanche_model_format.h"
#include "game/formats/render_block_model.h"
#include "game/formats/runtime_container.h"

extern bool g_IsJC4Mode;

extern LRESULT   ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
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

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_TRACE
    spdlog::set_level(spdlog::level::trace);
#endif

    spdlog::set_pattern("[%^%l%$] [%!] %v");
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

std::filesystem::path Window::CreateFileDialog(const CLSID& clsid, const std::string& title,
                                               const FileSelectionParams& params, uint32_t flags)
{
    std::filesystem::path selected = "";
    HRESULT               hr       = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr)) {
        // create the file open dialog object
        IFileDialog* dialog = nullptr;
        hr                  = CoCreateInstance(clsid, nullptr, CLSCTX_ALL, IID_IFileDialog, (void**)&dialog);

        if (SUCCEEDED(hr)) {
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

            // set title and flags
            dialog->SetTitle(converter.from_bytes(title).c_str());
            dialog->SetOptions(flags);

            // set default filename
            if (!params.Filename.empty()) {
                dialog->SetFileName(converter.from_bytes(params.Filename).c_str());
            }

            // set default extension
            if (!params.Extension.empty()) {
                dialog->SetDefaultExtension(converter.from_bytes(params.Extension).c_str());
            }

            // set filters
            std::vector<std::wstring>      buffer;
            std::vector<COMDLG_FILTERSPEC> filterspec;
            ConvertFileSelectionFilters(converter, params.Filters, &buffer, &filterspec);
            if (!filterspec.empty()) {
                dialog->SetFileTypes(filterspec.size(), filterspec.data());
            }

            hr = dialog->Show(m_Hwnd);

            if (SUCCEEDED(hr)) {
                IShellItem* item = nullptr;
                hr               = dialog->GetResult(&item);

                if (SUCCEEDED(hr)) {
                    PWSTR file_path;
                    hr = item->GetDisplayName(SIGDN_FILESYSPATH, &file_path);

                    if (SUCCEEDED(hr)) {
                        selected = file_path;
                        CoTaskMemFree(file_path);
                    }

                    item->Release();
                }
            }

            dialog->Release();
        }
    }

    CoUninitialize();
    return selected;
}

std::filesystem::path Window::ShowFileSelecton(const std::string& title, const FileSelectionParams& params,
                                               uint32_t flags)
{
    return CreateFileDialog(CLSID_FileOpenDialog, title, params, (flags | FOS_FILEMUSTEXIST));
}

std::filesystem::path Window::ShowFolderSelection(const std::string& title, const FileSelectionParams& params,
                                                  uint32_t flags)
{
    return CreateFileDialog(CLSID_FileOpenDialog, title, params, (flags | FOS_PICKFOLDERS | FOS_PATHMUSTEXIST));
}

std::filesystem::path Window::ShowSaveDialog(const std::string& title, const FileSelectionParams& params,
                                             uint32_t flags)
{
    return CreateFileDialog(CLSID_FileSaveDialog, title, params, (flags | FOS_OVERWRITEPROMPT));
}

void Window::OpenUrl(const std::string& url)
{
    ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void Window::SwitchMode(GameMode mode)
{
    // TODO: check if we are idle.
    // nothing should be loading at this point.

    if (FileLoader::m_DictionaryLoading) {
        SPDLOG_ERROR("m_DictionaryLoading is true!");
        return;
    }

    g_IsJC4Mode = (mode == GameMode::GameMode_JustCause4);

    // select the directory for the current game
    const auto& jc_directory = GetJustCauseDirectory();
    SPDLOG_INFO("Just Cause directory: \"{}\"", jc_directory.string());
    if (jc_directory.empty() || !std::filesystem::exists(jc_directory)) {
        SelectJustCauseDirectory();
    }

    // reset factories
    RuntimeContainer::Instances.clear();
    RenderBlockModel::Instances.clear();
    AvalancheArchive::Instances.clear();
    AvalancheModelFormat::Instances.clear();

    // reload managers
    TextureManager::Get()->Empty();
    ShaderManager::Get()->Init();
    FileLoader::Get()->Init();

    UI::Get()->SwitchToTab(TreeViewTab_FileExplorer);
}

void Window::SelectJustCauseDirectory(bool override_mode, bool jc3_mode)
{
    bool is_jc4_mode = g_IsJC4Mode;
    if (override_mode) {
        is_jc4_mode = !jc3_mode;
    }

    std::string title = is_jc4_mode ? "Please select your Just Cause 4 install folder"
                                    : "Please select your Just Cause 3 install folder";

    const auto& selected = ShowFolderSelection(title);
    if (!selected.empty()) {
        SPDLOG_INFO("Selected directory: \"{}\"", selected.string());

        std::filesystem::path exe_path = selected;
        exe_path /= (is_jc4_mode ? "JustCause4.exe" : "JustCause3.exe");

        if (!std::filesystem::exists(exe_path)) {
            ShowMessageBox("Invalid Just Cause directory specified.\n\nMake sure you select the root directory.");
            return SelectJustCauseDirectory(override_mode, jc3_mode);
        }

        Settings::Get()->SetValue(is_jc4_mode ? "jc4_directory" : "jc3_directory", selected.string().c_str());
    } else {
        const std::string jc_directory =
            Settings::Get()->GetValue<const char*>(is_jc4_mode ? "jc4_directory" : "jc3_directory", "");
        if (jc_directory.empty()) {
            ShowMessageBox("Unable to find Just Cause root directory.\n\nSome features will be disabled.");
        }
    }
}

std::filesystem::path Window::GetJustCauseDirectory()
{
    return Settings::Get()->GetValue<const char*>(g_IsJC4Mode ? "jc4_directory" : "jc3_directory", "");
}

void Window::CheckForUpdates(bool show_no_update_messagebox)
{
    static int32_t current_version[3] = {VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION};

    std::thread([show_no_update_messagebox] {
        try {
            httplib::Client client("kirkh.am");
            const auto      res = client.get("/jc-model-renderer/latest.json");
            if (res && res->status == 200) {
                rapidjson::Document doc;
                doc.Parse(res->body.c_str());

                const auto version_str = std::string(doc["version"].GetString());

                int32_t latest_version[3] = {0};
                std::sscanf(version_str.c_str(), "%d.%d.%d", &latest_version[0], &latest_version[1],
                            &latest_version[2]);

                if (std::lexicographical_compare(current_version, current_version + 3, latest_version,
                                                 latest_version + 3)) {
                    std::string msg = "A new version (" + version_str + ") is available for download.\n\n";
                    msg.append("Do you want to go to the release page on GitHub?");

                    if (Window::Get()->ShowMessageBox(msg, MB_ICONQUESTION | MB_YESNO) == IDYES) {
                        Window::Get()->OpenUrl("https://github.com/aaronkirkham/jc-model-renderer/releases/latest");
                    }
                } else if (show_no_update_messagebox) {
                    Window::Get()->ShowMessageBox("You have the latest version!", MB_ICONINFORMATION);
                }
            }
        } catch (...) {
            SPDLOG_ERROR("Failed to check for updates");
        }
    }).detach();
}

bool Window::LoadInternalResource(int32_t resource_id, std::vector<uint8_t>* out_buffer)
{
    const auto handle = GetModuleHandle(nullptr);
    const auto rc     = FindResource(handle, MAKEINTRESOURCE(resource_id), RT_RCDATA);
    if (rc == nullptr) {
        SPDLOG_ERROR("FindResource failed.");
        return false;
    }

    const auto data = LoadResource(handle, rc);
    if (data == nullptr) {
        SPDLOG_ERROR("LoadResource failed.");
        return false;
    }

    const void* ptr = LockResource(data);

    out_buffer->resize(SizeofResource(handle, rc));
    std::memcpy(out_buffer->data(), ptr, out_buffer->size());
    return true;
}

void Window::ConvertFileSelectionFilters(std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>& converter,
                                         const std::vector<FileSelectionFilter>&                 filters,
                                         std::vector<std::wstring>* buffer, std::vector<COMDLG_FILTERSPEC>* filterspec)
{
    if (filters.empty()) {
        filterspec->push_back(COMDLG_FILTERSPEC{L"All Files (*.*)", L"*.*"});
        return;
    }

    buffer->reserve(filters.size() * 2);
    filterspec->reserve(filters.size());

    for (const auto& filter : filters) {
        COMDLG_FILTERSPEC spec;

        buffer->push_back(converter.from_bytes(filter.first));
        spec.pszName = buffer->back().c_str();

        buffer->push_back(converter.from_bytes(filter.second));
        spec.pszSpec = buffer->back().c_str();

        filterspec->push_back(std::move(spec));
    }
}
