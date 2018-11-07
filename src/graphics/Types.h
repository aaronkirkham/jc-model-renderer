#pragma once

#include <cstdint>
#include <d3d11.h>
#include <vector>

using floats_t    = std::vector<float>;
using uint16s_t   = std::vector<uint16_t>;
using materials_t = std::vector<fs::path>;

class Renderer;
struct RenderContext_t {
    Renderer*            m_Renderer          = nullptr;
    ID3D11Device*        m_Device            = nullptr;
    ID3D11DeviceContext* m_DeviceContext     = nullptr;
    D3D11_FILL_MODE      m_FillMode          = D3D11_FILL_SOLID;
    D3D11_CULL_MODE      m_CullMode          = D3D11_CULL_BACK;
    bool                 m_RasterIsDirty     = true;
    bool                 m_BlendIsDirty      = true;
    bool                 m_AlphaBlendEnabled = true;
    bool                 m_AlphaTestEnabled  = true;
    D3D11_BLEND          m_BlendSourceColour = D3D11_BLEND_SRC_ALPHA;
    D3D11_BLEND          m_BlendSourceAlpha  = D3D11_BLEND_ONE;
    D3D11_BLEND          m_BlendDestColour   = D3D11_BLEND_INV_SRC_ALPHA;
    D3D11_BLEND          m_BlendDestAlpha    = D3D11_BLEND_ZERO;
    D3D11_BLEND_OP       m_BlendColourEq     = D3D11_BLEND_OP_ADD;
    D3D11_BLEND_OP       m_BlendAlphaEq      = D3D11_BLEND_OP_ADD;
    glm::mat4            m_viewProjectionMatrix;
};

enum BufferType { VERTEX_BUFFER = 1, INDEX_BUFFER = 2, NUM_BUFFER_TYPES = 3 };

struct IBuffer_t {
    ID3D11Buffer* m_Buffer = nullptr;
    FileBuffer    m_Data;
    uint32_t      m_ElementCount  = 0;
    uint32_t      m_ElementStride = 0;
    BufferType    m_Type          = NUM_BUFFER_TYPES;
    D3D11_USAGE   m_Usage         = D3D11_USAGE_DEFAULT;

    template <typename T> std::vector<T> CastData()
    {
        return {reinterpret_cast<T*>(m_Data.data()), reinterpret_cast<T*>(m_Data.data() + m_Data.size())};
    }
};

using VertexBuffer_t   = IBuffer_t;
using IndexBuffer_t    = IBuffer_t;
using ConstantBuffer_t = IBuffer_t;

struct VertexShader_t {
    virtual ~VertexShader_t()
    {
        m_Shader->Release();
        m_Shader = nullptr;
    }

    ID3D11VertexShader* m_Shader = nullptr;
    FileBuffer          m_Code;
    uint64_t            m_Size = 0;
};

struct PixelShader_t {
    virtual ~PixelShader_t()
    {
        m_Shader->Release();
        m_Shader = nullptr;
    }

    ID3D11PixelShader* m_Shader = nullptr;
    FileBuffer         m_Code;
    uint64_t           m_Size = 0;
};

struct VertexDeclaration_t {
    ID3D11InputLayout* m_Layout = nullptr;
};

struct SamplerState_t {
    ID3D11SamplerState* m_SamplerState = nullptr;
};

struct Ray {
    glm::vec3 m_Origin;
    glm::vec3 m_Direction;
    glm::vec3 m_InvDirection;

    Ray(const glm::vec3& origin, const glm::vec3& direction)
        : m_Origin(origin)
        , m_Direction(direction)
    {
        m_InvDirection = (1.0f / direction);
    }
};

struct BoundingBox {
    glm::vec3 m_Min;
    glm::vec3 m_Max;
    float     m_Scale = 1.0f;

    BoundingBox()
        : m_Min({})
        , m_Max({})
    {
    }

    BoundingBox(const glm::vec3& min, const glm::vec3& max)
        : m_Min(min)
        , m_Max(max)
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

    inline glm::vec3 GetMin() const
    {
        return (m_Min * m_Scale);
    }

    inline glm::vec3 GetMax() const
    {
        return (m_Max * m_Scale);
    }

    /*
    // If line is parallel and outsite the box it is not possible to intersect
    if (direction.x == 0.0f && (origin.x < min(leftBottom.x, rightTop.x) || origin.x > max(leftBottom.x, rightTop.x)))
    return false;

    if (direction.y == 0.0f && (origin.y < min(leftBottom.y, rightTop.y) || origin.y > max(leftBottom.y, rightTop.y)))
    return false;

    if (direction.z == 0.0f && (origin.z < min(leftBottom.z, rightTop.z) || origin.z > max(leftBottom.z, rightTop.z)))
    return false;
    */

    bool Intersect(const Ray& ray, float* distance) const
    {
        const auto min = (m_Min * m_Scale);
        const auto max = (m_Max * m_Scale);

        auto t1 = (min.x - ray.m_Origin.x) * ray.m_InvDirection.x;
        auto t2 = (max.x - ray.m_Origin.x) * ray.m_InvDirection.x;
        auto t3 = (min.y - ray.m_Origin.y) * ray.m_InvDirection.y;
        auto t4 = (max.y - ray.m_Origin.y) * ray.m_InvDirection.y;
        auto t5 = (min.z - ray.m_Origin.z) * ray.m_InvDirection.z;
        auto t6 = (max.z - ray.m_Origin.z) * ray.m_InvDirection.z;

        auto tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
        auto tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

        if (tmax < 0) {
            *distance = tmax;
            return false;
        }

        if (tmin > tmax) {
            *distance = tmax;
            return false;
        }

        *distance = tmin;
        return true;
    }
};
