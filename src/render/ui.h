#ifndef JCMR_RENDER_UI_H_HEADER_GUARD
#define JCMR_RENDER_UI_H_HEADER_GUARD

#include "app/os.h"

struct ImFont;

namespace jcmr
{
struct App;
struct Renderer;
struct RenderContext;

namespace game
{
    struct IFormat;
}

struct UI {
    using RenderCallback_t = std::function<void(RenderContext& context)>;

    enum ContextMenuFlags : u32 {
        E_CONTEXT_FOLDER = (1 << 0),
        E_CONTEXT_FILE   = (1 << 1),
    };

    enum DockSpacePosition : u32 {
        E_DOCKSPACE_LEFT         = 0,
        E_DOCKSPACE_RIGHT        = 1,
        E_DOCKSPACE_RIGHT_BOTTOM = 2,
    };

    static UI*  create(App& app, Renderer& renderer);
    static void destroy(UI* instance);

    virtual ~UI() = default;

    virtual void init(os::WindowHandle window_handle) = 0;
    virtual void shutdown()                           = 0;

    virtual void start_frame()                  = 0;
    virtual void render(RenderContext& context) = 0;

    virtual void draw_context_menu(const std::string& filename, game::IFormat* format, u32 flags) = 0;

    virtual void          on_render(RenderCallback_t callback) = 0;
    virtual const ImFont* get_large_font() const               = 0;

    virtual const u32 get_dockspace_id(DockSpacePosition position) const = 0;
};
} // namespace jcmr

#endif // JCMR_RENDER_UI_H_HEADER_GUARD
