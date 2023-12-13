#include "pch.h"

#include "renderer.h"

#include "app/app.h"
#include "app/directory_list.h"

#include "game/game.h"
#include "game/render_block.h"
#include "game/resource_manager.h"

#include "render/camera.h"
#include "render/dds_texture_loader.h"
#include "render/shader.h"
#include "render/texture.h"
#include "render/ui.h"

#include <d3d11.h>
#include <imgui.h>

#define SAFE_RELEASE(ptr)                                                                                              \
    if (ptr) {                                                                                                         \
        ptr->Release();                                                                                                \
        ptr = nullptr;                                                                                                 \
    }

namespace jcmr
{
struct RendererImpl final : Renderer {
    RendererImpl(App& app)
        : m_app(app)
    {
        m_ui     = UI::create(app, *this);
        m_camera = Camera::create(*this);

        // render output buffer
        m_ui->on_render([&](RenderContext&) {
            auto dock_id = m_ui->get_dockspace_id(UI::E_DOCKSPACE_RIGHT);
            ImGui::SetNextWindowDockID(dock_id, ImGuiCond_Appearing);

            if (ImGui::Begin("Output", nullptr)) {

                auto avail = ImGui::GetContentRegionAvail();

                ImGui::Image(m_gbuffer_srv[0], {avail.x, 600});
                ImGui::Image(m_gbuffer_srv[1], {avail.x, 600});
                ImGui::Image(m_gbuffer_srv[2], {avail.x, 600});
                ImGui::Image(m_gbuffer_srv[3], {avail.x, 600});
            }

            ImGui::End();
        });
    }

    ~RendererImpl()
    {
        Camera::destroy(m_camera);
        UI::destroy(m_ui);
    }

    bool init(const InitRendererArgs& init_renderer_args) override
    {
        create_device(init_renderer_args);
        create_depth_stencil(init_renderer_args.width, init_renderer_args.height);
        create_render_target(init_renderer_args.width, init_renderer_args.height);
        create_gbuffers(init_renderer_args.width, init_renderer_args.height);
        create_blend_state();
        create_rasterizer_state();

        m_context.renderer       = this;
        m_context.device         = m_device;
        m_context.device_context = m_device_context;

        m_ui->init(init_renderer_args.window_handle);
        return true;
    }

    void shutdown() override
    {
        m_ui->shutdown();

        m_context.device         = nullptr;
        m_context.device_context = nullptr;

        if (m_swap_chain) {
            m_swap_chain->SetFullscreenState(false, nullptr);
        }

        m_device_context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
        m_device_context->RSSetState(nullptr);

        destroy_render_target();
        destroy_gbuffers();
        destroy_depth_stencil();

        SAFE_RELEASE(m_rasterizer_state);
        SAFE_RELEASE(m_blend_state);
        SAFE_RELEASE(m_device_context);
        SAFE_RELEASE(m_device);
        SAFE_RELEASE(m_swap_chain);

#ifdef _DEBUG
#ifdef RENDERER_REPORT_LIVE_OBJECTS
        m_debugger->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
#endif
        SAFE_RELEASE(m_debugger);
#endif
    }

    void resize(u32 width, u32 height) override
    {
        if (!m_device || !m_device_context) return;
        if (width == 0 || height == 0) return;

        destroy_render_target();
        destroy_gbuffers();
        destroy_depth_stencil();

        m_swap_chain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);

        create_depth_stencil(width, height);
        create_gbuffers(width, height);
        create_render_target(width, height);
    }

    std::shared_ptr<Texture> create_texture(const std::string& filename, const ByteArray& buffer) override
    {
        if (auto texture = get_texture(filename); texture) {
            return texture;
        }

        auto texture = m_texture_cache[filename].lock();
        if (!texture) {
            texture = std::shared_ptr<Texture>(Texture::create(filename, *this));
            // TODO : should a texture load fail prevent us caching? probably..
            texture->load(buffer);
            m_texture_cache[filename] = texture;
        }

        return texture;
    }

    std::shared_ptr<Texture> get_texture(const std::string& filename) override
    {
        auto texture = m_texture_cache[filename];
        return texture.lock();
    }

    std::shared_ptr<Shader> create_shader(const std::string& filename, u8 shader_type, const ByteArray& buffer) override
    {
        auto filename_with_extension = (filename + Shader::shader_extensions[shader_type]);
        if (auto shader = get_shader(filename_with_extension); shader) {
            return shader;
        }

        auto shader = m_shader_cache[filename_with_extension].lock();
        if (!shader) {
            shader = std::shared_ptr<Shader>(Shader::create(filename, (Shader::ShaderType)shader_type, *this));
            // TODO : should a shader load fail prevent us caching? probably..
            shader->load(buffer);
            m_shader_cache[filename_with_extension] = shader;
        }

        return shader;
    }

    std::shared_ptr<Shader> get_shader(const std::string& filename) override
    {
        auto shader = m_shader_cache[filename].lock();
        return shader;
    }

    ID3D11Texture2D* create_texture(u32 width, u32 height, u32 format, u32 bind_flags, const char* debug_name) override
    {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width              = width;
        desc.Height             = height;
        desc.MipLevels          = 1;
        desc.ArraySize          = 1;
        desc.Format             = static_cast<DXGI_FORMAT>(format);
        desc.SampleDesc.Count   = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage              = D3D11_USAGE_DEFAULT;
        desc.BindFlags          = bind_flags;
        desc.CPUAccessFlags     = 0;
        desc.MiscFlags          = 0;

        ID3D11Texture2D* texture = nullptr;
        auto             result  = m_device->CreateTexture2D(&desc, nullptr, &texture);

#ifdef RENDERER_REPORT_LIVE_OBJECTS
        if (texture && debug_name) D3D_SET_OBJECT_NAME_A(texture, debug_name);
#endif

        return texture;
    }

    Sampler* create_sampler(const D3D11_SAMPLER_DESC& desc, const char* debug_name) override
    {
        ID3D11SamplerState* sampler = nullptr;
        auto                hr      = m_device->CreateSamplerState(&desc, &sampler);
        ASSERT(SUCCEEDED(hr));

#ifdef RENDERER_REPORT_LIVE_OBJECTS
        D3D_SET_OBJECT_NAME_A(sampler, debug_name);
#endif

        return sampler;
    }

    Buffer* create_buffer(const void* data, u32 count, u32 stride, u32 type, const char* debug_name) override
    {
        auto buffer    = new Buffer;
        buffer->count  = count;
        buffer->stride = stride;
        buffer->data.resize(count * stride);
        std::memcpy(buffer->data.data(), data, buffer->data.size());

        D3D11_BUFFER_DESC desc{};
        desc.Usage     = D3D11_USAGE_DEFAULT;
        desc.ByteWidth = buffer->data.size();
        desc.BindFlags = type;

        D3D11_SUBRESOURCE_DATA subresource_data{};
        subresource_data.pSysMem = data;

        auto hr = m_device->CreateBuffer(&desc, &subresource_data, &buffer->ptr);
#ifdef RENDERER_REPORT_LIVE_OBJECTS
        if (SUCCEEDED(hr) && debug_name) D3D_SET_OBJECT_NAME_A(buffer->ptr, debug_name);
#endif

        if (FAILED(hr)) {
            delete buffer;
            return nullptr;
        }

        return buffer;
    }

    Buffer* create_constant_buffer(const void* data, u32 vec4count, const char* debug_name = nullptr) override
    {
        auto buffer    = new Buffer;
        buffer->count  = 1;
        buffer->stride = (16 * vec4count);

        D3D11_BUFFER_DESC desc{};
        desc.Usage          = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth      = buffer->stride;
        desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        D3D11_SUBRESOURCE_DATA subresource_data{};
        subresource_data.pSysMem = data;

        auto hr = m_device->CreateBuffer(&desc, &subresource_data, &buffer->ptr);
#ifdef RENDERER_REPORT_LIVE_OBJECTS
        if (SUCCEEDED(hr) && debug_name) D3D_SET_OBJECT_NAME_A(buffer->ptr, debug_name);
#endif

        if (FAILED(hr)) {
            delete buffer;
            return nullptr;
        }

        return buffer;
    }

    bool map_buffer(Buffer* buffer, const void* data, u64 size) override
    {
        D3D11_MAPPED_SUBRESOURCE mapping{};
        if (FAILED(m_device_context->Map(buffer->ptr, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapping))) {
            return false;
        }

        std::memcpy(mapping.pData, data, size);
        m_device_context->Unmap(buffer->ptr, 0);
        return true;
    }

    void destroy_buffer(Buffer* buffer) override
    {
        if (buffer) {
            if (buffer->ptr) buffer->ptr->Release();
            delete buffer;
        }
    }

    // TODO : might be nice to move this into the Buffer class itself.
    //        Buffer::map(data, size) would be a nice API.
    void set_vertex_shader_constants(Buffer* buffer, u32 slot, const void* data, u64 size) override
    {
        ASSERT(buffer);
        ASSERT(buffer->ptr);

        if (map_buffer(buffer, data, size)) {
            m_device_context->VSSetConstantBuffers(slot, 1, &buffer->ptr);
        }
    }

    // TODO : might be nice to move this into the Buffer class itself.
    //        Buffer::map(data, size) would be a nice API.
    void set_pixel_shader_constants(Buffer* buffer, u32 slot, const void* data, u64 size) override
    {
        ASSERT(buffer);
        ASSERT(buffer->ptr);

        if (map_buffer(buffer, data, size)) {
            m_device_context->PSSetConstantBuffers(slot, 1, &buffer->ptr);
        }
    }

    void set_vertex_stream(Buffer* buffer, i32 slot, u32 offset) override
    {
        ASSERT(buffer);
        ASSERT(buffer->ptr);

        m_device_context->IASetVertexBuffers(slot, 1, &buffer->ptr, &buffer->stride, &offset);
    }

    void set_texture(Texture* texture, i32 slot, Sampler* sampler) override
    {
        ID3D11ShaderResourceView* srv = nullptr;
        if (texture) srv = texture->get_srv();

        m_device_context->PSSetShaderResources(slot, 1, &srv);
        if (sampler) m_device_context->PSSetSamplers(slot, 1, &sampler);
    }

    void set_sampler(Sampler* sampler, i32 slot) override
    {
        ASSERT(sampler);
        m_device_context->PSSetSamplers(slot, 1, &sampler);
    }

    void set_depth_enabled(bool state) override
    {
        m_device_context->OMSetDepthStencilState(state ? m_depth_stencil_enabled_state : m_depth_stencil_disabled_state,
                                                 1);
    }

    void set_cull_mode(D3D11_CULL_MODE mode) override
    {
        if (m_context.cull_mode == mode) return;

        m_context.cull_mode                 = mode;
        m_context.rasterizer_state_is_dirty = true;
    }

    void set_fill_mode(D3D11_FILL_MODE mode) override
    {
        if (m_context.fill_mode == mode) return;

        m_context.fill_mode                 = mode;
        m_context.rasterizer_state_is_dirty = true;
    }

    void draw(u32 offset, u32 count) override
    {
        setup_render_states();
        m_device_context->Draw(count, offset);
    }

    void draw_indexed(u32 start_index, u32 count) override
    {
        setup_render_states();
        m_device_context->DrawIndexed(count, start_index, 0);
    }

    void draw_indexed(Buffer* buffer, u32 start_index) override
    {
        //
        draw_indexed(buffer, start_index, buffer->count);
    }

    void draw_indexed(Buffer* buffer, u32 start_index, u32 count) override
    {
        ASSERT(buffer);
        ASSERT(buffer->ptr);

        setup_render_states();
        m_device_context->IASetIndexBuffer(buffer->ptr, DXGI_FORMAT_R16_UINT, 0);
        m_device_context->DrawIndexed(count, start_index, 0);
    }

    void add_to_renderlist(const std::vector<game::IRenderBlock*>& render_blocks) override
    {
        std::copy(render_blocks.begin(), render_blocks.end(), std::back_inserter(m_render_list));

        // sort by opaque items, so that transparent blocks will be under them
        // std::sort(m_render_list.begin(), m_render_list.end(),
        //           [](game::IRenderBlock* lhs, game::IRenderBlock* rhs) { return lhs->IsOpaque() > rhs->IsOpaque();
        //           });
    }

    void remove_from_renderlist(const std::vector<game::IRenderBlock*>& render_blocks) override
    {
        for (const auto& render_block : render_blocks) {
            m_render_list.erase(std::remove(m_render_list.begin(), m_render_list.end(), render_block),
                                m_render_list.end());
        }
    }

    void frame() override
    {
        if (!m_device || !m_device_context || !m_swap_chain) {
            return;
        }

        // m_context.dt = dt;

        m_ui->start_frame();

        // clear buffers
        {
            float col[4] = {0, 0, 0, 255};
            m_device_context->ClearRenderTargetView(m_back_buffer, col);
            m_device_context->ClearDepthStencilView(m_depth_stencil_view, (D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL),
                                                    1.0f, 0);

            // clear gbuffers
            for (auto* render_target : m_gbuffer) {
                if (render_target) {
                    m_device_context->ClearRenderTargetView(render_target, col);
                }
            }
        }

        // update camera
        m_camera->update(m_context);

        // global constants
        // TODO

        // render geometry
        {
            set_depth_enabled(true);
            m_device_context->OMSetRenderTargets(m_gbuffer.size(), &m_gbuffer[0], m_depth_stencil_view);

            m_app.get_game().setup_render_constants(m_context);

            // draw render lsits
            for (auto* render_block : m_render_list) {
                if (render_block->setup(m_context)) {
                    render_block->draw(m_context);
                    set_default_render_state();
                }
            }
        }

        // render ui
        {
            set_depth_enabled(false);
            m_device_context->OMSetRenderTargets(1, &m_back_buffer, m_depth_stencil_view);
            m_ui->render(m_context);
        }

        m_swap_chain->Present(1, 0);
    }

    const RenderContext& get_context() const override { return m_context; }
    UI&                  get_ui() override { return *m_ui; }
    Camera&              get_camera() override { return *m_camera; }

  private:
    void create_device(const InitRendererArgs& init_renderer_args)
    {
        DXGI_SWAP_CHAIN_DESC desc{};
        desc.BufferCount                        = 1;
        desc.BufferDesc.Width                   = init_renderer_args.width;
        desc.BufferDesc.Height                  = init_renderer_args.height;
        desc.BufferDesc.RefreshRate.Numerator   = 60;
        desc.BufferDesc.RefreshRate.Denominator = 1;
        desc.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.BufferDesc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        desc.BufferDesc.Scaling                 = DXGI_MODE_SCALING_UNSPECIFIED;
        desc.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.OutputWindow                       = static_cast<HWND>(init_renderer_args.window_handle);
        desc.SampleDesc.Count                   = 1;
        desc.SampleDesc.Quality                 = 0;
        desc.Windowed                           = true;
        desc.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;
        desc.Flags                              = 0;

        u32 device_flags = 0;
#ifdef _DEBUG
        device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        auto result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, device_flags, nullptr,
                                                    0, D3D11_SDK_VERSION, &desc, &m_swap_chain, &m_device, nullptr,
                                                    &m_device_context);
        ASSERT(SUCCEEDED(result));

#ifdef _DEBUG
        result = m_device->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&m_debugger));
        ASSERT(SUCCEEDED(result));

        ID3D11InfoQueue* info_queue;
        result = m_debugger->QueryInterface(__uuidof(ID3D11InfoQueue), reinterpret_cast<void**>(&info_queue));

        // hide specific messages
        D3D11_MESSAGE_ID messages_to_hide[] = {D3D11_MESSAGE_ID_DEVICE_DRAW_SAMPLER_NOT_SET};

        D3D11_INFO_QUEUE_FILTER filter{};
        filter.DenyList.NumIDs  = _countof(messages_to_hide);
        filter.DenyList.pIDList = messages_to_hide;
        info_queue->AddStorageFilterEntries(&filter);
        info_queue->Release();
#endif
    }

    void create_depth_stencil(u32 width, u32 height)
    {
        D3D11_DEPTH_STENCIL_DESC desc{};
        desc.DepthEnable                  = true;
        desc.DepthWriteMask               = D3D11_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc                    = D3D11_COMPARISON_LESS;
        desc.StencilEnable                = false;
        desc.FrontFace.StencilFunc        = D3D11_COMPARISON_ALWAYS;
        desc.FrontFace.StencilPassOp      = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
        desc.BackFace.StencilFunc         = D3D11_COMPARISON_ALWAYS;
        desc.BackFace.StencilPassOp       = D3D11_STENCIL_OP_KEEP;
        desc.BackFace.StencilFailOp       = D3D11_STENCIL_OP_KEEP;
        desc.BackFace.StencilDepthFailOp  = D3D11_STENCIL_OP_DECR;

        auto result = m_device->CreateDepthStencilState(&desc, &m_depth_stencil_enabled_state);
        ASSERT(SUCCEEDED(result));

#ifdef RENDERER_REPORT_LIVE_OBJECTS
        D3D_SET_OBJECT_NAME_A(m_depth_stencil_enabled_state, "Renderer depth stencil enabled state");
#endif

        desc.DepthEnable    = false;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

        result = m_device->CreateDepthStencilState(&desc, &m_depth_stencil_disabled_state);
        ASSERT(SUCCEEDED(result));

#ifdef RENDERER_REPORT_LIVE_OBJECTS
        D3D_SET_OBJECT_NAME_A(m_depth_stencil_disabled_state, "Renderer depth stencil disabled state");
#endif

        // enable depth
        m_device_context->OMSetDepthStencilState(m_depth_stencil_enabled_state, 1);

        // create the depth buffer texture
        m_depth_texture =
            create_texture(width, height, DXGI_FORMAT_R24G8_TYPELESS,
                           (D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE), "Renderer depth stencil texture");

        // create depth stencil view
        D3D11_DEPTH_STENCIL_VIEW_DESC view_desc;
        view_desc.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
        view_desc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
        view_desc.Texture2D.MipSlice = 0;
        view_desc.Flags              = 0;

        result = m_device->CreateDepthStencilView(m_depth_texture, &view_desc, &m_depth_stencil_view);
        ASSERT(SUCCEEDED(result));

#ifdef RENDERER_REPORT_LIVE_OBJECTS
        D3D_SET_OBJECT_NAME_A(m_depth_stencil_view, "Renderer depth stencil view");
#endif
    }

    void destroy_depth_stencil()
    {
        m_device_context->OMSetDepthStencilState(nullptr, 0);

        SAFE_RELEASE(m_depth_stencil_enabled_state);
        SAFE_RELEASE(m_depth_stencil_disabled_state);
        SAFE_RELEASE(m_depth_stencil_view);
        SAFE_RELEASE(m_depth_texture);
    }

    void create_render_target(u32 width, u32 height)
    {
        ID3D11Texture2D* back_buffer;
        auto result = m_swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back_buffer));
        ASSERT(SUCCEEDED(result));

        result = m_device->CreateRenderTargetView(back_buffer, nullptr, &m_back_buffer);
        ASSERT(SUCCEEDED(result));

        back_buffer->Release();

#ifdef RENDERER_REPORT_LIVE_OBJECTS
        D3D_SET_OBJECT_NAME_A(m_back_buffer, "Renderer back buffer render target");
#endif

        m_device_context->OMSetRenderTargets(1, &m_back_buffer, m_depth_stencil_view);

        D3D11_VIEWPORT viewport{};
        viewport.Width    = width;
        viewport.Height   = height;
        viewport.MaxDepth = 1.0f;
        m_device_context->RSSetViewports(1, &viewport);
    }

    void destroy_render_target()
    {
        static ID3D11RenderTargetView* null_targets[5] = {nullptr};
        m_device_context->OMSetRenderTargets(lengthOf(null_targets), null_targets, nullptr);

        // free back buffer
        SAFE_RELEASE(m_back_buffer);
    }

    void create_render_target_and_srv(u32 width, u32 height, DXGI_FORMAT format,
                                      ID3D11RenderTargetView** out_render_target, ID3D11ShaderResourceView** out_srv,
                                      const char* debug_name)
    {
        auto _tex_debug_name = fmt::format("Renderer GBuffer {} Texture", debug_name);
        auto _rt_debug_name  = fmt::format("Renderer GBuffer {}", debug_name);
        auto _srv_debug_name = fmt::format("Renderer GBuffer {} SRV", debug_name);

        auto texture = create_texture(width, height, format, (D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE),
                                      _tex_debug_name.c_str());

        auto hr = m_device->CreateRenderTargetView(texture, nullptr, out_render_target);
        ASSERT(SUCCEEDED(hr));

        D3D11_SHADER_RESOURCE_VIEW_DESC desc{};
        desc.Format        = format;
        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        desc.Texture2D     = {0, (UINT)-1};
        hr                 = m_device->CreateShaderResourceView(texture, &desc, out_srv);
        ASSERT(SUCCEEDED(hr));

        SAFE_RELEASE(texture);

#ifdef RENDERER_REPORT_LIVE_OBJECTS
        D3D_SET_OBJECT_NAME_A(*out_render_target, _rt_debug_name.c_str());
        D3D_SET_OBJECT_NAME_A(*out_srv, _srv_debug_name.c_str());
#endif
    }

    void create_gbuffers(u32 width, u32 height)
    {

        create_render_target_and_srv(width, height, DXGI_FORMAT_R8G8B8A8_UNORM, &m_gbuffer[0], &m_gbuffer_srv[0],
                                     "Diffuse");
        create_render_target_and_srv(width, height, DXGI_FORMAT_R10G10B10A2_UNORM, &m_gbuffer[1], &m_gbuffer_srv[1],
                                     "Normal");
        create_render_target_and_srv(width, height, DXGI_FORMAT_R8G8B8A8_UNORM, &m_gbuffer[2], &m_gbuffer_srv[2],
                                     "Properties");
        create_render_target_and_srv(width, height, DXGI_FORMAT_R8G8B8A8_UNORM, &m_gbuffer[3], &m_gbuffer_srv[3],
                                     "PropertiesEx");
    }

    void destroy_gbuffers()
    {
        for (auto* render_target_srv : m_gbuffer_srv)
            SAFE_RELEASE(render_target_srv);

        for (auto* render_target : m_gbuffer)
            SAFE_RELEASE(render_target);
    }

    void create_blend_state()
    {
        if (m_context.blend_state_is_dirty) {
            SAFE_RELEASE(m_blend_state);

            D3D11_BLEND_DESC desc{};
            desc.AlphaToCoverageEnable                 = false;
            desc.RenderTarget[0].BlendEnable           = m_context.alpha_blend_enabled;
            desc.RenderTarget[0].SrcBlend              = m_context.blend_source_colour;
            desc.RenderTarget[0].DestBlend             = m_context.blend_dest_colour;
            desc.RenderTarget[0].BlendOp               = m_context.blend_colour_eq;
            desc.RenderTarget[0].SrcBlendAlpha         = m_context.blend_source_alpha;
            desc.RenderTarget[0].DestBlendAlpha        = m_context.blend_dest_alpha;
            desc.RenderTarget[0].BlendOpAlpha          = m_context.blend_alpha_eq;
            desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

            auto hr = m_device->CreateBlendState(&desc, &m_blend_state);
            ASSERT(SUCCEEDED(hr));

#ifdef RENDERER_REPORT_LIVE_OBJECTS
            D3D_SET_OBJECT_NAME_A(m_blend_state, "Renderer Blend State");
#endif

            m_device_context->OMSetBlendState(m_blend_state, nullptr, 0xFFFFFFFF);
            m_context.blend_state_is_dirty = false;
        }
    }

    void create_rasterizer_state()
    {
        if (m_context.rasterizer_state_is_dirty) {
            SAFE_RELEASE(m_rasterizer_state);

            D3D11_RASTERIZER_DESC desc{};
            desc.FillMode        = m_context.fill_mode;
            desc.CullMode        = m_context.cull_mode;
            desc.DepthClipEnable = true;

            auto hr = m_device->CreateRasterizerState(&desc, &m_rasterizer_state);
            ASSERT(SUCCEEDED(hr));

#ifdef RENDERER_REPORT_LIVE_OBJECTS
            D3D_SET_OBJECT_NAME_A(m_rasterizer_state, "Renderer Rasterizer State");
#endif

            m_device_context->RSSetState(m_rasterizer_state);
            m_context.rasterizer_state_is_dirty = false;
        }
    }

    void setup_render_states()
    {
        create_blend_state();
        create_rasterizer_state();
    }

    void set_default_render_state()
    {
        // reset all shader resources
        static ID3D11ShaderResourceView* null_srvs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {nullptr};
        m_device_context->PSSetShaderResources(0, lengthOf(null_srvs), null_srvs);

        // reset all samplers
        static ID3D11SamplerState* null_samplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT] = {nullptr};
        m_device_context->PSSetSamplers(0, lengthOf(null_samplers), null_samplers);

        // reset blending states
        // TODO
    }

  private:
    App&          m_app;
    UI*           m_ui     = nullptr;
    Camera*       m_camera = nullptr;
    RenderContext m_context;

    ID3D11Device*                            m_device         = nullptr;
    ID3D11DeviceContext*                     m_device_context = nullptr;
    IDXGISwapChain*                          m_swap_chain     = nullptr;
    ID3D11RenderTargetView*                  m_back_buffer    = nullptr;
    std::array<ID3D11RenderTargetView*, 4>   m_gbuffer        = {nullptr};
    std::array<ID3D11ShaderResourceView*, 4> m_gbuffer_srv    = {nullptr};

    ID3D11Texture2D*         m_depth_texture                = nullptr;
    ID3D11DepthStencilState* m_depth_stencil_enabled_state  = nullptr;
    ID3D11DepthStencilState* m_depth_stencil_disabled_state = nullptr;
    ID3D11DepthStencilView*  m_depth_stencil_view           = nullptr;
    ID3D11BlendState*        m_blend_state                  = nullptr;
    ID3D11RasterizerState*   m_rasterizer_state             = nullptr;

#ifdef _DEBUG
    ID3D11Debug* m_debugger = nullptr;
#endif

    std::unordered_map<std::string, std::weak_ptr<Texture>> m_texture_cache;
    std::unordered_map<std::string, std::weak_ptr<Shader>>  m_shader_cache;

    std::vector<game::IRenderBlock*> m_render_list;
};

Renderer* Renderer::create(App& app)
{
    return new RendererImpl(app);
}

void Renderer::destroy(Renderer* renderer)
{
    delete renderer;
}

DDS_PIXELFORMAT Renderer::get_pixel_format(DXGI_FORMAT dxgi_format)
{
    DDS_PIXELFORMAT pixel_format{sizeof(DDS_PIXELFORMAT)};

    switch (dxgi_format) {
        case DXGI_FORMAT_BC1_UNORM: {
            pixel_format.flags       = DDS_FOURCC;
            pixel_format.RGBBitCount = 0;
            pixel_format.RBitMask    = 0;
            pixel_format.GBitMask    = 0;
            pixel_format.BBitMask    = 0;
            pixel_format.ABitMask    = 0;
            pixel_format.fourCC      = 0x31545844; // DXT1
            break;
        }

        case DXGI_FORMAT_BC2_UNORM: {
            pixel_format.flags       = DDS_FOURCC;
            pixel_format.RGBBitCount = 0;
            pixel_format.RBitMask    = 0;
            pixel_format.GBitMask    = 0;
            pixel_format.BBitMask    = 0;
            pixel_format.ABitMask    = 0;
            pixel_format.fourCC      = 0x33545844; // DXT3
            break;
        }

        case DXGI_FORMAT_BC3_UNORM: {
            pixel_format.flags       = DDS_FOURCC;
            pixel_format.RGBBitCount = 0;
            pixel_format.RBitMask    = 0;
            pixel_format.GBitMask    = 0;
            pixel_format.BBitMask    = 0;
            pixel_format.ABitMask    = 0;
            pixel_format.fourCC      = 0x35545844; // DXT5
            break;
        }

        case DXGI_FORMAT_B8G8R8A8_UNORM: {
            pixel_format.flags       = (DDS_RGB | DDS_ALPHA);
            pixel_format.RGBBitCount = 32; // A8B8G8R8
            pixel_format.RBitMask    = 0x00FF0000;
            pixel_format.GBitMask    = 0x0000FF00;
            pixel_format.BBitMask    = 0x000000FF;
            pixel_format.ABitMask    = 0xFF000000;
            pixel_format.fourCC      = 0;
            break;
        }

        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC7_UNORM: {
            pixel_format.flags  = DDS_FOURCC;
            pixel_format.fourCC = 0x30315844; // DX10
            break;
        }
    }

    return pixel_format;
}
} // namespace jcmr