#pragma once

#include <d3d11.h>
#include <cstdint>
#include <vector>

struct RenderContext_t
{
    ID3D11Device* m_Device = nullptr;
    ID3D11DeviceContext* m_DeviceContext = nullptr;
    D3D11_FILL_MODE m_FillMode = D3D11_FILL_SOLID;
    D3D11_CULL_MODE m_CullMode = D3D11_CULL_BACK;
    bool m_RasterIsDirty = true;
    bool m_BlendEnabled = true;
    D3D11_BLEND m_BlendMode;
};

struct IBuffer_t
{
    ID3D11Buffer* m_Buffer = nullptr;
    ID3D11ShaderResourceView* m_SRV = nullptr;
    uint32_t m_ElementCount = 0;
    uint32_t m_ElementStride = 0;
    D3D11_USAGE m_Usage = D3D11_USAGE_DEFAULT;
};

struct VertexBuffer_t : IBuffer_t {};
struct IndexBuffer_t : IBuffer_t {};
struct ConstantBuffer_t : IBuffer_t {};

struct VertexShader_t
{
    virtual ~VertexShader_t() {
        m_Shader->Release();
        m_Shader = nullptr;
    }

    ID3D11VertexShader* m_Shader = nullptr;
    FileBuffer m_Code;
    uint64_t m_Size = 0;
};

struct PixelShader_t
{
    virtual ~PixelShader_t() {
        m_Shader->Release();
        m_Shader = nullptr;
    }

    ID3D11PixelShader* m_Shader = nullptr;
    FileBuffer m_Code;
    uint64_t m_Size = 0;
};

struct VertexDeclaration_t
{
    ID3D11InputLayout* m_Layout = nullptr;
};

struct SamplerState_t
{
    ID3D11SamplerState* m_SamplerState = nullptr;
};

struct SamplerStateCreationParams_t
{
    D3D11_TEXTURE_ADDRESS_MODE m_AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    D3D11_TEXTURE_ADDRESS_MODE m_AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    D3D11_TEXTURE_ADDRESS_MODE m_AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    float m_MinMip = 0.0f;
    float m_MaxMip = 1.0f;
};