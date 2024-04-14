#include "pch.h"

#include "render_pipeline.h"

#include "app/app.h"

#include "render/camera.h"
#include "render/renderer.h"

#include <d3d11.h>

namespace jcmr
{
struct RenderPipelineImpl final : RenderPipeline {
    RenderPipelineImpl(App& app)
        : m_app(app)
    {
        m_camera = Camera::create();
        m_app.get_renderer().register_pipeline(this);
    }

    ~RenderPipelineImpl()
    {
        Camera::destroy(m_camera);
        m_app.get_renderer().unregister_pipeline(this);
    }

    void init(u32 width, u32 height) override
    {
        LOG_INFO("RenderPipeline::init({}, {})", width, height);

        // TODO : we do indeed want to move the gbuffer stuff into here, as we'll need to support rendering multiple
        //        views at once (each may contain it's own scene to render). That means we'll need the 4 g-buffer
        //        textures, and the depth texture in here.

        // We'll also want to improve slightly on rendering, and call the deferred combine shader, then combine the
        // inputs into a single output which will be the final image we render. That _should_ allow us to control
        // lighting.

        update_viewport(width, height);
        create_gbuffers(width, height);
        create_depth_texture(width, height);

        m_initialised = true;
    }

    void shutdown() override
    {
        destroy_gbuffers();
        destroy_depth_texture();
    }

    void resize(u32 width, u32 height) override
    {
        m_width  = width;
        m_height = height;

        destroy_gbuffers();
        destroy_depth_texture();

        update_viewport(width, height);
        create_gbuffers(width, height);
        create_depth_texture(width, height);
    }

    void clear(const float* colour) const override
    {
        const auto& context = m_app.get_renderer().get_context();

        if (m_depth_stencil_view) {
            context.device_context->ClearDepthStencilView(m_depth_stencil_view,
                                                          (D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL), 1.0f, 0);
        }

        // gbuffers
        for (auto* render_target : m_gbuffer) {
            if (!render_target) continue;
            context.device_context->ClearRenderTargetView(render_target, colour);
        }
    }

    void render(RenderContext& context) const override
    {
        if (!m_initialised) return;

        m_camera->update(context);

        context.device_context->RSSetViewports(1, &m_viewport);

        context.renderer->set_depth_enabled(true);
        context.device_context->OMSetRenderTargets(m_gbuffer.size(), &m_gbuffer[0], m_depth_stencil_view);

        if (m_callback) {
            m_callback(context);
        }
    }

    void set_render_callback(RenderCallback_t callback)
    {
        ASSERT(m_callback == nullptr);
        m_callback = callback;
    }

    Camera& get_camera() const override { return *m_camera; }

    const ID3D11ShaderResourceView* get_framebuffer(uint8_t index) const override
    {
        ASSERT(index <= m_gbuffer_srv.size()); // TODO : static_assert
        return m_gbuffer_srv[index];
    }

  private:
    void update_viewport(u32 width, u32 height)
    {
        m_viewport.Width    = width;
        m_viewport.Height   = height;
        m_viewport.MaxDepth = 1.0f;

        m_camera->resize(width, height);
    }

    void create_render_target(u32 width, u32 height, DXGI_FORMAT format, ID3D11RenderTargetView** out_render_target,
                              ID3D11ShaderResourceView** out_srv, const char* debug_name)
    {
        const auto& renderer = m_app.get_renderer();
        const auto& context  = renderer.get_context();

        auto  _tex_debug_name = fmt::format("Renderer GBuffer {} Texture", debug_name);
        auto* texture         = renderer.create_texture(
            width, height, format, (D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE), _tex_debug_name.c_str());

        auto hr = context.device->CreateRenderTargetView(texture, nullptr, out_render_target);
        ASSERT(SUCCEEDED(hr));

        D3D11_SHADER_RESOURCE_VIEW_DESC desc{};
        desc.Format        = format;
        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        desc.Texture2D     = {0, (UINT)-1};
        hr                 = context.device->CreateShaderResourceView(texture, &desc, out_srv);
        ASSERT(SUCCEEDED(hr));

        if (texture) {
            texture->Release();
            texture = nullptr;
        }

#ifdef RENDERER_REPORT_LIVE_OBJECTS
        auto _rt_debug_name = fmt::format("Renderer GBuffer {}", debug_name);
        D3D_SET_OBJECT_NAME_A(*out_render_target, _rt_debug_name.c_str());

        auto _srv_debug_name = fmt::format("Renderer GBuffer {} SRV", debug_name);
        D3D_SET_OBJECT_NAME_A(*out_srv, _srv_debug_name.c_str());
#endif
    }

    void create_gbuffers(u32 width, u32 height)
    {
        create_render_target(width, height, DXGI_FORMAT_R8G8B8A8_UNORM, &m_gbuffer[0], &m_gbuffer_srv[0], "Diffuse");
        create_render_target(width, height, DXGI_FORMAT_R10G10B10A2_UNORM, &m_gbuffer[1], &m_gbuffer_srv[1], "Normal");
        create_render_target(width, height, DXGI_FORMAT_R8G8B8A8_UNORM, &m_gbuffer[2], &m_gbuffer_srv[2], "Properties");
        create_render_target(width, height, DXGI_FORMAT_R8G8B8A8_UNORM, &m_gbuffer[3], &m_gbuffer_srv[3],
                             "PropertiesEx");
    }

    void destroy_gbuffers()
    {
        for (auto* render_target_srv : m_gbuffer_srv) {
            if (render_target_srv) {
                render_target_srv->Release();
                render_target_srv = nullptr;
            }
        }

        for (auto* render_target : m_gbuffer) {
            if (render_target) {
                render_target->Release();
                render_target = nullptr;
            }
        }
    }

    void create_depth_texture(u32 width, u32 height)
    {
        const auto& renderer = m_app.get_renderer();
        const auto& context  = renderer.get_context();

        // create the depth buffer texture
        m_depth_texture = renderer.create_texture(width, height, DXGI_FORMAT_R24G8_TYPELESS,
                                                  (D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE),
                                                  "Renderer depth stencil texture");

        // create depth stencil view
        D3D11_DEPTH_STENCIL_VIEW_DESC view_desc;
        view_desc.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
        view_desc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
        view_desc.Texture2D.MipSlice = 0;
        view_desc.Flags              = 0;

        const bool result = context.device->CreateDepthStencilView(m_depth_texture, &view_desc, &m_depth_stencil_view);
        ASSERT(SUCCEEDED(result));

#ifdef RENDERER_REPORT_LIVE_OBJECTS
        D3D_SET_OBJECT_NAME_A(m_depth_stencil_view, "Renderer depth stencil view");
#endif
    }

    void destroy_depth_texture()
    {
        if (m_depth_stencil_view) {
            m_depth_stencil_view->Release();
            m_depth_stencil_view = nullptr;
        }

        if (m_depth_texture) {
            m_depth_texture->Release();
            m_depth_texture = nullptr;
        }
    }

  private:
    App&             m_app;
    RenderCallback_t m_callback    = nullptr;
    bool             m_initialised = false;
    u32              m_width;
    u32              m_height;
    Camera*          m_camera = nullptr;

    D3D11_VIEWPORT                           m_viewport{};
    std::array<ID3D11RenderTargetView*, 4>   m_gbuffer            = {nullptr};
    std::array<ID3D11ShaderResourceView*, 4> m_gbuffer_srv        = {nullptr};
    ID3D11Texture2D*                         m_depth_texture      = nullptr;
    ID3D11DepthStencilView*                  m_depth_stencil_view = nullptr;
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