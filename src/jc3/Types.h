#pragma once

#include <cstdint>
#include <glm.hpp>

#pragma pack(push, 1)
namespace JustCause3
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
            uint16_t u0;
            uint16_t v0;
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
            uint16_t u0;
            uint16_t v0;
            uint16_t u1;
            uint16_t v1;
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
            uint16_t u0;
            uint16_t v0;
            uint16_t u1;
            uint16_t v1;
            uint16_t u2;
            uint16_t v2;
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
            uint16_t u0;
            uint16_t v0;
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
            uint16_t u0;
            uint16_t v0;
            uint16_t u1;
            uint16_t v1;
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
            uint16_t u0;
            uint16_t v0;
            uint16_t u1;
            uint16_t v1;
            uint16_t u2;
            uint16_t v2;
            uint32_t tangent;
        };
        static_assert(sizeof(Packed8Bones3UVs) == 0x28, "Packed8Bones3UVs alignment is wrong!");
    }; // namespace RenderBlockCharacter

    template <typename T> static T pack(float value)
    {
#undef max
        return static_cast<T>(value / 1.0f * std::numeric_limits<T>::max());
    }

    template <typename T> static float unpack(T value)
    {
#undef max
        return (value * 1.0f / std::numeric_limits<T>::max());
    }

    static float pack_vector()
    {
        return 0.0f;
    }

    static glm::vec3 unpack_vector(float packed, bool colour = false)
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;

        if (colour) {
            x = packed;
            y = packed / 64;
            z = packed / 4096;
        } else {
            x = packed;
            y = packed / 256.0f;
            z = packed / 65536.0f;
        }

        x -= std::floorf(x);
        y -= std::floorf(y);
        z -= std::floorf(z);

        if (!colour) {
            x = x * 2 - 1;
            y = y * 2 - 1;
            z = z * 2 - 1;
        }

        return {x, y, z};
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
        bool     m_IsSource;
    };

    struct Header {
        uint32_t    m_Magic   = 0x58545641; // "AVTX"
        uint8_t     m_Version = 1;
        char        unknown[2];
        uint8_t     m_Dimension;
        DXGI_FORMAT m_Format;
        uint16_t    m_Width;
        uint16_t    m_Height;
        uint16_t    m_Depth;
        uint16_t    m_Flags;
        uint8_t     m_Mips;
        uint8_t     m_MipsRedisent;
        char        pad[10];
        Stream      m_Streams[8];
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

    static std::tuple<uint8_t, bool> FindBest(Header* texture, bool ignore_external = false)
    {
        uint8_t biggest      = 0;
        uint8_t stream_index = 0;
        bool    load_source  = false;

        for (uint8_t i = 0; i < 8; ++i) {
            const auto& stream = texture->m_Streams[i];

            // skip this stream if we have no data
            if (stream.m_Size == 0)
                continue;

            // find the biggest stream index
            if ((!ignore_external && stream.m_IsSource) || (!stream.m_IsSource && stream.m_Size > biggest)) {
                biggest      = stream.m_Size;
                stream_index = i;
                load_source  = stream.m_IsSource;
            }
        }

        return {stream_index, load_source};
    }
}; // namespace AvalancheTexture

namespace ArchiveTable
{
    struct VfsTabEntry {
        uint32_t m_NameHash;
        uint32_t m_Offset;
        uint32_t m_Size;
    };

    static_assert(sizeof(VfsTabEntry) == 0xC, "VfsTabEntry alignment is wrong!");

    struct VfsCompressedTabEntry {
        uint32_t m_Hash;
        uint32_t m_Offset;
        uint32_t m_Size;
        uint32_t m_DecompressedSize;
    };

    static_assert(sizeof(VfsCompressedTabEntry) == 0x10, "VfsCompressedTabEntry alignment is wrong!");

    struct VfsArchive {
        uint64_t                 m_Magic;
        int64_t                  m_Size;
        int64_t                  m_Offset;
        uint32_t                 m_Index;
        uint16_t                 m_Version;
        std::vector<VfsTabEntry> m_Entries;
    };

    struct TabFileHeader {
        uint32_t m_Magic     = 0x424154; // "TAB"
        uint16_t m_Version   = 2;
        uint16_t m_Endian    = 1;
        int32_t  m_Alignment = 0;
    };

    static_assert(sizeof(TabFileHeader) == 0xC, "TabFileHeader alignment is wrong!");
}; // namespace ArchiveTable

namespace StreamArchive
{
    struct SARCHeader {
        uint32_t m_MagicLength = 4;
        char     m_Magic[4]    = {'S', 'A', 'R', 'C'};
        uint32_t m_Version     = 2;
        uint32_t m_Size        = 0;
    };

    static_assert(sizeof(SARCHeader) == 0x10, "SARCHeader alignment is wrong!");
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
        uint32_t m_Data;
        uint8_t  m_Type;
    };

    static_assert(sizeof(Header) == 0x8, "RuntimeContainer Header alignment is wrong!");
    static_assert(sizeof(Node) == 0xC, "RuntimeContainer Node alignment is wrong!");
    static_assert(sizeof(Property) == 0x9, "RuntimeContainer Property alignment is wrong!");
}; // namespace RuntimeContainer

namespace AvalancheDataFormat
{
    struct Header {
        uint32_t    m_Magic            = 0x41444620; // "ADF "
        uint32_t    m_Version          = 4;
        uint32_t    m_InstanceCount    = 0;
        uint32_t    m_InstanceOffset   = 0;
        uint32_t    m_TypeCount        = 0;
        uint32_t    m_TypeOffset       = 0;
        uint32_t    m_StringHashCount  = 0;
        uint32_t    m_StringHashOffset = 0;
        uint32_t    m_StringCount      = 0;
        uint32_t    m_StringOffset     = 0;
        uint32_t    m_FileSize         = 0;
        char        _padding[20];
        char const* m_Description = "";
    };

    enum class TypeDefinitionType : uint32_t {
        Primitive   = 0,
        Structure   = 1,
        Pointer     = 2,
        Array       = 3,
        InlineArray = 4,
        String      = 5,
        BitField    = 7,
        Enumeration = 8,
        StringHash  = 9,
    };

    enum class TypeHashes : uint32_t {
        Int8   = 0x580D0A62,
        UInt8  = 0xCA2821D,
        Int16  = 0xD13FCF93,
        UInt16 = 0x86D152BD,
        Int32  = 0x192FE633,
        UInt32 = 0x75E4E4F,
        Int64  = 0xAF41354F,
        UInt64 = 0xA139E01F,
        Float  = 0x7515A207,
        Double = 0xC609F663,
        String = 0x8955583E,
    };

    static const char* TypeDefintionStr(TypeDefinitionType type)
    {
        switch (type) {
            case TypeDefinitionType::Primitive:
                return "Primitive";
            case TypeDefinitionType::Structure:
                return "Structure";
            case TypeDefinitionType::Pointer:
                return "Pointer";
            case TypeDefinitionType::Array:
                return "Array";
            case TypeDefinitionType::InlineArray:
                return "InlineArray";
            case TypeDefinitionType::String:
                return "String";
            case TypeDefinitionType::BitField:
                return "BitField";
            case TypeDefinitionType::Enumeration:
                return "Enumeration";
            case TypeDefinitionType::StringHash:
                return "StringHash";
        }

        return "Unknown";
    }

    struct TypeDefinition {
        TypeDefinitionType m_Type;
        uint32_t           m_Size;
        uint32_t           m_Alignment;
        uint32_t           m_NameHash;
        int64_t            m_NameIndex;
        uint32_t           m_Flags;
        uint32_t           m_ElementTypeHash;
        uint32_t           m_ElementLength;
    };

    struct InstanceInfo {
        uint32_t m_NameHash;
        uint32_t m_TypeHash;
        uint32_t m_Offset;
        uint32_t m_Size;
        int64_t  m_NameIndex;
    };

    static_assert(sizeof(Header) == 0x48, "AvalancheDataFormat Header alignment is wrong!");
    static_assert(sizeof(TypeDefinition) == 0x24, "AvalancheDataFormat TypeDefinition alignment is wrong!");
    static_assert(sizeof(InstanceInfo) == 0x18, "AvalancheDataFormat InstanceInfo alignment is wrong!");
} // namespace AvalancheDataFormat

template <typename T> inline static T ALIGN_TO_BOUNDARY(T& value, uint32_t alignment = sizeof(uint32_t))
{
    if ((value % alignment) != 0) {
        return (value + (alignment - (value % alignment)));
    }

    return value;
}

template <typename T> inline static uint32_t DISTANCE_TO_BOUNDARY(T& value, uint32_t alignment = sizeof(uint32_t))
{
    if ((value % alignment) != 0) {
        return (alignment - (value % alignment));
    }

    return 0;
}
}; // namespace JustCause3
#pragma pack(pop)
