#include <graphics/Renderer.h>
#include <graphics/ShaderManager.h>
#include <graphics/UI.h>
#include <graphics/Camera.h>
#include <graphics/DebugRenderer.h>
#include <Window.h>

void SetupImGuiStyle();

ConstantBuffer_t* m_LightBuffers = nullptr;

Renderer::Renderer()
{
    Window::Get()->Events().WindowResized.connect([this](const glm::vec2& size) {
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

ID3D11Texture2D* Renderer::CreateTexture2D(const glm::vec2& size, DXGI_FORMAT format, uint32_t bindFlags)
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

    // setup lighting
    {
        LightConstants constants;
        m_LightBuffers = CreateConstantBuffer(constants);
    }

    // initialise imgui
    ImGui_ImplDX11_Init(hwnd, m_Device, m_DeviceContext);
    SetupImGuiStyle();

    // setup the render context
    m_RenderContext.m_Device = m_Device;
    m_RenderContext.m_DeviceContext = m_DeviceContext;

    DEBUG_LOG("Renderer is ready!");
    return true;
}

void Renderer::Shutdown()
{
    if (m_SwapChain) {
        m_SwapChain->SetFullscreenState(false, nullptr);
    }

    ImGui_ImplDX11_Shutdown();

    m_DeviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    m_DeviceContext->RSSetState(nullptr);

    DestroyRenderTarget();
    DestroyDepthStencil();

    safe_release(m_BlendState);
    safe_release(m_RasterizerState);
    safe_release(m_SamplerState);
    safe_release(m_DepthTexture);
    safe_release(m_DeviceContext);
    safe_release(m_Device);
    safe_release(m_SwapChain);
}

bool Renderer::Render()
{
    ImGui_ImplDX11_NewFrame();

    ID3D11ShaderResourceView* nullViews[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = { 0 };
    m_DeviceContext->PSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nullViews);

    // begin scene
    {
        static const float colour[4] = { 0.15f, 0.15f, 0.15f, 1.0f };

        m_DeviceContext->ClearRenderTargetView(m_RenderTargetView, colour);
        m_DeviceContext->ClearDepthStencilView(m_DepthStencilView, (D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL), 1.0f, 0);
    }

    // lighting
    {
        LightConstants constants;
        constants.position = Camera::Get()->GetPosition();//glm::vec3(0, 1, 0);
        constants.direction = glm::vec3();
        constants.diffuseColour = glm::vec4(1, 1, 1, 1);
        //constants.lightDirection = glm::vec3(0.25f, -2.5f, -1.0f);
        //constants.lightDirection = glm::vec3(0, 0, -1.0f);
        //constants.padding = 0.0f;

        SetPixelShaderConstants(m_LightBuffers, 0, constants);
    }

    // update the camera
    Camera::Get()->Update();

    m_RenderEvents.RenderFrame(&m_RenderContext);

    SetDepthEnabled(false);

    // Render 2D
    DebugRenderer::Get()->Render(&m_RenderContext);

    // Render UI
    UI::Get()->Render();

    // end scene
    ImGui::Render();

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
    // create render target back buffer
    ID3D11Texture2D* backBuffer = nullptr;
    auto result = m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
    assert(SUCCEEDED(result));

    result = m_Device->CreateRenderTargetView(backBuffer, nullptr, &m_RenderTargetView);
    assert(SUCCEEDED(result));

    backBuffer->Release();
    m_DeviceContext->OMSetRenderTargets(1, &m_RenderTargetView, m_DepthStencilView);

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
#ifdef DEBUG
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    auto result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, deviceFlags, nullptr, 0, D3D11_SDK_VERSION, &desc, &m_SwapChain, &m_Device, nullptr, &m_DeviceContext);
    assert(SUCCEEDED(result));
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

        depthStencilDesc.DepthEnable = false;
        depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

        result = m_Device->CreateDepthStencilState(&depthStencilDesc, &m_DepthStencilDisabledState);
        assert(SUCCEEDED(result));

        // enable depth
        SetDepthEnabled(true);
    }

    // create the depth buffer
    m_DepthTexture = CreateTexture2D(size, DXGI_FORMAT_R24G8_TYPELESS, D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE);

    // create depth stencil view
    {
        D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
        depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        depthStencilViewDesc.Texture2D.MipSlice = 0;
        depthStencilViewDesc.Flags = 0;

        auto result = m_Device->CreateDepthStencilView(m_DepthTexture, &depthStencilViewDesc, &m_DepthStencilView);
        assert(SUCCEEDED(result));
    }
}

void Renderer::CreateBlendState()
{
    D3D11_BLEND_DESC blendDesc;
    ZeroMemory(&blendDesc, sizeof(blendDesc));

    D3D11_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc;
    ZeroMemory(&renderTargetBlendDesc, sizeof(renderTargetBlendDesc));

    renderTargetBlendDesc.BlendEnable = true;
    renderTargetBlendDesc.SrcBlend = D3D11_BLEND_SRC_COLOR;
    renderTargetBlendDesc.DestBlend = D3D11_BLEND_BLEND_FACTOR;
    renderTargetBlendDesc.BlendOp = D3D11_BLEND_OP_ADD;
    renderTargetBlendDesc.SrcBlendAlpha = D3D11_BLEND_ONE;
    renderTargetBlendDesc.DestBlendAlpha = D3D11_BLEND_ZERO;
    renderTargetBlendDesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
    renderTargetBlendDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    blendDesc.AlphaToCoverageEnable = false;
    blendDesc.RenderTarget[0] = renderTargetBlendDesc;

    auto result = m_Device->CreateBlendState(&blendDesc, &m_BlendState);
    assert(SUCCEEDED(result));
}

void Renderer::CreateRasterizerState()
{
    if (m_RenderContext.m_RasterIsDirty) {
        safe_release(m_RasterizerState);

        D3D11_RASTERIZER_DESC rasterDesc;
        ZeroMemory(&rasterDesc, sizeof(D3D11_RASTERIZER_DESC));

        rasterDesc.FillMode = m_RenderContext.m_FillMode;
        rasterDesc.CullMode = m_RenderContext.m_CullMode;
        rasterDesc.DepthClipEnable = true;

        auto result = m_Device->CreateRasterizerState(&rasterDesc, &m_RasterizerState);
        assert(SUCCEEDED(result));

        m_DeviceContext->RSSetState(m_RasterizerState);

        m_RenderContext.m_RasterIsDirty = false;
    }
}

void Renderer::DestroyRenderTarget()
{
    m_DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    safe_release(m_RenderTargetView);
}

void Renderer::DestroyDepthStencil()
{
    m_DeviceContext->OMSetDepthStencilState(nullptr, 0);

    safe_release(m_DepthStencilEnabledState);
    safe_release(m_DepthStencilDisabledState);
    safe_release(m_DepthStencilView);
}

VertexBuffer_t* Renderer::CreateVertexBuffer(uint32_t count, uint32_t stride)
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

    if (FAILED(result)) {
        delete buffer;
        return nullptr;
    }

    // TODO: create SRV if we need it.

    return buffer;
}

VertexBuffer_t* Renderer::CreateVertexBuffer(const void* data, uint32_t count, uint32_t stride, D3D11_USAGE usage)
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

    if (FAILED(result)) {
        delete buffer;
        return nullptr;
    }

    // TODO: create SRV if we need it.

    return buffer;
}

IndexBuffer_t* Renderer::CreateIndexBuffer(const void* data, uint32_t count, D3D11_USAGE usage)
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

    if (FAILED(result)) {
        delete buffer;
        return nullptr;
    }

    // TODO: create SRV if we need it.

    return buffer;
}

void Renderer::DestroyBuffer(IBuffer_t* buffer)
{
    if (buffer) {
        safe_release(buffer->m_SRV);
        safe_release(buffer->m_Buffer);

        delete buffer;
        buffer = nullptr;
    }
}

VertexDeclaration_t* Renderer::CreateVertexDeclaration(const D3D11_INPUT_ELEMENT_DESC* layout, uint32_t count, VertexShader_t* m_Shader)
{
    auto declaration = new VertexDeclaration_t;

    auto result = m_Device->CreateInputLayout(layout, count, m_Shader->m_Code.data(), m_Shader->m_Size, &declaration->m_Layout);
    assert(SUCCEEDED(result));

    return declaration;
}

void Renderer::DestroyVertexDeclaration(VertexDeclaration_t* declaration)
{
    if (declaration) {
        safe_release(declaration->m_Layout);

        delete declaration;
        declaration = nullptr;
    }
}

SamplerState_t* Renderer::CreateSamplerState(const SamplerStateCreationParams_t& params)
{
    auto sampler = new SamplerState_t;

    D3D11_SAMPLER_DESC samplerDesc;
    ZeroMemory(&samplerDesc, sizeof(D3D11_SAMPLER_DESC));

    samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.BorderColor[0] = 1.0f;
    samplerDesc.BorderColor[1] = 1.0f;
    samplerDesc.BorderColor[2] = 1.0f;
    samplerDesc.BorderColor[3] = 1.0f;
    samplerDesc.MinLOD = -D3D11_FLOAT32_MAX;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    auto result = m_Device->CreateSamplerState(&samplerDesc, &sampler->m_SamplerState);
    assert(SUCCEEDED(result));

    if (FAILED(result)) {
        delete sampler;
        return nullptr;
    }

    return sampler;
}

void Renderer::DestroySamplerState(SamplerState_t* sampler)
{
    if (sampler) {
        safe_release(sampler->m_SamplerState);

        delete sampler;
        sampler = nullptr;
    }
}