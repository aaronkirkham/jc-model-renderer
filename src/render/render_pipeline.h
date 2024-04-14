#ifndef JCMR_RENDER_RENDER_PIPELINE_H_HEADER_GUARD
#define JCMR_RENDER_RENDER_PIPELINE_H_HEADER_GUARD

#include "platform.h"

namespace jcmr
{
struct App;
struct RenderContext;

struct RenderPipeline {
    using RenderCallback_t = std::function<void(const RenderContext&)>;

    static RenderPipeline* create(App& app);
    static void            destroy(RenderPipeline* pipeline);

    virtual ~RenderPipeline() = default;

    virtual void init()                        = 0;
    virtual void shutdown()                    = 0;
    virtual void resize(u32 width, u32 height) = 0;

    virtual void render(const RenderContext& context) = 0;

    virtual void set_render_callback(RenderCallback_t callback) = 0;
};
} // namespace jcmr

#endif // JCMR_RENDER_RENDER_PIPELINE_H_HEADER_GUARD