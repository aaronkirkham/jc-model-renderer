#ifndef JCMR_APP_OS_H_HEADER_GUARD
#define JCMR_APP_OS_H_HEADER_GUARD

#include "platform.h"

namespace jcmr
{
struct App;

namespace os
{
    struct Point {
        i32 x, y;
    };

    struct Size {
        i32 width, height;
    };

    struct Rect {
        i32 left, top, width, height;
    };

    using WindowHandle                           = void*;
    constexpr WindowHandle INVALID_WINDOW_HANDLE = nullptr;

    struct Window {
        WindowHandle handle   = INVALID_WINDOW_HANDLE;
        i32          width    = 0;
        i32          height   = 0;
        bool         resizing = false;
    };

    struct InitWindowArgs {
        enum Flags : u32 {
            NO_DECORATION   = (1 << 0),
            NO_TASKBAR_ICON = (1 << 1),
            NO_RESIZE       = (1 << 2),
            //
            CENTERED          = (1 << 5),
            ACCEPT_DROP_FILES = (1 << 6),
        };

        const char* name     = "";
        Point       position = {0, 0};
        Size        size     = {1280, 720};
        u32         flags    = 0;
        Window*     parent   = nullptr;
    };

    struct Event {
        enum class Type {
            QUIT,
            WINDOW_CLOSE,
            WINDOW_MOVE,
            WINDOW_SIZE,
            WINDOW_FOCUS,
        };

        Type    type;
        Window* window;
        union {
            struct {
                i32 x, y;
            } window_move;

            struct {
                i32 width, height;
            } window_size;

            struct {
                bool gained;
            } window_focus;
        };
    };

    struct FileDialogFilter {
        char name[256] = {0};
        char spec[256] = {0};
    };

    struct FileDialogParams {
        char                          filename[MAX_PATH_LENGTH] = {0};
        char                          extension[10]             = {0};
        std::vector<FileDialogFilter> filters;
    };

    void init(App& app);
    void shutdown();

    Window* create_window(const InitWindowArgs& init_window_args);
    void    destroy_window(Window* window);
    void    quit();

    void set_window_title(Window* window, const char* title);

    void set_window_rect(Window* window, const Rect& rect);
    Rect get_window_rect(Window* window);

    i32 get_dpi();

    u64 get_tick_count();

    ByteArray map_view_of_file(const char* filename, u64 offset, u64 size);

    bool get_open_file(std::filesystem::path* out_path, FileDialogParams& params);
    bool get_open_folder(std::filesystem::path* out_path, FileDialogParams& params);
} // namespace os
} // namespace jcmr

#endif // JCMR_APP_OS_H_HEADER_GUARD
