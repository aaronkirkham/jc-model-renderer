#pragma once

#include <cstdint>

#pragma pack(push, 1)
namespace jc
{
struct SObjectNameHash {
    uint16_t m_First;
    uint16_t m_Second;
    uint16_t m_Third;
};

struct SObjectID {
    SObjectNameHash m_Hash;
    uint16_t        m_UserData;

    SObjectID() = default;
    SObjectID(const uint64_t object_id)
    {
        m_UserData      = object_id;
        m_Hash.m_First  = (object_id >> 0x30) & 0xFFFF;
        m_Hash.m_Second = (object_id >> 0x20) & 0xFFFF;
        m_Hash.m_Third  = (object_id >> 0x10) & 0xFFFF;
    }

    SObjectID(const uint64_t object_id, uint16_t user_data)
    {
        m_UserData      = user_data;
        m_Hash.m_First  = (object_id >> 0x30) & 0xFFFF;
        m_Hash.m_Second = (object_id >> 0x20) & 0xFFFF;
        m_Hash.m_Third  = (object_id >> 0x10) & 0xFFFF;
    }

    // format used in .xml files output by Ricks tools.
    std::string to_gibbed_str()
    {
        char buffer[18] = {0};
        sprintf_s(buffer, "%04X%04X=%04X%04X", m_Hash.m_Second, m_Hash.m_First, m_UserData, m_Hash.m_Third);
        return buffer;
    }

    std::string to_str()
    {
        char buffer[17] = {0};
        sprintf_s(buffer, "%016llx", to_uint64());
        return buffer;
    }

    uint64_t to_uint64()
    {
        return m_UserData
               | ((m_Hash.m_Third | ((m_Hash.m_Second | ((uint64_t)m_Hash.m_First << 0x10)) << 0x10)) << 0x10);
    }
};

using SEventID = SObjectID;
}; // namespace jc
namespace jc::Vertex
{
enum EPackedFormat {
    PACKED_FORMAT_FLOAT = 0,
    PACKED_FORMAT_INT16 = 1,
};

struct SPackedAttribute {
    int32_t   m_Format;
    float     m_Scale = 1.0f;
    glm::vec2 m_UV0Extent;
    glm::vec2 m_UV1Extent;
    float     m_ColourExtent;
    uint8_t   m_Colour[4];
};
static_assert(sizeof(SPackedAttribute) == 0x20, "SPackedAttribute alignment is wrong!");

struct PackedVertexPosition {
    int16_t x;
    int16_t y;
    int16_t z;
    int16_t pad;
};
static_assert(sizeof(PackedVertexPosition) == 0x8, "PackedVertexPosition alignment is wrong!");

struct UnpackedVertexPosition {
    float x;
    float y;
    float z;
};
static_assert(sizeof(UnpackedVertexPosition) == 0xC, "UnpackedVertexPosition alignment is wrong!");

struct UnpackedVertexPositionXYZW {
    float x;
    float y;
    float z;
    float w;
};
static_assert(sizeof(UnpackedVertexPositionXYZW) == 0x10, "UnpackedVertexPositionXYZW alignment is wrong!");

struct UnpackedVertexPosition2UV {
    float x;
    float y;
    float z;
    float u0;
    float v0;
    float u1;
    float v1;
    float n;
    float t;
    float col;
};
static_assert(sizeof(UnpackedVertexPosition2UV) == 0x28, "UnpackedVertexPosition2UV alignment is wrong!");

struct GeneralShortPacked {
    int16_t u0;
    int16_t v0;
    int16_t u1;
    int16_t v1;
    float   n;
    float   t;
    float   col;
};
static_assert(sizeof(GeneralShortPacked) == 0x14, "GeneralShortPacked alignment is wrong!");

struct UnpackedVertexWithNormal1 {
    float x;
    float y;
    float z;
    float n;
};
static_assert(sizeof(UnpackedVertexWithNormal1) == 0x10, "UnpackedVertexWithNormal1 alignment is wrong!");

struct PackedTex2UV {
    int16_t u0;
    int16_t v0;
    int16_t u1;
    int16_t v1;
};
static_assert(sizeof(PackedTex2UV) == 0x8, "PackedTex2UV alignment is wrong!");

struct PackedTexWithSomething {
    int16_t u0;
    int16_t v0;
    float   unknown;
};

static_assert(sizeof(PackedTexWithSomething) == 0x8, "PackedTexWithSomething alignment is wrong!");

struct UnpackedUV {
    float u;
    float v;
};
static_assert(sizeof(UnpackedUV) == 0x8, "UnpackedUV alignment is wrong!");

struct UnpackedNormals {
    float u0;
    float v0;
    float u1;
    float v1;
    float n;
    float t;
};
static_assert(sizeof(UnpackedNormals) == 0x18, "UnpackedNormals alignment is wrong!");

struct VertexDeformPos {
    float   x;
    float   y;
    float   z;
    float   d;
    int16_t wi0;
    int16_t wi1;
    int16_t wi2;
    int16_t wi3;
};
static_assert(sizeof(VertexDeformPos) == 0x18, "VertexDeformPos alignment is wrong!");

struct VertexDeformNormal2 {
    float u0;
    float v0;
    float u1;
    float v1;
    float n;
    float t;
    float dn;
    float dt;
};
static_assert(sizeof(VertexDeformNormal2) == 0x20, "VertexDeformNormal2 alignment is wrong!");

namespace RenderBlockCharacter
{
    static const int32_t VertexStrides[] = {0x18, 0x1C, 0x20, 0x20, 0x24, 0x28};

    struct Packed4Bones1UV {
        int16_t  x;
        int16_t  y;
        int16_t  z;
        int16_t  padding;
        uint8_t  w0[4];
        uint8_t  i0[4];
        int16_t  u0;
        int16_t  v0;
        uint32_t tangent;
    };
    static_assert(sizeof(Packed4Bones1UV) == 0x18, "Packed4Bones1UV alignment is wrong!");

    struct Packed4Bones2UVs {
        int16_t  x;
        int16_t  y;
        int16_t  z;
        int16_t  padding;
        uint8_t  w0[4];
        uint8_t  i0[4];
        int16_t  u0;
        int16_t  v0;
        int16_t  u1;
        int16_t  v1;
        uint32_t tangent;
    };
    static_assert(sizeof(Packed4Bones2UVs) == 0x1C, "Packed4Bones2UVs alignment is wrong!");

    struct Packed4Bones3UVs {
        int16_t  x;
        int16_t  y;
        int16_t  z;
        int16_t  padding;
        uint8_t  w0[4];
        uint8_t  i0[4];
        int16_t  u0;
        int16_t  v0;
        int16_t  u1;
        int16_t  v1;
        int16_t  u2;
        int16_t  v2;
        uint32_t tangent;
    };
    static_assert(sizeof(Packed4Bones3UVs) == 0x20, "Packed4Bones3UVs alignment is wrong!");

    struct Packed8Bones1UV {
        int16_t  x;
        int16_t  y;
        int16_t  z;
        int16_t  padding;
        uint8_t  w0[4];
        uint8_t  i0[4];
        uint8_t  w1[4];
        uint8_t  i1[4];
        int16_t  u0;
        int16_t  v0;
        uint32_t tangent;
    };
    static_assert(sizeof(Packed8Bones1UV) == 0x20, "Packed8Bones1UV alignment is wrong!");

    struct Packed8Bones2UVs {
        int16_t  x;
        int16_t  y;
        int16_t  z;
        int16_t  padding;
        uint8_t  w0[4];
        uint8_t  i0[4];
        uint8_t  w1[4];
        uint8_t  i1[4];
        int16_t  u0;
        int16_t  v0;
        int16_t  u1;
        int16_t  v1;
        uint32_t tangent;
    };
    static_assert(sizeof(Packed8Bones2UVs) == 0x24, "Packed8Bones2UVs alignment is wrong!");

    struct Packed8Bones3UVs {
        int16_t  x;
        int16_t  y;
        int16_t  z;
        int16_t  padding;
        uint8_t  w0[4];
        uint8_t  i0[4];
        uint8_t  w1[4];
        uint8_t  i1[4];
        int16_t  u0;
        int16_t  v0;
        int16_t  u1;
        int16_t  v1;
        int16_t  u2;
        int16_t  v2;
        uint32_t tangent;
    };
    static_assert(sizeof(Packed8Bones3UVs) == 0x28, "Packed8Bones3UVs alignment is wrong!");
}; // namespace RenderBlockCharacter

template <typename T> static inline T pack(float value)
{
#undef max
    return static_cast<T>(value / 1.0f * std::numeric_limits<T>::max());
}

template <typename T> static inline float unpack(T value)
{
#undef max
    return (value * 1.0f / std::numeric_limits<T>::max());
}

static inline float pack_normal(const glm::vec3& normal)
{
    float x = (((normal.x + 1.0f) * 127.0f) * 0.00390625f);
    float y = ((normal.y + 1.0f) * 127.0f);
    float z = (((normal.z + 1.0f) * 127.0f) * 256.0f);
    return (x + y + z);
}

static inline glm::vec3 unpack_normal(const float normal)
{
    float x = normal;
    float y = (normal / 256.0f);
    float z = (normal / 65536.0f);

    x -= glm::floor(x);
    y -= glm::floor(y);
    z -= glm::floor(z);

    x = (x * 2.0f) - 1.0f;
    y = (y * 2.0f) - 1.0f;
    z = (z * 2.0f) - 1.0f;

    return {x, y, z};
}

static inline float pack_colour(const glm::vec4& colour)
{
    return 0.0f;
}

static inline glm::vec4 unpack_colour(const float colour)
{
    return {};
}
}; // namespace jc::Vertex
#pragma pack(pop)
