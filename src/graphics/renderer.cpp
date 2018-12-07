#include "renderer.h"
#include "../game/renderblocks/irenderblock.h"
#include "../window.h"
#include "camera.h"
#include "shader_manager.h"
#include "texture_manager.h"
#include "types.h"
#include "ui.h"

#include <examples/imgui_impl_dx11.h>
#include <examples/imgui_impl_win32.h>
#include <imgui.h>

#include <glm/gtc/type_ptr.hpp>

void SetupImGuiStyle();

Renderer::Renderer()
{
    Window::Get()->Events().SizeChanged.connect([this](const glm::vec2& size) { SetResolution(size); });

    // draw render blocks
    m_RenderEvents.PreRender.connect([this](RenderContext_t* context) {
        std::lock_guard<decltype(m_RenderListMutex)> _lk{m_RenderListMutex};
        for (const auto& render_block : m_RenderList) {
            // draw the model
            render_block->Setup(context);

            // update alpha test constants
            SetPixelShaderConstants(m_AlphaTestConstants, 4, m_cbAlphaTestConsts);

            render_block->Draw(context);

            // reset render states
            SetDefaultRenderStates();
        }
    });
}

Renderer::~Renderer()
{
    Shutdown();
}

ID3D11Texture2D* Renderer::CreateTexture2D(const glm::vec2& size, DXGI_FORMAT format, uint32_t bindFlags,
                                           const char* debugName)
{
    D3D11_TEXTURE2D_DESC textureDesc{};
    textureDesc.Width              = static_cast<UINT>(size.x);
    textureDesc.Height             = static_cast<UINT>(size.y);
    textureDesc.MipLevels          = 1;
    textureDesc.ArraySize          = 1;
    textureDesc.Format             = format;
    textureDesc.SampleDesc.Count   = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage              = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags          = bindFlags;
    textureDesc.CPUAccessFlags     = 0;
    textureDesc.MiscFlags          = 0;

    ID3D11Texture2D* texture = nullptr;
    auto             result  = m_Device->CreateTexture2D(&textureDesc, nullptr, &texture);
    assert(SUCCEEDED(result));

#ifdef RENDERER_REPORT_LIVE_OBJECTS
    D3D_SET_OBJECT_NAME_A(texture, debugName);
#endif

    return texture;
}

bool Renderer::Initialise(const HWND& hwnd)
{
    auto size = Window::Get()->GetSize();

    CreateDevice(hwnd, size);
    CreateRasterizerState();
    CreateDepthStencil(size);
    CreateRenderTarget(size);
    CreateGBuffer(size);
    CreateBlendState();

    // create vertex shader global constants
    m_GlobalConstants[0] = CreateConstantBuffer(m_cbVertexGlobalConsts, "Renderer VertexGlobalConstants");
    memset(&m_cbVertexGlobalConsts, 0, sizeof(m_cbVertexGlobalConsts));

    // create fragment shader global constants
    m_GlobalConstants[1] = CreateConstantBuffer(m_cbFragmentGlobalConsts, "Renderer FragmentGlobalConstants");
    memset(&m_cbFragmentGlobalConsts, 0, sizeof(m_cbFragmentGlobalConsts));

    // create alpha test constants
    m_AlphaTestConstants = CreateConstantBuffer(m_cbAlphaTestConsts, "Renderer AlphaTestConstants");

    // setup imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // initialise imgui
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(m_Device, m_DeviceContext);
    SetupImGuiStyle();

    // setup the render context
    m_RenderContext.m_Renderer      = this;
    m_RenderContext.m_Device        = m_Device;
    m_RenderContext.m_DeviceContext = m_DeviceContext;
    return true;
}

void Renderer::Shutdown()
{
    TextureManager::Get()->Shutdown();
    ShaderManager::Get()->Shutdown();

    if (m_SwapChain) {
        m_SwapChain->SetFullscreenState(false, nullptr);
    }

    m_DeviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    m_DeviceContext->RSSetState(nullptr);

    DestroyRenderTargets();
    DestroyGBuffer();
    DestroyDepthStencil();
    DestroyBuffer(m_GlobalConstants[0]);
    DestroyBuffer(m_GlobalConstants[1]);
    DestroyBuffer(m_AlphaTestConstants);

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    SAFE_RELEASE(m_BlendState);
    SAFE_RELEASE(m_RasterizerState);
    SAFE_RELEASE(m_DeviceContext);
    SAFE_RELEASE(m_Device);
    SAFE_RELEASE(m_SwapChain);

#ifdef RENDERER_REPORT_LIVE_OBJECTS
    m_DeviceDebugger->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
#endif

#ifdef DEBUG
    SAFE_RELEASE(m_DeviceDebugger);
#endif
}

void Renderer::SetResolution(const glm::vec2& size)
{
    if (size.x == 0 || size.y == 0 || !m_Device || !m_DeviceContext) {
        return;
    }

    // ImGui_ImplDX11_InvalidateDeviceObjects();

    DestroyRenderTargets();
    DestroyGBuffer();
    DestroyDepthStencil();

    m_SwapChain->ResizeBuffers(0, static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y), DXGI_FORMAT_UNKNOWN, 0);

    CreateDepthStencil(size);
    CreateRenderTarget(size);
    CreateGBuffer(size);

    // ImGui_ImplDX11_CreateDeviceObjects();
}

void Renderer::SetGBufferResolution(const glm::vec2& size)
{
    DestroyGBuffer();
    CreateGBuffer(size);
}

bool Renderer::Render(const float delta_time)
{
    // start frame for imgui
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    m_RenderContext.m_DeltaTime = delta_time;

    // begin scene
    {
        // clear back buffer
        m_DeviceContext->ClearRenderTargetView(m_BackBuffer, glm::value_ptr(m_ClearColour));

        // clear gbuffer
        static float _black[4] = {0, 0, 0, 0};
        for (const auto& rt : m_GBuffer) {
            if (rt) {
                m_DeviceContext->ClearRenderTargetView(rt, glm::value_ptr(m_ClearColour));
            }
        }

        m_DeviceContext->ClearDepthStencilView(m_DepthStencilView, (D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL), 1.0f, 0);
    }

    // update the camera
    Camera::Get()->Update(&m_RenderContext);

    // update global constants
    UpdateGlobalConstants();

    // g buffer
    {
        SetDepthEnabled(true);
        m_DeviceContext->OMSetRenderTargets(m_GBuffer.size(), &m_GBuffer[0], m_DepthStencilView);
        m_RenderEvents.PreRender(&m_RenderContext);
    }

    // activate back buffer
    SetDepthEnabled(false);
    m_DeviceContext->OMSetRenderTargets(1, &m_BackBuffer, m_DepthStencilView);

    // render ui
    UI::Get()->Render(&m_RenderContext);

    m_RenderEvents.PostRender(&m_RenderContext);

    // end scene
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    m_SwapChain->Present(1, 0);
    return true;
}

void Renderer::SetupRenderStates()
{
    CreateBlendState();
    CreateRasterizerState();
}

void Renderer::SetDefaultRenderStates()
{
    // reset all shader resources
    static ID3D11ShaderResourceView* nullViews[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {nullptr};
    m_DeviceContext->PSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nullViews);

    // reset all samplers
    static ID3D11SamplerState* nullSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT] = {nullptr};
    m_DeviceContext->PSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, nullSamplers);

    // reset blending
    SetBlendingFunc(D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_ONE, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_ZERO);
    SetBlendingEq(D3D11_BLEND_OP_ADD, D3D11_BLEND_OP_ADD);
}

void Renderer::Draw(uint32_t first_vertex, uint32_t vertex_count)
{
    SetupRenderStates();
    m_DeviceContext->Draw(vertex_count, first_vertex);
}

void Renderer::DrawIndexed(uint32_t start_index, uint32_t index_count)
{
    SetupRenderStates();
    m_DeviceContext->DrawIndexed(index_count, start_index, 0);
}

void Renderer::DrawIndexed(uint32_t start_index, uint32_t index_count, IndexBuffer_t* buffer)
{
    if (buffer->m_Buffer) {
        m_DeviceContext->IASetIndexBuffer(buffer->m_Buffer, DXGI_FORMAT_R16_UINT, 0);
    }

    DrawIndexed(start_index, index_count);
}

void Renderer::SetBlendingEnabled(bool state)
{
    if (m_RenderContext.m_AlphaBlendEnabled != state) {
        m_RenderContext.m_AlphaBlendEnabled = state;
        m_RenderContext.m_BlendIsDirty      = true;
    }
}

void Renderer::SetBlendingFunc(D3D11_BLEND source_colour, D3D11_BLEND source_alpha, D3D11_BLEND dest_colour,
                               D3D11_BLEND dest_alpha)
{
    if (m_RenderContext.m_BlendSourceColour != source_colour) {
        m_RenderContext.m_BlendSourceColour = source_colour;
        m_RenderContext.m_BlendIsDirty      = true;
    }

    if (m_RenderContext.m_BlendSourceAlpha != source_alpha) {
        m_RenderContext.m_BlendSourceAlpha = source_alpha;
        m_RenderContext.m_BlendIsDirty     = true;
    }

    if (m_RenderContext.m_BlendDestColour != dest_colour) {
        m_RenderContext.m_BlendDestColour = dest_colour;
        m_RenderContext.m_BlendIsDirty    = true;
    }

    if (m_RenderContext.m_BlendDestAlpha != dest_alpha) {
        m_RenderContext.m_BlendDestAlpha = dest_alpha;
        m_RenderContext.m_BlendIsDirty   = true;
    }
}

void Renderer::SetBlendingEq(D3D11_BLEND_OP colour, D3D11_BLEND_OP alpha)
{
    if (m_RenderContext.m_BlendColourEq != colour) {
        m_RenderContext.m_BlendColourEq = colour;
        m_RenderContext.m_BlendIsDirty  = true;
    }

    if (m_RenderContext.m_BlendAlphaEq != alpha) {
        m_RenderContext.m_BlendAlphaEq = alpha;
        m_RenderContext.m_BlendIsDirty = true;
    }
}

void Renderer::SetDepthEnabled(bool state)
{
    m_DeviceContext->OMSetDepthStencilState(state ? m_DepthStencilEnabledState : m_DepthStencilDisabledState, 1);
}

void Renderer::SetAlphaTestEnabled(bool state)
{
    /*if (m_RenderContext.m_AlphaTestEnabled != state) {
        m_RenderContext.m_AlphaTestEnabled = state;

        if (state) {
            m_cbAlphaTestConsts.AlphaMulRef[0] = m_cbAlphaTestConsts.AlphaMulRef[2];
            m_cbAlphaTestConsts.AlphaMulRef[1] = m_cbAlphaTestConsts.AlphaMulRef[3];
        } else {
            m_cbAlphaTestConsts.AlphaMulRef[0] = 0.0f;
            m_cbAlphaTestConsts.AlphaMulRef[1] = 1.0f;
        }
    }*/
}

void Renderer::SetFillMode(D3D11_FILL_MODE mode)
{
    if (m_RenderContext.m_FillMode != mode) {
        m_RenderContext.m_FillMode      = mode;
        m_RenderContext.m_RasterIsDirty = true;
    }
}

void Renderer::SetCullMode(D3D11_CULL_MODE mode)
{
    if (m_RenderContext.m_CullMode != mode) {
        m_RenderContext.m_CullMode      = mode;
        m_RenderContext.m_RasterIsDirty = true;
    }
}

void Renderer::CreateRenderTarget(const glm::vec2& size)
{
    // create back buffer render target
    ID3D11Texture2D* back_buffer = nullptr;
    auto result = m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back_buffer));
    assert(SUCCEEDED(result));

    result = m_Device->CreateRenderTargetView(back_buffer, nullptr, &m_BackBuffer);
    assert(SUCCEEDED(result));

    SAFE_RELEASE(back_buffer);

#ifdef RENDERER_REPORT_LIVE_OBJECTS
    D3D_SET_OBJECT_NAME_A(m_BackBuffer, "Renderer Back Buffer");
#endif

    // set the render target
    m_DeviceContext->OMSetRenderTargets(1, &m_BackBuffer, m_DepthStencilView);

    // create the viewport
    {
        D3D11_VIEWPORT viewport{};
        viewport.Width    = size.x;
        viewport.Height   = size.y;
        viewport.MaxDepth = 1.0f;

        m_DeviceContext->RSSetViewports(1, &viewport);
    }
}

void Renderer::CreateGBuffer(const glm::vec2& size)
{
    // create diffuse render target
    {
        auto tex    = CreateTexture2D(size, DXGI_FORMAT_R8G8B8A8_UNORM,
                                   (D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE), "Diffuse texture");
        auto result = m_Device->CreateRenderTargetView(tex, nullptr, &m_GBuffer[0]);
        assert(SUCCEEDED(result));

        D3D11_SHADER_RESOURCE_VIEW_DESC desc;
        desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        desc.Texture2D     = {0, static_cast<UINT>(-1)};
        result             = m_Device->CreateShaderResourceView(tex, &desc, &m_GBufferSRV[0]);
        assert(SUCCEEDED(result));

        SAFE_RELEASE(tex);

#ifdef RENDERER_REPORT_LIVE_OBJECTS
        D3D_SET_OBJECT_NAME_A(m_GBuffer[0], "Renderer GBuffer (Diffuse)");
        D3D_SET_OBJECT_NAME_A(m_GBufferSRV[0], "Renderer GBuffer SRV (Diffuse)");
#endif
    }

    // create normals render target
    {
        auto tex    = CreateTexture2D(size, DXGI_FORMAT_R10G10B10A2_UNORM,
                                   (D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE), "Normals texture");
        auto result = m_Device->CreateRenderTargetView(tex, nullptr, &m_GBuffer[1]);
        assert(SUCCEEDED(result));

        D3D11_SHADER_RESOURCE_VIEW_DESC desc;
        desc.Format        = DXGI_FORMAT_R10G10B10A2_UNORM;
        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        desc.Texture2D     = {0, static_cast<UINT>(-1)};
        result             = m_Device->CreateShaderResourceView(tex, &desc, &m_GBufferSRV[1]);
        assert(SUCCEEDED(result));

        SAFE_RELEASE(tex);

#ifdef RENDERER_REPORT_LIVE_OBJECTS
        D3D_SET_OBJECT_NAME_A(m_GBuffer[1], "Renderer GBuffer (Normals)");
        D3D_SET_OBJECT_NAME_A(m_GBufferSRV[1], "Renderer GBuffer SRV (Normals)");
#endif
    }

    // create properties render target
    {
        auto tex    = CreateTexture2D(size, DXGI_FORMAT_R8G8B8A8_UNORM,
                                   (D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE), "Properties texture");
        auto result = m_Device->CreateRenderTargetView(tex, nullptr, &m_GBuffer[2]);
        assert(SUCCEEDED(result));

        D3D11_SHADER_RESOURCE_VIEW_DESC desc;
        desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        desc.Texture2D     = {0, static_cast<UINT>(-1)};
        result             = m_Device->CreateShaderResourceView(tex, &desc, &m_GBufferSRV[2]);
        assert(SUCCEEDED(result));

        SAFE_RELEASE(tex);

#ifdef RENDERER_REPORT_LIVE_OBJECTS
        D3D_SET_OBJECT_NAME_A(m_GBuffer[2], "Renderer GBuffer (Properties)");
        D3D_SET_OBJECT_NAME_A(m_GBufferSRV[2], "Renderer GBuffer SRV (Properties)");
#endif
    }

    // create properties ex render target
    {
        auto tex    = CreateTexture2D(size, DXGI_FORMAT_R8G8B8A8_UNORM,
                                   (D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE), "PropertiesEx texture");
        auto result = m_Device->CreateRenderTargetView(tex, nullptr, &m_GBuffer[3]);
        assert(SUCCEEDED(result));

        D3D11_SHADER_RESOURCE_VIEW_DESC desc;
        desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        desc.Texture2D     = {0, static_cast<UINT>(-1)};
        result             = m_Device->CreateShaderResourceView(tex, &desc, &m_GBufferSRV[3]);
        assert(SUCCEEDED(result));

        SAFE_RELEASE(tex);

#ifdef RENDERER_REPORT_LIVE_OBJECTS
        D3D_SET_OBJECT_NAME_A(m_GBuffer[3], "Renderer GBuffer (PropertiesEx)");
        D3D_SET_OBJECT_NAME_A(m_GBufferSRV[3], "Renderer GBuffer SRV (PropertiesEx)");
#endif
    }
}

void Renderer::CreateDevice(const HWND& hwnd, const glm::vec2& size)
{
    DXGI_SWAP_CHAIN_DESC desc{};
    desc.BufferCount                        = 1;
    desc.BufferDesc.Width                   = static_cast<uint32_t>(size.x);
    desc.BufferDesc.Height                  = static_cast<uint32_t>(size.y);
    desc.BufferDesc.RefreshRate.Numerator   = 60;
    desc.BufferDesc.RefreshRate.Denominator = 1;
    desc.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferDesc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    desc.BufferDesc.Scaling                 = DXGI_MODE_SCALING_UNSPECIFIED;
    desc.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow                       = hwnd;
    desc.SampleDesc.Count                   = 1;
    desc.SampleDesc.Quality                 = 0;
    desc.Windowed                           = true;
    desc.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;
    desc.Flags                              = 0;

    UINT deviceFlags = 0;
#ifdef DEBUG
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    auto result =
        D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, deviceFlags, nullptr, 0,
                                      D3D11_SDK_VERSION, &desc, &m_SwapChain, &m_Device, nullptr, &m_DeviceContext);
    assert(SUCCEEDED(result));

#ifdef DEBUG
    result = m_Device->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&m_DeviceDebugger));
    assert(SUCCEEDED(result));

    ID3D11InfoQueue* info_queue;
    result = m_DeviceDebugger->QueryInterface(__uuidof(ID3D11InfoQueue), reinterpret_cast<void**>(&info_queue));
    assert(SUCCEEDED(result));

    // hide specific messages
    D3D11_MESSAGE_ID messages_to_hide[] = {D3D11_MESSAGE_ID_DEVICE_DRAW_SAMPLER_NOT_SET};

    D3D11_INFO_QUEUE_FILTER filter{};
    filter.DenyList.NumIDs  = _countof(messages_to_hide);
    filter.DenyList.pIDList = messages_to_hide;
    info_queue->AddStorageFilterEntries(&filter);
    info_queue->Release();
#endif
}

void Renderer::CreateDepthStencil(const glm::vec2& size)
{
    // create the depth stencil
    {
        D3D11_DEPTH_STENCIL_DESC depthStencilDesc{};
        depthStencilDesc.DepthEnable                  = true;
        depthStencilDesc.DepthWriteMask               = D3D11_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc                    = D3D11_COMPARISON_LESS;
        depthStencilDesc.StencilEnable                = false;
        depthStencilDesc.FrontFace.StencilFunc        = D3D11_COMPARISON_ALWAYS;
        depthStencilDesc.FrontFace.StencilPassOp      = D3D11_STENCIL_OP_KEEP;
        depthStencilDesc.FrontFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;
        depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
        depthStencilDesc.BackFace.StencilFunc         = D3D11_COMPARISON_ALWAYS;
        depthStencilDesc.BackFace.StencilPassOp       = D3D11_STENCIL_OP_KEEP;
        depthStencilDesc.BackFace.StencilFailOp       = D3D11_STENCIL_OP_KEEP;
        depthStencilDesc.BackFace.StencilDepthFailOp  = D3D11_STENCIL_OP_DECR;

        auto result = m_Device->CreateDepthStencilState(&depthStencilDesc, &m_DepthStencilEnabledState);
        assert(SUCCEEDED(result));

#ifdef RENDERER_REPORT_LIVE_OBJECTS
        D3D_SET_OBJECT_NAME_A(m_DepthStencilEnabledState, "CreateDepthStencil m_DepthStencilEnabledState");
#endif

        depthStencilDesc.DepthEnable    = false;
        depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

        result = m_Device->CreateDepthStencilState(&depthStencilDesc, &m_DepthStencilDisabledState);
        assert(SUCCEEDED(result));

#ifdef RENDERER_REPORT_LIVE_OBJECTS
        D3D_SET_OBJECT_NAME_A(m_DepthStencilDisabledState, "CreateDepthStencil m_DepthStencilDisabledState");
#endif

        // enable depth
        SetDepthEnabled(true);
    }

    // create the depth buffer
    m_DepthTexture = CreateTexture2D(size, DXGI_FORMAT_R24G8_TYPELESS,
                                     (D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE), "Depth stencil texture");

    // create depth stencil view
    {
        D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
        depthStencilViewDesc.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthStencilViewDesc.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
        depthStencilViewDesc.Texture2D.MipSlice = 0;
        depthStencilViewDesc.Flags              = 0;

        auto result = m_Device->CreateDepthStencilView(m_DepthTexture, &depthStencilViewDesc, &m_DepthStencilView);
        assert(SUCCEEDED(result));

#ifdef RENDERER_REPORT_LIVE_OBJECTS
        D3D_SET_OBJECT_NAME_A(m_DepthStencilDisabledState, "Renderer::CreateDepthStencil - m_DepthTexture");
#endif
    }
}

void Renderer::CreateBlendState()
{
    if (m_RenderContext.m_BlendIsDirty) {
        SAFE_RELEASE(m_BlendState);

        D3D11_BLEND_DESC blendDesc{};
        blendDesc.AlphaToCoverageEnable                 = false;
        blendDesc.RenderTarget[0].BlendEnable           = m_RenderContext.m_AlphaBlendEnabled;
        blendDesc.RenderTarget[0].SrcBlend              = m_RenderContext.m_BlendSourceColour;
        blendDesc.RenderTarget[0].DestBlend             = m_RenderContext.m_BlendDestColour;
        blendDesc.RenderTarget[0].BlendOp               = m_RenderContext.m_BlendColourEq;
        blendDesc.RenderTarget[0].SrcBlendAlpha         = m_RenderContext.m_BlendSourceAlpha;
        blendDesc.RenderTarget[0].DestBlendAlpha        = m_RenderContext.m_BlendDestAlpha;
        blendDesc.RenderTarget[0].BlendOpAlpha          = m_RenderContext.m_BlendAlphaEq;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

        auto result = m_Device->CreateBlendState(&blendDesc, &m_BlendState);
        assert(SUCCEEDED(result));

#ifdef RENDERER_REPORT_LIVE_OBJECTS
        D3D_SET_OBJECT_NAME_A(m_BlendState, "Renderer::CreateBlendState");
#endif

        m_DeviceContext->OMSetBlendState(m_BlendState, nullptr, 0xFFFFFFFF);

        m_RenderContext.m_BlendIsDirty = false;
    }
}

void Renderer::CreateRasterizerState()
{
    if (m_RenderContext.m_RasterIsDirty) {
        SAFE_RELEASE(m_RasterizerState);

        D3D11_RASTERIZER_DESC rasterDesc{};
        rasterDesc.FillMode        = m_RenderContext.m_FillMode;
        rasterDesc.CullMode        = m_RenderContext.m_CullMode;
        rasterDesc.DepthClipEnable = true;

        auto result = m_Device->CreateRasterizerState(&rasterDesc, &m_RasterizerState);
        assert(SUCCEEDED(result));

#ifdef RENDERER_REPORT_LIVE_OBJECTS
        D3D_SET_OBJECT_NAME_A(m_RasterizerState, "Renderer::CreateRasterizerState");
#endif

        m_DeviceContext->RSSetState(m_RasterizerState);

        m_RenderContext.m_RasterIsDirty = false;
    }
}

void Renderer::DestroyRenderTargets()
{
    static ID3D11RenderTargetView* null_targets[5] = {nullptr};
    m_DeviceContext->OMSetRenderTargets(5, null_targets, nullptr);

    // free back buffer
    SAFE_RELEASE(m_BackBuffer);
}

void Renderer::DestroyGBuffer()
{
    for (int i = 0; i < m_GBuffer.size(); ++i) {
        SAFE_RELEASE(m_GBufferSRV[i]);
        SAFE_RELEASE(m_GBuffer[i]);
    }
}

void Renderer::DestroyDepthStencil()
{
    m_DeviceContext->OMSetDepthStencilState(nullptr, 0);

    SAFE_RELEASE(m_DepthStencilEnabledState);
    SAFE_RELEASE(m_DepthStencilDisabledState);
    SAFE_RELEASE(m_DepthStencilView);
    SAFE_RELEASE(m_DepthTexture);
}

IBuffer_t* Renderer::CreateBuffer(const void* data, uint32_t count, uint32_t stride, BufferType type,
                                  const char* debugName)
{
    auto buffer             = new IBuffer_t;
    buffer->m_ElementCount  = count;
    buffer->m_ElementStride = stride;
    buffer->m_Type          = type;
    buffer->m_Usage         = D3D11_USAGE_DEFAULT;

    // copy the buffer
    buffer->m_Data.resize(count * stride);
    std::memcpy(buffer->m_Data.data(), data, (count * stride));

    D3D11_BUFFER_DESC desc{};
    desc.Usage     = buffer->m_Usage;
    desc.ByteWidth = (count * stride);
    desc.BindFlags = type == VERTEX_BUFFER ? D3D11_BIND_VERTEX_BUFFER : D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA resourceData{};
    resourceData.pSysMem = data;

    auto result = m_Device->CreateBuffer(&desc, &resourceData, &buffer->m_Buffer);

#ifdef RENDERER_REPORT_LIVE_OBJECTS
    D3D_SET_OBJECT_NAME_A(buffer->m_Buffer, debugName);
#endif

    if (FAILED(result)) {
        SAFE_DELETE(buffer);
        return nullptr;
    }

    return buffer;
}

void Renderer::DestroyBuffer(IBuffer_t* buffer)
{
    if (buffer) {
        SAFE_RELEASE(buffer->m_Buffer);
        SAFE_DELETE(buffer);
    }
}

void Renderer::SetVertexStream(VertexBuffer_t* buffer, int32_t slot, uint32_t offset)
{
    assert(buffer && buffer->m_Type == VERTEX_BUFFER);
    m_DeviceContext->IASetVertexBuffers(slot, 1, &buffer->m_Buffer, &buffer->m_ElementStride, &offset);
}

void Renderer::SetSamplerState(SamplerState_t* sampler, int32_t slot)
{
    assert(sampler && sampler->m_SamplerState);
    m_DeviceContext->PSSetSamplers(slot, 1, &sampler->m_SamplerState);
}

void Renderer::ClearTexture(int32_t slot)
{
    ClearTextures(slot, slot);
}

void Renderer::ClearTextures(int32_t start, int32_t last)
{
    static ID3D11ShaderResourceView* null_srv[20] = {nullptr};
    m_DeviceContext->PSSetShaderResources(start, (last - start) + 1, null_srv);
}

VertexDeclaration_t* Renderer::CreateVertexDeclaration(const D3D11_INPUT_ELEMENT_DESC* layout, uint32_t count,
                                                       VertexShader_t* shader, const char* debugName)
{
    auto declaration = new VertexDeclaration_t;
    assert(declaration);

    auto result =
        m_Device->CreateInputLayout(layout, count, shader->m_Code.data(), shader->m_Size, &declaration->m_Layout);
    assert(SUCCEEDED(result));

#ifdef RENDERER_REPORT_LIVE_OBJECTS
    D3D_SET_OBJECT_NAME_A(declaration->m_Layout, debugName);
#endif

    return declaration;
}

void Renderer::DestroyVertexDeclaration(VertexDeclaration_t* declaration)
{
    if (declaration) {
        SAFE_RELEASE(declaration->m_Layout);
        SAFE_DELETE(declaration);
    }
}

SamplerState_t* Renderer::CreateSamplerState(const D3D11_SAMPLER_DESC& params, const char* debugName)
{
    auto sampler = new SamplerState_t;
    auto result  = m_Device->CreateSamplerState(&params, &sampler->m_SamplerState);
    assert(SUCCEEDED(result));

#ifdef RENDERER_REPORT_LIVE_OBJECTS
    D3D_SET_OBJECT_NAME_A(sampler->m_SamplerState, debugName);
#endif

    if (FAILED(result)) {
        SAFE_DELETE(sampler);
        return nullptr;
    }

    return sampler;
}

void Renderer::DestroySamplerState(SamplerState_t* sampler)
{
    if (sampler) {
        SAFE_RELEASE(sampler->m_SamplerState);
        SAFE_DELETE(sampler);
    }
}

void Renderer::UpdateGlobalConstants()
{
    // update vertex shader constants
    {
        m_cbVertexGlobalConsts.ViewProjectionMatrix = m_RenderContext.m_viewProjectionMatrix;
        // m_cbVertexGlobalConsts.CameraPosition = glm::vec4(Camera::Get()->GetPosition(), 1);
        m_cbVertexGlobalConsts.LightDiffuseColour    = glm::vec4(1, 0, 0, 1);
        m_cbVertexGlobalConsts.LightDirection        = glm::vec4(-1, -1, -1, 0);
        m_cbVertexGlobalConsts.ViewProjectionMatrix2 = m_RenderContext.m_viewProjectionMatrix;
        m_cbVertexGlobalConsts.ViewProjectionMatrix3 = m_RenderContext.m_viewProjectionMatrix;

        // set the shader constants
        SetVertexShaderConstants(m_GlobalConstants[0], 0, m_cbVertexGlobalConsts);
    }

    // update fragment shader constants
    {
        m_cbFragmentGlobalConsts.LightSpecularColour = glm::vec4(0, 1, 0, 1);
        m_cbFragmentGlobalConsts.LightDiffuseColour  = m_cbVertexGlobalConsts.LightDiffuseColour;
        m_cbFragmentGlobalConsts.LightDirection      = m_cbVertexGlobalConsts.LightDirection;
        // m_cbFragmentGlobalConsts.CameraPosition = m_cbVertexGlobalConsts.CameraPosition;
        m_cbFragmentGlobalConsts.LightDirection2 = m_cbVertexGlobalConsts.LightDirection;

        // set the shader constants
        SetPixelShaderConstants(m_GlobalConstants[1], 0, m_cbFragmentGlobalConsts);
    }
}

void Renderer::AddToRenderList(const std::vector<IRenderBlock*>& renderblocks)
{
    std::lock_guard<decltype(m_RenderListMutex)> _lk{m_RenderListMutex};
    std::copy(renderblocks.begin(), renderblocks.end(), std::back_inserter(m_RenderList));

    // sort by opaque items, so that transparent blocks will be under them
    std::sort(m_RenderList.begin(), m_RenderList.end(),
              [](IRenderBlock* lhs, IRenderBlock* rhs) { return lhs->IsOpaque() > rhs->IsOpaque(); });
}

void Renderer::AddToRenderList(IRenderBlock* renderblock)
{
    std::lock_guard<decltype(m_RenderListMutex)> _lk{m_RenderListMutex};
    m_RenderList.emplace_back(renderblock);

    // sort by opaque items, so that transparent blocks will be under them
    std::sort(m_RenderList.begin(), m_RenderList.end(),
              [](IRenderBlock* lhs, IRenderBlock* rhs) { return lhs->IsOpaque() > rhs->IsOpaque(); });
}

void Renderer::RemoveFromRenderList(const std::vector<IRenderBlock*>& renderblocks)
{
    std::lock_guard<decltype(m_RenderListMutex)> _lk{m_RenderListMutex};
    for (const auto& render_block : renderblocks) {
        m_RenderList.erase(std::remove(m_RenderList.begin(), m_RenderList.end(), render_block), m_RenderList.end());
    }
}
