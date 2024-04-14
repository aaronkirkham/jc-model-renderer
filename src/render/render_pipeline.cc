#include "pch.h"

#include "render_pipeline.h"

#include "app/app.h"

#include "render/renderer.h"

namespace jcmr
{
struct RenderPipelineImpl final : RenderPipeline {
    RenderPipelineImpl(App& app)
        : m_app(app)
    {
        m_app.get_renderer().register_pipeline(this);
        //
    }

    ~RenderPipelineImpl()
    {
        m_app.get_renderer().unregister_pipeline(this);
        //
    }

    void init() override
    {
        // create_gbuffers(800, 600);
        //
    }

    void shutdown() override
    {
        // destroy_gbuffers();
        //
    }

    void resize(u32 width, u32 height) override
    {
        m_width  = width;
        m_height = height;
    }

    void render(const RenderContext& context) override
    {
        if (m_callback) {
            m_callback(context);
        }
    }

    void set_render_callback(RenderCallback_t callback)
    {
        ASSERT(m_callback == nullptr);
        m_callback = callback;
    }

  public:
    App&             m_app;
    RenderCallback_t m_callback = nullptr;
    u32              m_width;
    u32              m_height;
};

RenderPipeline* RenderPipeline::create(App& app)
{
    return new RenderPipelineImpl(app);
}

void RenderPipeline::destroy(RenderPipeline* pipeline)
{
    delete pipeline;
}
} // namespace jcmr