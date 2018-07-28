#pragma once

#include <StdInc.h>
#include <singleton.h>
#include <graphics/Types.h>
#include <imgui.h>
#include <examples/imgui_impl_win32.h>
#include <examples/imgui_impl_dx11.h>
#include <array>

#define SAFE_DELETE(ptr) if (ptr) { delete ptr; ptr = nullptr; }
#define SAFE_RELEASE(obj) if (obj) { obj->Release(); obj = nullptr; }

static auto g_DefaultClearColour = glm::vec4{ 0.15f, 0.15f, 0.15f, 1.0f };

struct RenderEvents
{
    ksignals::Event<void(RenderContext_t*)> RenderFrame;
};

struct GlobalConstants
{
    glm::mat4 ViewProjectionMatrix;         // 0
    glm::vec4 CameraPosition;               // 4
    glm::vec4 _unknown[24];                 // 5
    glm::mat4 ViewProjectionMatrix2;        // 29
    glm::mat4 ViewProjectionMatrix3;        // 33
    glm::vec4 _unknown2[12];                // 37
};

class Renderer : public Singleton<Renderer>
{
private:
    RenderContext_t m_RenderContext;
    RenderEvents m_RenderEvents;

    ID3D11Device* m_Device = nullptr;
    ID3D11DeviceContext* m_DeviceContext = nullptr;
    IDXGISwapChain* m_SwapChain = nullptr;
    std::array<ID3D11RenderTargetView*, 3> m_RenderTargetView = { nullptr };
    std::array<ID3D11ShaderResourceView*, 2> m_RenderTargetResourceView = { nullptr };
    ID3D11RasterizerState* m_RasterizerState = nullptr;
    ID3D11Texture2D* m_DepthTexture = nullptr;
    ID3D11DepthStencilState* m_DepthStencilEnabledState = nullptr;
    ID3D11DepthStencilState* m_DepthStencilDisabledState = nullptr;
    ID3D11DepthStencilView* m_DepthStencilView = nullptr;
    ID3D11SamplerState* m_SamplerState = nullptr;
    ID3D11BlendState* m_BlendState = nullptr;

    ConstantBuffer_t* m_GlobalConstants = nullptr;
    GlobalConstants m_cbGlobalConsts;

    glm::vec4 m_ClearColour = g_DefaultClearColour;

#ifdef DEBUG
    ID3D11Debug* m_DeviceDebugger = nullptr;
#endif

    ID3D11Texture2D* CreateTexture2D(const glm::vec2& size, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM, uint32_t bindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);

    void CreateDevice(const HWND& hwnd, const glm::vec2& size);
    void CreateRenderTarget(const glm::vec2& size);
    void CreateDepthStencil(const glm::vec2& size);
    void CreateBlendState();
    void CreateRasterizerState();
    //void CreateSamplerState();

    void DestroyRenderTarget();
    void DestroyDepthStencil();

    void UpdateGlobalConstants();

public:
    Renderer();
    ~Renderer();

    virtual RenderEvents& Events() { return m_RenderEvents; }

    bool Initialise(const HWND& hwnd);
    void Shutdown();

    void SetClearColour(const glm::vec4& colour) { m_ClearColour = colour; }
    const glm::vec4& GetClearColour() { return m_ClearColour; }

    bool Render();

    void SetupRenderStates();

    void SetDefaultRenderStates();

    void Draw(uint32_t first_vertex, uint32_t vertex_count);
    void DrawIndexed(uint32_t start_index, uint32_t index_count);
    void DrawIndexed(uint32_t start_index, uint32_t index_count, IndexBuffer_t* buffer);

    void SetBlendingEnabled(bool state);
    void SetDepthEnabled(bool state);

    void SetFillMode(D3D11_FILL_MODE mode);
    void SetCullMode(D3D11_CULL_MODE mode);

    // buffers
    VertexBuffer_t* CreateVertexBuffer(uint32_t count, uint32_t stride, const char* debugName = nullptr);
    VertexBuffer_t* CreateVertexBuffer(const void* data, uint32_t count, uint32_t stride, D3D11_USAGE usage = D3D11_USAGE_DEFAULT, const char* debugName = nullptr);
    IndexBuffer_t* CreateIndexBuffer(const void* data, uint32_t count, D3D11_USAGE usage = D3D11_USAGE_DEFAULT, const char* debugName = nullptr);

    template <typename T>
    ConstantBuffer_t* CreateConstantBuffer(T& data, const char* debugName = nullptr)
    {
        return CreateConstantBuffer(data, sizeof(T) / 16, debugName);
    }

    template <typename T>
    ConstantBuffer_t* CreateConstantBuffer(T& data, int32_t vec4count, const char* debugName = nullptr)
    {
        auto buffer = new ConstantBuffer_t;
        buffer->m_ElementCount = 1;
        buffer->m_ElementStride = 16 * vec4count;
        buffer->m_Usage = D3D11_USAGE_DYNAMIC;

        D3D11_BUFFER_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth = 16 * vec4count;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        D3D11_SUBRESOURCE_DATA resourceData;
        ZeroMemory(&resourceData, sizeof(resourceData));
        resourceData.pSysMem = &data;

        auto result = m_Device->CreateBuffer(&desc, &resourceData, &buffer->m_Buffer);
        assert(SUCCEEDED(result));

#ifdef RENDERER_REPORT_LIVE_OBJECTS
        D3D_SET_OBJECT_NAME_A(buffer->m_Buffer, debugName);
#endif

        if (FAILED(result)) {
            SAFE_DELETE(buffer);
            return nullptr;
        }

        return buffer;
    }
    
    void DestroyBuffer(IBuffer_t* buffer);

    template <typename T>
    void MapBuffer(ID3D11Buffer* buffer, T& data)
    {
        D3D11_MAPPED_SUBRESOURCE mapping;
        auto hr = m_DeviceContext->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapping);
        assert(SUCCEEDED(hr));

        memcpy(mapping.pData, &data, sizeof(T));

        m_DeviceContext->Unmap(buffer, 0);
    }

    template <typename T>
    void SetVertexShaderConstants(ConstantBuffer_t* buffer, uint32_t slot, T& data)
    {
        MapBuffer(buffer->m_Buffer, data);
        m_DeviceContext->VSSetConstantBuffers(slot, 1, &buffer->m_Buffer);
    }

    template <typename T>
    void SetPixelShaderConstants(ConstantBuffer_t* buffer, uint32_t slot, T& data)
    {
        MapBuffer(buffer->m_Buffer, data);
        m_DeviceContext->PSSetConstantBuffers(slot, 1, &buffer->m_Buffer);
    }

    void SetVertexStream(VertexBuffer_t* buffer, int32_t slot, uint32_t offset = 0);
    void SetSamplerState(SamplerState_t* sampler, int32_t slot);

    // vertex declarations
    VertexDeclaration_t* CreateVertexDeclaration(const D3D11_INPUT_ELEMENT_DESC* layout, uint32_t count, VertexShader_t* m_Shader, const char* debugName = nullptr);
    void DestroyVertexDeclaration(VertexDeclaration_t* declaration);

    // samplers
    SamplerState_t* CreateSamplerState(const SamplerStateCreationParams_t& params, const char* debugName = nullptr);
    void DestroySamplerState(SamplerState_t* sampler);

    ID3D11Device* GetDevice() { return m_Device; }
    ID3D11DeviceContext* GetDeviceContext() { return m_DeviceContext; }
    IDXGISwapChain* GetSwapChain() { return m_SwapChain; }
};