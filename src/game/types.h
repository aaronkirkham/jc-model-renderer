#pragma once

#include <cstdint>
#include <dxgiformat.h>
#include <glm/glm.hpp>

#include "jc3/types.h"
#include "jc4/types.h"

#pragma pack(push, 8)
template <typename T> struct AdfArray {
    T*       m_Data;
    uint32_t m_Count;
};

struct SAdfDeferredPtr {
    void*    m_Ptr;
    uint32_t m_Type;
};

static_assert(sizeof(AdfArray<void>) == 0x10, "AdfArray alignment is wrong!");
static_assert(sizeof(SAdfDeferredPtr) == 0x10, "SAdfDeferredPtr alignment is wrong!");
#pragma pack(pop)

#pragma pack(push, 1)
namespace jc
{
namespace Vertex
{
    struct SPackedAttribute {
        int32_t   format;
        float     scale = 1.0f;
        glm::vec2 uv0Extent;
        glm::vec2 uv1Extent;
        float     colourExtent;
        uint8_t   colour[4];
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
        return x + y + z;
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
}; // namespace Vertex

namespace AvalancheTexture
{
    enum Flags : uint32_t {
        STREAMED    = 0x1,
        PLACEHOLDER = 0x2,
        TILED       = 0x4,
        SRGB        = 0x8,
        CUBE        = 0x40,
    };

    struct Stream {
        uint32_t m_Offset;
        uint32_t m_Size;
        uint16_t m_Alignment;
        bool     m_IsTile;
        uint8_t  m_Source;
    };

    struct Header {
        uint32_t    m_Magic    = 0x58545641; // "AVTX"
        uint8_t     m_Version  = 1;
        char        unknown[2] = {};
        uint8_t     m_Dimension;
        DXGI_FORMAT m_Format;
        uint16_t    m_Width;
        uint16_t    m_Height;
        uint16_t    m_Depth;
        uint16_t    m_Flags;
        uint8_t     m_Mips;
        uint8_t     m_MipsRedisent;
        char        pad[10]      = {};
        Stream      m_Streams[8] = {};
    };

    static uint32_t GetHighestRank(Header* texture, uint32_t stream_index)
    {
        uint32_t rank = 0;
        for (uint32_t i = 0; i < 8; ++i) {
            if (i == stream_index)
                continue;

            if (texture->m_Streams[i].m_Size > texture->m_Streams[stream_index].m_Size) {
                ++rank;
            }
        }

        return rank;
    }

    static uint8_t FindBest(Header* texture, uint8_t source)
    {
        uint8_t biggest_seen = 0;
        uint8_t stream_index = 0;

        for (uint8_t i = 0; i < 8; ++i) {
            const auto& stream = texture->m_Streams[i];
            if (stream.m_Size == 0) {
                continue;
            }

            // find the best stream to use
            if ((stream.m_Source == source) || (stream.m_Source == 0 && stream.m_Size > biggest_seen)) {
                biggest_seen = stream.m_Size;
                stream_index = i;
            }
        }

        return stream_index;
    }
}; // namespace AvalancheTexture

namespace StreamArchive
{
    struct Header {
        uint32_t m_MagicLength = 4;
        char     m_Magic[4]    = {'S', 'A', 'R', 'C'};
        uint32_t m_Version     = 2;
        uint32_t m_Size        = 0;
    };

    static_assert(sizeof(Header) == 0x10, "StreamArchive header alignment is wrong!");
}; // namespace StreamArchive

namespace AvalancheArchive
{
    struct Header {
        char     m_Magic[4] = {'A', 'A', 'F'};
        uint32_t m_Version  = 1;
        char     m_Magic2[28];
        uint32_t m_TotalUncompressedSize  = 0;
        uint32_t m_UncompressedBufferSize = 0;
        uint32_t m_ChunkCount             = 0;

        Header()
        {
            strncpy(m_Magic2, "AVALANCHEARCHIVEFORMATISCOOL", 29);
        }
    };

    struct Chunk {
        uint32_t             m_CompressedSize   = 0;
        uint32_t             m_UncompressedSize = 0;
        uint32_t             m_DataSize         = 0;
        uint32_t             m_Magic            = 0x4D415745; // "EWAM"
        std::vector<uint8_t> m_BlockData;
    };

    static constexpr auto CHUNK_SIZE          = (sizeof(uint32_t) * 4);
    static constexpr auto MAX_CHUNK_DATA_SIZE = 33554432;
}; // namespace AvalancheArchive

struct RBMHeader {
    uint32_t  m_MagicLength     = 5;
    uint8_t   m_Magic[5]        = {'R', 'B', 'M', 'D', 'L'};
    uint32_t  m_VersionMajor    = 1;
    uint32_t  m_VersionMinor    = 16;
    uint32_t  m_VersionRevision = 0;
    glm::vec3 m_BoundingBoxMin;
    glm::vec3 m_BoundingBoxMax;
    uint32_t  m_NumberOfBlocks = 0;
    uint32_t  m_Flags          = 0;
};

static_assert(sizeof(RBMHeader) == 0x35, "RBMHeader alignment is wrong!");

struct CSkinBatch {
    int16_t* m_BatchLookup;
    int32_t  m_BatchSize;
    int32_t  m_Size;
    int32_t  m_Offset;
    char     pad2[4];
};

static_assert(sizeof(CSkinBatch) == 0x18, "CSkinBatch alignment is wrong!");

struct CDeformTable {
    uint16_t table[256];
    uint8_t  size = 0;
};

static_assert(sizeof(CDeformTable) == 0x201, "CDeformTable alignment is wrong!");

namespace RuntimeContainer
{
    struct Header {
        char     m_Magic[4] = {'R', 'T', 'P', 'C'};
        uint32_t m_Version  = 1;
    };

    struct Node {
        uint32_t m_NameHash;
        uint32_t m_DataOffset;
        uint16_t m_PropertyCount;
        uint16_t m_InstanceCount;
    };

    struct Property {
        uint32_t m_NameHash;
        uint32_t m_DataOffset;
        uint8_t  m_Type;
    };

    static_assert(sizeof(Header) == 0x8, "RuntimeContainer Header alignment is wrong!");
    static_assert(sizeof(Node) == 0xC, "RuntimeContainer Node alignment is wrong!");
    static_assert(sizeof(Property) == 0x9, "RuntimeContainer Property alignment is wrong!");
}; // namespace RuntimeContainer

namespace AvalancheDataFormat
{
    enum class EAdfType : uint32_t {
        Primitive   = 0,
        Structure   = 1,
        Pointer     = 2,
        Array       = 3,
        InlineArray = 4,
        String      = 5,
        Unknown     = 6,
        BitField    = 7,
        Enumeration = 8,
        StringHash  = 9,
        Deferred    = 10,
    };

    enum class ScalarType : uint16_t {
        Signed   = 0,
        Unsigned = 1,
        Float    = 2,
    };

    enum EHeaderFlags {
        RELATIVE_OFFSETS_EXISTS        = 0x1,
        HAS_INLINE_ARRAY_WITH_POINTERS = 0x2,
    };

    struct HeaderV2 {
        uint32_t m_Magic = 0x41444620; // "ADF "
        uint32_t m_Version;
        uint32_t m_InstanceCount;
        uint32_t m_FirstInstanceOffset;
        uint32_t m_TypeCount;
        uint32_t m_FirstTypeOffset;
    };

    struct HeaderV3 : HeaderV2 {
        uint32_t m_StringHashCount;
        uint32_t m_FirstStringHashOffset;
    };

    struct Header : HeaderV3 {
        uint32_t    m_StringCount;
        uint32_t    m_FirstStringDataOffset;
        uint32_t    m_FileSize;
        uint32_t    unknown;
        uint32_t    m_Flags;
        uint32_t    m_IncludedLibraries;
        char        gap38[8];
        const char* m_Description;
    };

    static_assert(sizeof(HeaderV3) == 0x20, "AvalancheDataFormat HeaderV3 alignment is wrong.");
    static_assert(sizeof(HeaderV2) == 0x18, "AvalancheDataFormat HeaderV2 alignment is wrong.");
    static_assert(sizeof(Header) == 0x48, "AvalancheDataFormat Header alignment is wrong.");

    struct Type {
        EAdfType   m_AdfType;
        uint32_t   m_Size;
        uint32_t   m_Align;
        uint32_t   m_TypeHash;
        uint64_t   m_Name;
        uint16_t   m_Flags;
        ScalarType m_ScalarType;
        uint32_t   m_SubTypeHash;
        uint32_t   m_ArraySizeOrBitCount;
        uint32_t   m_MemberCount;
    };

    struct TypeV3 {
        EAdfType   m_AdfType;
        uint32_t   m_Size;
        uint32_t   m_Align;
        uint32_t   m_TypeHash;
        char       m_Name[64];
        uint16_t   m_Flags;
        ScalarType m_ScalarType;
        uint32_t   m_SubTypeHash;
        uint32_t   m_ArraySizeOrBitCount;
        uint32_t   m_MemberCount;
    };

    static_assert(sizeof(Type) == 0x28, "AvalancheDataFormat Type alignment is wrong.");
    static_assert(sizeof(TypeV3) == 0x60, "AvalancheDataFormat TypeV3 alignment is wrong.");

    struct Instance {
        uint32_t m_NameHash;
        uint32_t m_TypeHash;
        uint32_t m_PayloadOffset;
        uint32_t m_PayloadSize;
        uint64_t m_Name;
    };

    struct InstanceV3 {
        uint32_t m_NameHash;
        uint32_t m_TypeHash;
        uint32_t m_PayloadOffset;
        uint32_t m_PayloadSize;
        char     m_Name[32];
    };

    static_assert(sizeof(Instance) == 0x18, "AvalancheDataFormat Instance alignment is wrong.");
    static_assert(sizeof(InstanceV3) == 0x30, "AvalancheDataFormat InstanceV3 alignment is wrong.");

    struct Member {
        uint64_t m_Name;
        uint32_t m_TypeHash;
        uint32_t m_Align;
        uint32_t m_Offset : 24;
        uint32_t m_BitOffset : 8;
        uint32_t m_Flags;
        uint64_t m_DefaultValue;
    };

    struct MemberV3 {
        char     m_Name[32];
        uint32_t m_TypeHash;
        uint32_t m_Align;
        uint32_t m_Offset : 24;
        uint32_t m_BitOffset : 8;
        uint32_t m_Flags;
        uint64_t m_DefaultValue;
    };

    static_assert(sizeof(Member) == 0x20, "AvalancheDataFormat Member alignment is wrong.");
    static_assert(sizeof(MemberV3) == 0x38, "AvalancheDataFormat MemberV3 alignment is wrong.");

    struct Enum {
        uint64_t m_Name;
        int32_t  m_Value;
    };

    struct EnumV3 {
        char    m_Name[96];
        int32_t m_Value;
    };

    static_assert(sizeof(Enum) == 0xC, "AvalancheDataFormat Enum alignment is wrong.");
    static_assert(sizeof(EnumV3) == 0x64, "AvalancheDataFormat EnumV3 alignment is wrong.");

    struct SInstanceInfo {
        uint32_t    m_NameHash;
        uint32_t    m_TypeHash;
        const char* m_Name;
        const void* m_Instance;
        uint32_t    m_InstanceSize;
    };

    static_assert(sizeof(SInstanceInfo) == 0x1C, "SInstanceInfo alignment is wrong.");
} // namespace AvalancheDataFormat

template <typename T> inline static T ALIGN_TO_BOUNDARY(T value, uint32_t alignment = sizeof(uint32_t))
{
    if ((value % alignment) != 0) {
        return (value + (alignment - (value % alignment)));
    }

    return value;
}

template <typename T> inline static uint32_t DISTANCE_TO_BOUNDARY(T value, uint32_t alignment = sizeof(uint32_t))
{
    if ((value % alignment) != 0) {
        return (alignment - (value % alignment));
    }

    return 0;
}
}; // namespace jc
#pragma pack(pop)
