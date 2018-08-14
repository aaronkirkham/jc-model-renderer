#pragma once

#include <d3d11.h>
#include <cstdint>
#include <vector>

class Renderer;
struct RenderContext_t
{
    Renderer* m_Renderer = nullptr;
    ID3D11Device* m_Device = nullptr;
    ID3D11DeviceContext* m_DeviceContext = nullptr;
    D3D11_FILL_MODE m_FillMode = D3D11_FILL_SOLID;
    D3D11_CULL_MODE m_CullMode = D3D11_CULL_BACK;
    bool m_RasterIsDirty = true;
    bool m_BlendIsDirty = true;
    bool m_BlendEnabled = true;
    bool m_AlphaEnabled = true;
    D3D11_BLEND m_BlendSourceColour = D3D11_BLEND_SRC_ALPHA;
    D3D11_BLEND m_BlendSourceAlpha = D3D11_BLEND_ONE;
    D3D11_BLEND m_BlendDestColour = D3D11_BLEND_INV_SRC_ALPHA;
    D3D11_BLEND m_BlendDestAlpha = D3D11_BLEND_ZERO;
    D3D11_BLEND_OP m_BlendColourEq = D3D11_BLEND_OP_ADD;
    D3D11_BLEND_OP m_BlendAlphaEq = D3D11_BLEND_OP_ADD;
    glm::mat4 m_viewProjectionMatrix;
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

struct SamplerStateParams_t
{
    D3D11_TEXTURE_ADDRESS_MODE m_AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    D3D11_TEXTURE_ADDRESS_MODE m_AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    D3D11_TEXTURE_ADDRESS_MODE m_AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    float m_MinMip = 0.0f;
    float m_MaxMip = 1.0f;
    D3D11_COMPARISON_FUNC m_ZFunc = D3D11_COMPARISON_NEVER;
};

struct BoundingBox
{
    glm::vec3 m_Min;
    glm::vec3 m_Max;
    float m_Scale = 1.0f;

    BoundingBox()
        : m_Min({}),
        m_Max({})
    {
    }

    BoundingBox(const glm::vec3& min, const glm::vec3& max)
        : m_Min(min),
        m_Max(max)
    {
    }

    inline void SetScale(const float scale)
    {
        m_Scale = scale;
    }

    inline glm::vec3 GetCenter() const
    {
        return ((m_Max * m_Scale) + (m_Min * m_Scale)) * 0.5f;
    }

    inline glm::vec3 GetSize() const
    {
        return ((m_Max * m_Scale) - (m_Min * m_Scale));
    }
};