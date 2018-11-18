#pragma once

#include <StdInc.h>
#include <array>
#include <graphics/Types.h>
#include <imgui.h>
#include <singleton.h>

#include <examples/imgui_impl_dx11.h>
#include <examples/imgui_impl_win32.h>

#define SAFE_DELETE(ptr)                                                                                               \
    if (ptr) {                                                                                                         \
        delete ptr;                                                                                                    \
        ptr = nullptr;                                                                                                 \
    }
#define SAFE_RELEASE(obj)                                                                                              \
    if (obj) {                                                                                                         \
        obj->Release();                                                                                                \
        obj = nullptr;                                                                                                 \
    }

static auto g_DefaultClearColour = glm::vec4{0.15f, 0.15f, 0.15f, 1.0f};

struct RenderEvents {
    ksignals::Event<void(RenderContext_t*)> PreRender;
    ksignals::Event<void(RenderContext_t*)> PostRender;
};

struct VertexGlobalConstants {
    glm::mat4 ViewProjectionMatrix;  // 0 - 4
    glm::vec4 CameraPosition;        // 4 - 5
    glm::vec4 _unknown[2];           // 5 - 7
    glm::vec4 LightDiffuseColour;    // 7 - 8
    glm::vec4 LightDirection;        // 8 - 9
    glm::vec4 _unknown2[20];         // 9 - 29
    glm::mat4 ViewProjectionMatrix2; // 29 - 33
    glm::mat4 ViewProjectionMatrix3; // 33 - 37
    glm::vec4 _unknown3[12];         // 37 - 49
};

struct FragmentGlobalConstants {
    glm::vec4 _unknown;            // 0 - 1
    glm::vec4 LightSpecularColour; // 1 - 2
    glm::vec4 LightDiffuseColour;  // 2 - 3
    glm::vec4 LightDirection;      // 3 - 4
    glm::vec4 _unknown2;           // 4 - 5
    glm::vec4 CameraPosition;      // 5 - 6
    glm::vec4 _unknown3[25];       // 6 - 31
    glm::vec4 LightDirection2;     // 31 - 32
    glm::vec4 _unknown4[63];       // 32 - 95
};

struct AlphaTestConstants {
    float AlphaMulRef[4] = {50.0f, -1.0f, 50.0f, -1.0f};
};

class IRenderBlock;
class Renderer : public Singleton<Renderer>
{
  private:
    RenderContext_t m_RenderContext;
    RenderEvents    m_RenderEvents;

    ID3D11Device*                            m_Device                    = nullptr;
    ID3D11DeviceContext*                     m_DeviceContext             = nullptr;
    IDXGISwapChain*                          m_SwapChain                 = nullptr;
    ID3D11RenderTargetView*                  m_BackBuffer                = nullptr;
    std::array<ID3D11RenderTargetView*, 4>   m_GBuffer                   = {nullptr};
    std::array<ID3D11ShaderResourceView*, 4> m_GBufferSRV                = {nullptr};
    ID3D11RasterizerState*                   m_RasterizerState           = nullptr;
    ID3D11Texture2D*                         m_DepthTexture              = nullptr;
    ID3D11DepthStencilState*                 m_DepthStencilEnabledState  = nullptr;
    ID3D11DepthStencilState*                 m_DepthStencilDisabledState = nullptr;
    ID3D11DepthStencilView*                  m_DepthStencilView          = nullptr;
    ID3D11BlendState*                        m_BlendState                = nullptr;

    std::array<ConstantBuffer_t*, 2> m_GlobalConstants    = {nullptr};
    ConstantBuffer_t*                m_AlphaTestConstants = nullptr;
    VertexGlobalConstants            m_cbVertexGlobalConsts;
    FragmentGlobalConstants          m_cbFragmentGlobalConsts;
    AlphaTestConstants               m_cbAlphaTestConsts;

    glm::vec4 m_ClearColour = g_DefaultClearColour;

    std::recursive_mutex       m_RenderListMutex;
    std::vector<IRenderBlock*> m_RenderList;

#ifdef DEBUG
    ID3D11Debug* m_DeviceDebugger = nullptr;
#endif

    ID3D11Texture2D* CreateTexture2D(const glm::vec2& size, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM,
                                     uint32_t    bindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
                                     const char* debugName = nullptr);

    void CreateDevice(const HWND& hwnd, const glm::vec2& size);
    void CreateRenderTarget(const glm::vec2& size);
    void CreateGBuffer(const glm::vec2& size);
    void CreateDepthStencil(const glm::vec2& size);
    void CreateBlendState();
    void CreateRasterizerState();

    void DestroyRenderTargets();
    void DestroyGBuffer();
    void DestroyDepthStencil();

    void UpdateGlobalConstants();

  public:
    Renderer();
    virtual ~Renderer();

    virtual RenderEvents& Events()
    {
        return m_RenderEvents;
    }

    bool Initialise(const HWND& hwnd);
    void Shutdown();

    void SetResolution(const glm::vec2& size);
    void SetGBufferResolution(const glm::vec2& size);

    void SetClearColour(const glm::vec4& colour)
    {
        m_ClearColour = colour;
    }

    const glm::vec4& GetClearColour()
    {
        return m_ClearColour;
    }

    bool Render(const float delta_time);

    void SetupRenderStates();

    void SetDefaultRenderStates();

    void Draw(uint32_t first_vertex, uint32_t vertex_count);
    void DrawIndexed(uint32_t start_index, uint32_t index_count);
    void DrawIndexed(uint32_t start_index, uint32_t index_count, IndexBuffer_t* buffer);

    void SetBlendingEnabled(bool state);
    void SetBlendingFunc(D3D11_BLEND source_colour, D3D11_BLEND source_alpha, D3D11_BLEND dest_colour,
                         D3D11_BLEND dest_alpha);
    void SetBlendingEq(D3D11_BLEND_OP colour, D3D11_BLEND_OP alpha);

    void SetDepthEnabled(bool state);
    void SetAlphaTestEnabled(bool state);

    void SetFillMode(D3D11_FILL_MODE mode);
    void SetCullMode(D3D11_CULL_MODE mode);

    // buffers
    IBuffer_t* CreateBuffer(const void* data, uint32_t count, uint32_t stride, BufferType type,
                            const char* debugName = nullptr);

    template <typename T> ConstantBuffer_t* CreateConstantBuffer(T& data, const char* debugName = nullptr)
    {
        return CreateConstantBuffer(data, sizeof(T) / 16, debugName);
    }

    template <typename T>
    ConstantBuffer_t* CreateConstantBuffer(T& data, int32_t vec4count, const char* debugName = nullptr)
    {
        assert(vec4count > 0);

        auto buffer             = new ConstantBuffer_t;
        buffer->m_ElementCount  = 1;
        buffer->m_ElementStride = 16 * vec4count;
        buffer->m_Usage         = D3D11_USAGE_DYNAMIC;

        D3D11_BUFFER_DESC desc{};
        desc.Usage          = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth      = 16 * vec4count;
        desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        D3D11_SUBRESOURCE_DATA resourceData{};
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

    template <typename T> void MapBuffer(ID3D11Buffer* buffer, T& data)
    {
        D3D11_MAPPED_SUBRESOURCE mapping;
        auto                     hr = m_DeviceContext->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapping);
        assert(SUCCEEDED(hr));

        memcpy(mapping.pData, &data, sizeof(T));

        m_DeviceContext->Unmap(buffer, 0);
    }

    template <typename T> void SetVertexShaderConstants(ConstantBuffer_t* buffer, uint32_t slot, T& data)
    {
        MapBuffer(buffer->m_Buffer, data);
        m_DeviceContext->VSSetConstantBuffers(slot, 1, &buffer->m_Buffer);
    }

    template <typename T> void SetPixelShaderConstants(ConstantBuffer_t* buffer, uint32_t slot, T& data)
    {
        MapBuffer(buffer->m_Buffer, data);
        m_DeviceContext->PSSetConstantBuffers(slot, 1, &buffer->m_Buffer);
    }

    void SetVertexStream(VertexBuffer_t* buffer, int32_t slot, uint32_t offset = 0);
    void SetSamplerState(SamplerState_t* sampler, int32_t slot);
    void ClearTexture(int32_t slot);
    void ClearTextures(int32_t first, int32_t last);

    // vertex declarations
    VertexDeclaration_t* CreateVertexDeclaration(const D3D11_INPUT_ELEMENT_DESC* layout, uint32_t count,
                                                 VertexShader_t* shader, const char* debugName = nullptr);
    void                 DestroyVertexDeclaration(VertexDeclaration_t* declaration);

    // samplers
    SamplerState_t* CreateSamplerState(const D3D11_SAMPLER_DESC& params, const char* debugName = nullptr);
    void            DestroySamplerState(SamplerState_t* sampler);

    // render list
    void AddToRenderList(const std::vector<IRenderBlock*>& renderblocks);
    void AddToRenderList(IRenderBlock* renderblock);
    void RemoveFromRenderList(const std::vector<IRenderBlock*>& renderblocks);

    ID3D11Device* GetDevice()
    {
        return m_Device;
    }

    ID3D11DeviceContext* GetDeviceContext()
    {
        return m_DeviceContext;
    }

    IDXGISwapChain* GetSwapChain()
    {
        return m_SwapChain;
    }

    ID3D11ShaderResourceView* GetGBufferSRV(uint8_t index)
    {
        return m_GBufferSRV[index];
    }
};
