#pragma once

#include "../types.h"

// jc4 types

#pragma pack(push, 1)
namespace jc4
{
namespace ArchiveTable
{
    struct TabFileHeader {
        uint32_t m_Magic                  = 0x424154; // "TAB"
        uint16_t m_Version                = 2;
        uint16_t m_Endian                 = 1;
        int32_t  m_Alignment              = 0;
        uint32_t _unknown                 = 0;
        uint32_t m_MaxCompressedBlockSize = 0;
        uint32_t m_UncompressedBlockSize  = 0;
    };

    static_assert(sizeof(TabFileHeader) == 0x18, "JC4 TabFileHeader alignment is wrong!");

    enum CompressionType : uint8_t {
        CompressionType_None  = 0,
        CompressionType_Zlib  = 1,
        CompressionType_Oodle = 4,
    };

    struct VfsTabEntry {
        uint32_t        m_NameHash;
        uint32_t        m_Offset;
        uint32_t        m_CompressedSize;
        uint32_t        m_UncompressedSize;
        uint16_t        m_CompressedBlockIndex;
        CompressionType m_CompressionType;
        uint8_t         m_Flags;
    };

    static_assert(sizeof(VfsTabEntry) == 0x14, "JC4 VfsTabEntry alignment is wrong!");

    struct VfsArchive {
        uint64_t                 m_Magic;
        int64_t                  m_Size;
        int64_t                  m_Offset;
        uint32_t                 m_Index;
        uint16_t                 m_Version;
        std::vector<VfsTabEntry> m_Entries;
    };

    struct VfsTabCompressedBlock {
        uint32_t m_CompressedSize;
        uint32_t m_UncompressedSize;
    };

    static_assert(sizeof(VfsTabCompressedBlock) == 0x8, "JC4 VfsTabCompressedBlock alignment is wrong!");
}; // namespace ArchiveTable

enum EAmfUsage {
    AmfUsage_Unspecified       = 0x0,
    AmfUsage_Position          = 0x1,
    AmfUsage_TextureCoordinate = 0x2,
    AmfUsage_Normal            = 0x3,
    AmfUsage_Tangent           = 0x4,
    AmfUsage_BiTangent         = 0x5,
    AmfUsage_TangentSpace      = 0x6,
    AmfUsage_BoneIndex         = 0x7,
    AmfUsage_BoneWeight        = 0x8,
    AmfUsage_Color             = 0x9,
    AmfUsage_WireRadius        = 0xA,
};

enum EAmfFormat {
    AmfFormat_R32G32B32A32_FLOAT          = 0x0,
    AmfFormat_R32G32B32A32_UINT           = 0x1,
    AmfFormat_R32G32B32A32_SINT           = 0x2,
    AmfFormat_R32G32B32_FLOAT             = 0x3,
    AmfFormat_R32G32B32_UINT              = 0x4,
    AmfFormat_R32G32B32_SINT              = 0x5,
    AmfFormat_R16G16B16A16_FLOAT          = 0x6,
    AmfFormat_R16G16B16A16_UNORM          = 0x7,
    AmfFormat_R16G16B16A16_UINT           = 0x8,
    AmfFormat_R16G16B16A16_SNORM          = 0x9,
    AmfFormat_R16G16B16A16_SINT           = 0xA,
    AmfFormat_R16G16B16_FLOAT             = 0xB,
    AmfFormat_R16G16B16_UNORM             = 0xC,
    AmfFormat_R16G16B16_UINT              = 0xD,
    AmfFormat_R16G16B16_SNORM             = 0xE,
    AmfFormat_R16G16B16_SINT              = 0xF,
    AmfFormat_R32G32_FLOAT                = 0x10,
    AmfFormat_R32G32_UINT                 = 0x11,
    AmfFormat_R32G32_SINT                 = 0x12,
    AmfFormat_R10G10B10A2_UNORM           = 0x13,
    AmfFormat_R10G10B10A2_UINT            = 0x14,
    AmfFormat_R11G11B10_FLOAT             = 0x15,
    AmfFormat_R8G8B8A8_UNORM              = 0x16,
    AmfFormat_R8G8B8A8_UNORM_SRGB         = 0x17,
    AmfFormat_R8G8B8A8_UINT               = 0x18,
    AmfFormat_R8G8B8A8_SNORM              = 0x19,
    AmfFormat_R8G8B8A8_SINT               = 0x1A,
    AmfFormat_R16G16_FLOAT                = 0x1B,
    AmfFormat_R16G16_UNORM                = 0x1C,
    AmfFormat_R16G16_UINT                 = 0x1D,
    AmfFormat_R16G16_SNORM                = 0x1E,
    AmfFormat_R16G16_SINT                 = 0x1F,
    AmfFormat_R32_FLOAT                   = 0x20,
    AmfFormat_R32_UINT                    = 0x21,
    AmfFormat_R32_SINT                    = 0x22,
    AmfFormat_R8G8_UNORM                  = 0x23,
    AmfFormat_R8G8_UINT                   = 0x24,
    AmfFormat_R8G8_SNORM                  = 0x25,
    AmfFormat_R8G8_SINT                   = 0x26,
    AmfFormat_R16_FLOAT                   = 0x27,
    AmfFormat_R16_UNORM                   = 0x28,
    AmfFormat_R16_UINT                    = 0x29,
    AmfFormat_R16_SNORM                   = 0x2A,
    AmfFormat_R16_SINT                    = 0x2B,
    AmfFormat_R8_UNORM                    = 0x2C,
    AmfFormat_R8_UINT                     = 0x2D,
    AmfFormat_R8_SNORM                    = 0x2E,
    AmfFormat_R8_SINT                     = 0x2F,
    AmfFormat_R32_UNIT_VEC_AS_FLOAT       = 0x30,
    AmfFormat_R32_R8G8B8A8_UNORM_AS_FLOAT = 0x31,
    AmfFormat_R8G8B8A8_TANGENT_SPACE      = 0x32,
};
}; // namespace jc4
#pragma pack(pop)
