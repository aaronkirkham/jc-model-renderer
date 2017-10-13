#pragma once

#include <StdInc.h>
#include <singleton.h>
#include <graphics/Types.h>
#include <imgui.h>
#include <examples/directx11_example/imgui_impl_dx11.h>

#define test_hresult(hr) if (FAILED(hr)) { return false; }
#define safe_release(obj) if (obj) { obj->Release(); obj = 0; }

struct RenderEvents
{
    ksignals::Event<void(RenderContext_t*)> RenderFrame;
};

struct LightConstants
{
    glm::vec4 diffuseColour;
    glm::vec3 lightDirection;
    float padding;
};

class Renderer : public Singleton<Renderer>
{
private:
    RenderContext_t m_RenderContext;
    RenderEvents m_RenderEvents;

    ID3D11Device* m_Device = nullptr;
    ID3D11DeviceContext* m_DeviceContext = nullptr;
    IDXGISwapChain* m_SwapChain = nullptr;
    ID3D11RenderTargetView* m_RenderTargetView = nullptr;
    ID3D11RasterizerState* m_RasterizerState = nullptr;
    ID3D11Texture2D* m_DepthTexture = nullptr;
    ID3D11DepthStencilState* m_DepthStencilEnabledState = nullptr;
    ID3D11DepthStencilState* m_DepthStencilDisabledState = nullptr;
    ID3D11DepthStencilView* m_DepthStencilView = nullptr;
    ID3D11SamplerState* m_SamplerState = nullptr;
    ID3D11BlendState* m_BlendState = nullptr;

    ID3D11Texture2D* CreateTexture2D(const glm::vec2& size, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM, uint32_t bindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);

    void CreateDevice(const HWND& hwnd, const glm::vec2& size);
    void CreateRenderTarget(const glm::vec2& size);
    void CreateDepthStencil(const glm::vec2& size);
    void CreateBlendState();
    void CreateRasterizerState();
    //void CreateSamplerState();

    void DestroyRenderTarget();
    void DestroyDepthStencil();

public:
    Renderer();
    ~Renderer();

    virtual RenderEvents& Events() { return m_RenderEvents; }

    bool Initialise(const HWND& hwnd);
    void Shutdown();

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
    VertexBuffer_t* CreateVertexBuffer(uint32_t count, uint32_t stride);
    VertexBuffer_t* CreateVertexBuffer(const void* data, uint32_t count, uint32_t stride, D3D11_USAGE usage = D3D11_USAGE_DEFAULT);
    IndexBuffer_t* CreateIndexBuffer(const void* data, uint32_t count, D3D11_USAGE usage = D3D11_USAGE_DEFAULT);

    template <typename T>
    ConstantBuffer_t* CreateConstantBuffer(T& data)
    {
        auto buffer = new ConstantBuffer_t;
        buffer->m_ElementCount = 1;
        buffer->m_ElementStride = sizeof(T);
        buffer->m_Usage = D3D11_USAGE_DYNAMIC;

        D3D11_BUFFER_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth = sizeof(T);
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        D3D11_SUBRESOURCE_DATA resourceData;
        ZeroMemory(&resourceData, sizeof(resourceData));
        resourceData.pSysMem = &data;

        auto result = m_Device->CreateBuffer(&desc, &resourceData, &buffer->m_Buffer);
        assert(SUCCEEDED(result));

        if (FAILED(result)) {
            delete buffer;
            return nullptr;
        }

        return buffer;
    }
    
    void DestroyBuffer(IBuffer_t* buffer);

    template <typename T>
    void MapBuffer(ID3D11Buffer* buffer, T& data)
    {
        D3D11_MAPPED_SUBRESOURCE mapping;
        m_DeviceContext->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapping);

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

    // vertex declarations
    VertexDeclaration_t* CreateVertexDeclaration(const D3D11_INPUT_ELEMENT_DESC* layout, uint32_t count, VertexShader_t* m_Shader);
    void DestroyVertexDeclaration(VertexDeclaration_t* declaration);

    // samplers
    SamplerState_t* CreateSamplerState(const SamplerStateCreationParams_t& params);
    void DestroySamplerState(SamplerState_t* sampler);

    ID3D11Device* GetDevice() { return m_Device; }
    ID3D11DeviceContext* GetDeviceContext() { return m_DeviceContext; }
    IDXGISwapChain* GetSwapChain() { return m_SwapChain; }
};