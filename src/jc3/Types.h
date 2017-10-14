#pragma once

#include <cstdint>
#include <glm.hpp>

#pragma pack(push, 1)
namespace JustCause3
{
    namespace Vertex
    {
        struct SPackedAttribute
        {
            int32_t format;
            float scale;
            glm::vec2 uv0Extent;
            glm::vec2 uv1Extent;
            float colourExtent;
            uint8_t colour[4];
        };

        struct Packed
        {
            int16_t x;
            int16_t y;
            int16_t z;
            int16_t pad;
        };

        struct GeneralShortPacked
        {
            int16_t u0;
            int16_t v0;
            int16_t u1;
            int16_t v1;
            float n;
            float t;
            float col;
        };

        namespace RenderBlockCharacter
        {
            struct PackedCharacterPos4Bones1UVs
            {
                int16_t x;
                int16_t y;
                int16_t z;
                int16_t dummy;
                uint8_t w0[4];
                uint8_t i0[4];
                int16_t u0;
                int16_t v0;
                uint32_t tangent_space;
            };
        };

        namespace RenderBlockCharacterSkin
        {
            struct PackedCharacterSkinPos4Bones1UVs
            {
                int16_t x;
                int16_t y;
                int16_t z;
                int16_t dummy;
                uint8_t w0[4];
                uint8_t i0[4];
                int16_t u0;
                int16_t v0;
                uint32_t tangent_space;
            };
        };

        template <typename T>
        static T pack(float value) {
            return static_cast<T>(value / 1.0f * std::numeric_limits<T>::max());
        }

        template <typename T>
        static float unpack(T value) {
            return (value * 1.0f / std::numeric_limits<T>::max());
        }

        static float pack_vector() {
            return 0.0f;
        }

        static glm::vec3 unpack_vector(float packed, bool colour = false) {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;

            if (colour) {
                x = packed;
                y = packed / 64;
                z = packed / 4096;
            }
            else {
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

            return { x, y, z };
        }
    };

    struct AvalancheTextureStream
    {
        uint32_t m_Offset;
        uint32_t m_Size;
        uint16_t m_Alignment;
        char unknown[2];
    };

    struct AvalancheTexture
    {
        uint32_t m_Magic;
        uint8_t m_Version;
        char unknown[2];
        uint8_t m_Dimension;
        DXGI_FORMAT m_Format;
        uint16_t m_Width;
        uint16_t m_Height;
        uint16_t m_Depth;
        uint16_t m_Flags;
        uint8_t m_Mips;
        uint8_t m_MipsRedisent;
        char pad[10];
        AvalancheTextureStream m_Streams[8];
    };

    namespace ArchiveTable
    {
        struct VfsTabEntry
        {
            uint32_t hash;
            uint32_t offset;
            uint32_t size;
        };

        struct VfsCompressedTabEntry
        {
            uint32_t hash;
            uint32_t offset;
            uint32_t size;
            uint32_t decompressedSize;
        };

        struct VfsArchive
        {
            uint64_t magic;
            int64_t size;
            int64_t offset;
            uint32_t index;
            uint16_t version;
            std::vector<VfsTabEntry> entries;
        };

        struct TabFileHeader
        {
            uint32_t magic;
            uint16_t version;
            uint16_t endian;
            int32_t alignment;
        };
    };

    namespace StreamArchive
    {
        struct SARCHeader
        {
            uint32_t m_MagicLength;
            char m_Magic[4];
            int32_t m_Version;
            uint32_t m_Size;
        };
    };

    struct AvalancheArchiveHeader
    {
        char m_Magic[4];
        uint32_t m_Version;
        char m_Magic2[28];
        uint32_t m_TotalUncompressedSize;
        uint32_t m_BlockSize;
        uint32_t m_BlockCount;
    };

    struct AvalancheArchiveChunk
    {
        uint64_t m_DataOffset;
        uint32_t m_CompressedSize;
        uint32_t m_UncompressedSize;
        std::vector<uint8_t> m_BlockData;
    };

    struct RBM
    {
        uint32_t m_MagicLength;
        uint8_t m_Magic[5];
        uint32_t m_VersionMajor;
        uint32_t m_VersionMinor;
        uint32_t m_VersionRevision;
        glm::vec3 m_BoundingBoxMin;
        glm::vec3 m_BoundingBoxMax;
        uint32_t m_NumberOfBlocks;
        uint32_t m_Flags;
    };

    struct CSkinBatch
    {
        char pad[8];
        int32_t batchSize;
        int32_t size;
        int32_t offset;
    };
};
#pragma pack(pop)