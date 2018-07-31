#include <graphics/Renderer.h>
#include <graphics/ShaderManager.h>
#include <graphics/TextureManager.h>
#include <graphics/UI.h>
#include <graphics/Camera.h>
#include <graphics/DebugRenderer.h>

#include <Window.h>

#include <gtc/type_ptr.hpp>

void SetupImGuiStyle();

Renderer::Renderer()
{
    Window::Get()->Events().SizeChanged.connect([this](const glm::vec2& size) {
        if (size.x == 0 || size.y == 0 || !m_Device || !m_DeviceContext) {
            return true;
        }

        ImGui_ImplDX11_InvalidateDeviceObjects();

        DestroyRenderTarget();
        DestroyDepthStencil();

        m_SwapChain->ResizeBuffers(0, static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y), DXGI_FORMAT_UNKNOWN, 0);

        CreateDepthStencil(size);
        CreateRenderTarget(size);

        ImGui_ImplDX11_CreateDeviceObjects();
        return true;
    });
}

Renderer::~Renderer()
{
    Shutdown();
}

ID3D11Texture2D* Renderer::CreateTexture2D(const glm::vec2& size, DXGI_FORMAT format, uint32_t bindFlags, const char* debugName)
{
    D3D11_TEXTURE2D_DESC textureDesc;
    textureDesc.Width = static_cast<UINT>(size.x);
    textureDesc.Height = static_cast<UINT>(size.y);
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = format;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = bindFlags;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

    ID3D11Texture2D* texture = nullptr;
    auto result = m_Device->CreateTexture2D(&textureDesc, nullptr, &texture);
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
    CreateBlendState();

    // create global constants
    m_GlobalConstants = CreateConstantBuffer(m_cbGlobalConsts, "Renderer GlobalConstants");
    memset(&m_cbGlobalConsts, 0, sizeof(m_cbGlobalConsts));

    // setup imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // initialise imgui
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(m_Device, m_DeviceContext);
    SetupImGuiStyle();

    // setup the render context
    m_RenderContext.m_Renderer = this;
    m_RenderContext.m_Device = m_Device;
    m_RenderContext.m_DeviceContext = m_DeviceContext;

    DEBUG_LOG("Renderer is ready!");
    return true;
}

void Renderer::Shutdown()
{
    DEBUG_LOG("Renderer is shutting down...");

    TextureManager::Get()->Shutdown();
    ShaderManager::Get()->Shutdown();
    Camera::Get()->Shutdown();

    if (m_SwapChain) {
        m_SwapChain->SetFullscreenState(false, nullptr);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    m_DeviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    m_DeviceContext->RSSetState(nullptr);

    DestroyRenderTarget();
    DestroyDepthStencil();
    DestroyBuffer(m_GlobalConstants);

    SAFE_RELEASE(m_BlendState);
    SAFE_RELEASE(m_SamplerState);
    SAFE_RELEASE(m_RasterizerState);
    SAFE_RELEASE(m_SwapChain);
    SAFE_RELEASE(m_DeviceContext);
    SAFE_RELEASE(m_Device);

#ifdef RENDERER_REPORT_LIVE_OBJECTS
    m_DeviceDebugger->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
#endif

#ifdef DEBUG
    SAFE_RELEASE(m_DeviceDebugger);
#endif
}

bool Renderer::Render()
{
    // start frame for imgui
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // begin scene
    {
        for (auto& render_target : m_RenderTargetView) {
            if (render_target) {
                m_DeviceContext->ClearRenderTargetView(render_target, glm::value_ptr(m_ClearColour));
            }
        }

        m_DeviceContext->ClearDepthStencilView(m_DepthStencilView, (D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL), 1.0f, 0);
    }

    // Begin the 2d renderer
    DebugRenderer::Get()->Begin(&m_RenderContext);

    // update the camera
    Camera::Get()->Update(&m_RenderContext);

    // update global constants
    UpdateGlobalConstants();

    //
    m_RenderEvents.RenderFrame(&m_RenderContext);

    SetDepthEnabled(false);

    // Render UI
    UI::Get()->Render();

    // end scene
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    SetDepthEnabled(true);
    m_SwapChain->Present(1, 0);
    return true;
}

void Renderer::SetupRenderStates()
{
    //CreateBlendState();
    CreateRasterizerState();
}

void Renderer::SetDefaultRenderStates()
{
    // reset all shader resources
    ID3D11ShaderResourceView* nullViews[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = { nullptr };
    m_DeviceContext->PSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nullViews);

    // reset all samplers
    ID3D11SamplerState* nullSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT] = { nullptr };
    m_DeviceContext->PSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, nullSamplers);
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
    m_DeviceContext->OMSetBlendState(state ? m_BlendState : nullptr, nullptr, 0xFFFFFFFF);
}

void Renderer::SetDepthEnabled(bool state)
{
    m_DeviceContext->OMSetDepthStencilState(state ? m_DepthStencilEnabledState : m_DepthStencilDisabledState, 1);
}

void Renderer::SetFillMode(D3D11_FILL_MODE mode)
{
    if (m_RenderContext.m_FillMode != mode) {
        m_RenderContext.m_FillMode = mode;
        m_RenderContext.m_RasterIsDirty = true;
    }
}

void Renderer::SetCullMode(D3D11_CULL_MODE mode)
{
    if (m_RenderContext.m_CullMode != mode) {
        m_RenderContext.m_CullMode = mode;
        m_RenderContext.m_RasterIsDirty = true;
    }
}

void Renderer::CreateRenderTarget(const glm::vec2& size)
{
    // create back buffer render target
    {
        ID3D11Texture2D* back_buffer = nullptr;
        auto result = m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back_buffer));
        assert(SUCCEEDED(result));

        result = m_Device->CreateRenderTargetView(back_buffer, nullptr, &m_RenderTargetView[0]);
        assert(SUCCEEDED(result));

        SAFE_RELEASE(back_buffer);

#ifdef RENDERER_REPORT_LIVE_OBJECTS
        D3D_SET_OBJECT_NAME_A(m_RenderTargetView[0], "Renderer Back Buffer");
#endif
    }

    // create normals render target
    {
        auto normalsTex = CreateTexture2D(size, DXGI_FORMAT_R8G8B8A8_UNORM, (D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE), "Normals texture");
        assert(normalsTex);

        auto result = m_Device->CreateRenderTargetView(normalsTex, nullptr, &m_RenderTargetView[1]);
        assert(SUCCEEDED(result));

        D3D11_SHADER_RESOURCE_VIEW_DESC desc;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        desc.Texture2D = { 0, static_cast<UINT>(-1) };
        result = m_Device->CreateShaderResourceView(normalsTex, &desc, &m_RenderTargetResourceView[0]);
        assert(SUCCEEDED(result));

        SAFE_RELEASE(normalsTex);

#ifdef RENDERER_REPORT_LIVE_OBJECTS
        D3D_SET_OBJECT_NAME_A(m_RenderTargetView[1], "Renderer Normal Buffer");
        D3D_SET_OBJECT_NAME_A(m_RenderTargetResourceView[0], "Renderer Normal Buffer SRV");
#endif
    }

    // create metallic render target
    {
        auto metallicTex = CreateTexture2D(size, DXGI_FORMAT_R8G8B8A8_UNORM, (D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE), "Metallic texture");
        assert(metallicTex);

        auto result = m_Device->CreateRenderTargetView(metallicTex, nullptr, &m_RenderTargetView[2]);
        assert(SUCCEEDED(result));

        D3D11_SHADER_RESOURCE_VIEW_DESC desc;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        desc.Texture2D = { 0, static_cast<UINT>(-1) };
        result = m_Device->CreateShaderResourceView(metallicTex, &desc, &m_RenderTargetResourceView[1]);
        assert(SUCCEEDED(result));

        SAFE_RELEASE(metallicTex);

#ifdef RENDERER_REPORT_LIVE_OBJECTS
        D3D_SET_OBJECT_NAME_A(m_RenderTargetView[2], "Renderer Metallic Buffer");
        D3D_SET_OBJECT_NAME_A(m_RenderTargetResourceView[1], "Renderer Metallic Buffer SRV");
#endif
    }

    // set the render target
    m_DeviceContext->OMSetRenderTargets(3, &m_RenderTargetView[0], m_DepthStencilView);

    // create the viewport
    {
        D3D11_VIEWPORT viewport;
        viewport.Width = size.x;
        viewport.Height = size.y;
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        viewport.TopLeftX = 0;
        viewport.TopLeftY = 0;

        m_DeviceContext->RSSetViewports(1, &viewport);
    }
}

void Renderer::CreateDevice(const HWND& hwnd, const glm::vec2& size)
{
    DXGI_SWAP_CHAIN_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.BufferCount = 1;
    desc.BufferDesc.Width = static_cast<uint32_t>(size.x);
    desc.BufferDesc.Height = static_cast<uint32_t>(size.y);
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferDesc.RefreshRate.Numerator = 60;
    desc.BufferDesc.RefreshRate.Denominator = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = hwnd;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Windowed = true;
    desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    desc.Flags = 0;

    UINT deviceFlags = 0;
#if 0
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    auto result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, deviceFlags, nullptr, 0, D3D11_SDK_VERSION, &desc, &m_SwapChain, &m_Device, nullptr, &m_DeviceContext);
    assert(SUCCEEDED(result));

#if 0
    result = m_Device->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&m_DeviceDebugger));
    assert(SUCCEEDED(result));

    ID3D11InfoQueue* info_queue;
    result = m_DeviceDebugger->QueryInterface(__uuidof(ID3D11InfoQueue), reinterpret_cast<void**>(&info_queue));
    assert(SUCCEEDED(result));

    // hide specific messages
    D3D11_MESSAGE_ID messages_to_hide[] = {
        D3D11_MESSAGE_ID_DEVICE_DRAW_SAMPLER_NOT_SET
    };

    D3D11_INFO_QUEUE_FILTER filter;
    ZeroMemory(&filter, sizeof(filter));
    filter.DenyList.NumIDs = _countof(messages_to_hide);
    filter.DenyList.pIDList = messages_to_hide;
    info_queue->AddStorageFilterEntries(&filter);
    info_queue->Release();
#endif
}

void Renderer::CreateDepthStencil(const glm::vec2& size)
{
    // create the depth stencil
    {
        D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
        ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));
        depthStencilDesc.DepthEnable = true;
        depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
        depthStencilDesc.StencilEnable = false;
        depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
        depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;

        auto result = m_Device->CreateDepthStencilState(&depthStencilDesc, &m_DepthStencilEnabledState);
        assert(SUCCEEDED(result));

#ifdef RENDERER_REPORT_LIVE_OBJECTS
        D3D_SET_OBJECT_NAME_A(m_DepthStencilEnabledState, "CreateDepthStencil m_DepthStencilEnabledState");
#endif

        depthStencilDesc.DepthEnable = false;
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
    m_DepthTexture = CreateTexture2D(size, DXGI_FORMAT_R24G8_TYPELESS, (D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE), "Depth stencil texture");

    // create depth stencil view
    {
        D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
        depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        depthStencilViewDesc.Texture2D.MipSlice = 0;
        depthStencilViewDesc.Flags = 0;

        auto result = m_Device->CreateDepthStencilView(m_DepthTexture, &depthStencilViewDesc, &m_DepthStencilView);
        assert(SUCCEEDED(result));

#ifdef RENDERER_REPORT_LIVE_OBJECTS
        D3D_SET_OBJECT_NAME_A(m_DepthStencilDisabledState, "Renderer::CreateDepthStencil - m_DepthTexture");
#endif
    }
}

void Renderer::CreateBlendState()
{
    D3D11_BLEND_DESC blendDesc;
    ZeroMemory(&blendDesc, sizeof(blendDesc));

    blendDesc.AlphaToCoverageEnable = false;
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    auto result = m_Device->CreateBlendState(&blendDesc, &m_BlendState);
    assert(SUCCEEDED(result));

#ifdef RENDERER_REPORT_LIVE_OBJECTS
    D3D_SET_OBJECT_NAME_A(m_BlendState, "Renderer::CreateBlendState");
#endif
}

void Renderer::CreateRasterizerState()
{
    if (m_RenderContext.m_RasterIsDirty) {
        SAFE_RELEASE(m_RasterizerState);

        D3D11_RASTERIZER_DESC rasterDesc;
        ZeroMemory(&rasterDesc, sizeof(D3D11_RASTERIZER_DESC));

        rasterDesc.FillMode = m_RenderContext.m_FillMode;
        rasterDesc.CullMode = m_RenderContext.m_CullMode;
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

void Renderer::DestroyRenderTarget()
{
    ID3D11RenderTargetView* null_targets[3] = { nullptr };
    m_DeviceContext->OMSetRenderTargets(3, null_targets, nullptr);

    for (auto& resource_view : m_RenderTargetResourceView) {
        SAFE_RELEASE(resource_view);
    }

    for (auto& render_target : m_RenderTargetView) {
        SAFE_RELEASE(render_target);
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

VertexBuffer_t* Renderer::CreateVertexBuffer(uint32_t count, uint32_t stride, const char* debugName)
{
    auto buffer = new VertexBuffer_t;
    buffer->m_ElementCount = count;
    buffer->m_ElementStride = stride;
    buffer->m_Usage = D3D11_USAGE_DYNAMIC;

    D3D11_BUFFER_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.ByteWidth = (count * stride);
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    auto result = m_Device->CreateBuffer(&desc, nullptr, &buffer->m_Buffer);
    assert(SUCCEEDED(result));

#ifdef RENDERER_REPORT_LIVE_OBJECTS
    D3D_SET_OBJECT_NAME_A(buffer->m_Buffer, debugName);
#endif

    if (FAILED(result)) {
        SAFE_DELETE(buffer);
        return nullptr;
    }

    // TODO: create SRV if we need it.

    return buffer;
}

VertexBuffer_t* Renderer::CreateVertexBuffer(const void* data, uint32_t count, uint32_t stride, D3D11_USAGE usage, const char* debugName)
{
    auto buffer = new VertexBuffer_t;
    buffer->m_ElementCount = count;
    buffer->m_ElementStride = stride;
    buffer->m_Usage = usage;

    D3D11_BUFFER_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Usage = usage;
    desc.ByteWidth = (count * stride);
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA resourceData;
    ZeroMemory(&resourceData, sizeof(resourceData));
    resourceData.pSysMem = data;

    auto result = m_Device->CreateBuffer(&desc, &resourceData, &buffer->m_Buffer);
    assert(SUCCEEDED(result));

#ifdef RENDERER_REPORT_LIVE_OBJECTS
    D3D_SET_OBJECT_NAME_A(buffer->m_Buffer, debugName);
#endif

    if (FAILED(result)) {
        SAFE_DELETE(buffer);
        return nullptr;
    }

    // TODO: create SRV if we need it.

    return buffer;
}

IndexBuffer_t* Renderer::CreateIndexBuffer(const void* data, uint32_t count, D3D11_USAGE usage, const char* debugName)
{
    auto buffer = new IndexBuffer_t;
    buffer->m_ElementCount = count;
    buffer->m_ElementStride = sizeof(uint16_t);
    buffer->m_Usage = usage;

    D3D11_BUFFER_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Usage = usage;
    desc.ByteWidth = (count * sizeof(uint16_t));
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA resourceData;
    ZeroMemory(&resourceData, sizeof(resourceData));
    resourceData.pSysMem = data;

    auto result = m_Device->CreateBuffer(&desc, &resourceData, &buffer->m_Buffer);
    assert(SUCCEEDED(result));

#ifdef RENDERER_REPORT_LIVE_OBJECTS
    D3D_SET_OBJECT_NAME_A(buffer->m_Buffer, debugName);
#endif

    if (FAILED(result)) {
        SAFE_DELETE(buffer);
        return nullptr;
    }

    // TODO: create SRV if we need it.

    return buffer;
}

void Renderer::DestroyBuffer(IBuffer_t* buffer)
{
    if (buffer) {
        SAFE_RELEASE(buffer->m_SRV);
        SAFE_RELEASE(buffer->m_Buffer);
        SAFE_DELETE(buffer);
    }
}

void Renderer::SetVertexStream(VertexBuffer_t* buffer, int32_t slot, uint32_t offset)
{
    assert(buffer);
    m_DeviceContext->IASetVertexBuffers(slot, 1, &buffer->m_Buffer, &buffer->m_ElementStride, &offset);
}

void Renderer::SetSamplerState(SamplerState_t* sampler, int32_t slot)
{
    assert(sampler);
    m_DeviceContext->PSSetSamplers(slot, 1, &sampler->m_SamplerState);
}

VertexDeclaration_t* Renderer::CreateVertexDeclaration(const D3D11_INPUT_ELEMENT_DESC* layout, uint32_t count, VertexShader_t* m_Shader, const char* debugName)
{
    auto declaration = new VertexDeclaration_t;

    auto result = m_Device->CreateInputLayout(layout, count, m_Shader->m_Code.data(), m_Shader->m_Size, &declaration->m_Layout);
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

SamplerState_t* Renderer::CreateSamplerState(const SamplerStateCreationParams_t& params, const char* debugName)
{
    auto sampler = new SamplerState_t;

    D3D11_SAMPLER_DESC samplerDesc;
    ZeroMemory(&samplerDesc, sizeof(D3D11_SAMPLER_DESC));

    samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
    samplerDesc.AddressU = params.m_AddressU;
    samplerDesc.AddressV = params.m_AddressV;
    samplerDesc.AddressW = params.m_AddressW;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = params.m_ZFunc;
    samplerDesc.BorderColor[0] = 1.0f;
    samplerDesc.BorderColor[1] = 1.0f;
    samplerDesc.BorderColor[2] = 1.0f;
    samplerDesc.BorderColor[3] = 1.0f;
    samplerDesc.MinLOD = -D3D11_FLOAT32_MAX;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    auto result = m_Device->CreateSamplerState(&samplerDesc, &sampler->m_SamplerState);
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
    m_cbGlobalConsts.ViewProjectionMatrix = m_RenderContext.m_viewProjectionMatrix;
    //m_cbGlobalConsts.CameraPosition = glm::vec4(Camera::Get()->GetPosition(), 1);
    m_cbGlobalConsts.ViewProjectionMatrix2 = m_RenderContext.m_viewProjectionMatrix;
    m_cbGlobalConsts.ViewProjectionMatrix3 = m_RenderContext.m_viewProjectionMatrix;

    // set the shader constants
    SetVertexShaderConstants(m_GlobalConstants, 0, m_cbGlobalConsts);
}